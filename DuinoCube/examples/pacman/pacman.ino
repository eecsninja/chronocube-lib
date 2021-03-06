// Copyright (C) 2013 Simon Que
//
// This file is part of DuinoCube.
//
// DuinoCube is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DuinoCube is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DuinoCube.  If not, see <http://www.gnu.org/licenses/>.

// A Pacman port.

#include <DuinoCube.h>
#include <SPI.h>

#include "defines.h"
#include "map.h"
#include "resources.h"
#include "sprites.h"

#define GHOST_MOVEMENT_SPEED      1
#define PLAYER_MOVEMENT_SPEED     1

#define GHOST_ANIMATION_PERIOD    8
#define GHOST_FRAME_PERIOD        2

#define PLAYER_ANIMATION_PERIOD   4
#define PLAYER_FRAME_PERIOD       4

// Array of all sprites.
Sprite g_sprites[NUM_GHOSTS + 1];

// Actual sprite objects.
Sprite& g_player = g_sprites[0];
Sprite* g_ghosts = g_sprites + 1;

extern uint8_t __bss_end;   // End of statically allocated memory.
extern uint8_t __stack;     // Where local variables are allocated.

// Initialize sprites.
static void initSprites() {
  // Initialize player sprite.
  g_player.state = SPRITE_ALIVE;
  g_player.dir = SPRITE_RIGHT;
  g_player.x = PLAYER_START_X;
  g_player.y = PLAYER_START_Y;

  g_player.base_offset = g_sprite_offset + PLAYER_SPRITE_BASE_OFFSET;
  g_player.size = SPRITE_SIZE;

  // Initialize ghost sprites.
  for (int i = 0; i < NUM_GHOSTS; ++i) {
    Sprite& ghost = g_ghosts[i];
    ghost.state = SPRITE_ALIVE;
    ghost.dir = (i < NUM_GHOSTS / 2) ? SPRITE_LEFT : SPRITE_RIGHT;
    ghost.x = GHOST_START_X0 + i * GHOST_START_DX;
    ghost.y = GHOST_START_Y;

    ghost.base_offset = g_sprite_offset +
                        GHOST_SPRITE_BASE_OFFSET +
                        i * SPRITE_SIZE * NUM_FRAMES_PER_GHOST;
    ghost.size = SPRITE_SIZE;
  }
}

// Update ghost sprite frame and orientation.
static void setGhostFrame(Sprite* ghost_ptr) {
  Sprite& ghost = *ghost_ptr;

  uint8_t frame_offset =
      (ghost.counter / GHOST_ANIMATION_PERIOD) % GHOST_FRAME_PERIOD;

  // These are based on the image offsets in the sprite image data.
  // TODO: make it less hard-coded.
  switch (ghost.dir) {
  case SPRITE_UP:
    ghost.frame = 4 + frame_offset;
    ghost.flip = 0;
    break;
  case SPRITE_DOWN:
    ghost.frame = 2 + frame_offset;
    ghost.flip = 0;
    break;
  case SPRITE_LEFT:
  case SPRITE_RIGHT:
    ghost.frame = 0 + frame_offset;
    ghost.flip = (ghost.dir == SPRITE_RIGHT) ? 0 : (1 << SPRITE_FLIP_X);
    break;
  }
}

// Update player sprite frame and orientation.
static void setPlayerFrame(Sprite* player_ptr) {
  Sprite& player = *player_ptr;

  uint8_t frame_offset =
      (player.counter / PLAYER_ANIMATION_PERIOD) % PLAYER_FRAME_PERIOD;

  // These are based on the image offsets in the sprite image data.
  // TODO: make it less hard-coded.
  const uint8_t player_frames[] = { 1, 2, 1, 0 };
  player.frame = player_frames[frame_offset];

  // Determine orientation.
  switch (player.dir) {
  case SPRITE_UP:
    player.flip = (1 << SPRITE_FLIP_X) | (1 << SPRITE_FLIP_XY);
    break;
  case SPRITE_DOWN:
    player.flip = (1 << SPRITE_FLIP_XY);
    break;
  case SPRITE_LEFT:
    player.flip = (1 << SPRITE_FLIP_X);
    break;
  case SPRITE_RIGHT:
    player.flip = 0;
    break;
  }
}

// Handle ghost state and movement.
static void updateGhosts() {
  for (int i = 0; i < NUM_GHOSTS; ++i) {
    Sprite& ghost = g_ghosts[i];
    // Is ghost at an intersection? If so, randomly choose an available
    // direction that doesn't entail going backwards.
    // TODO: Correctly handle a dead end.
    if (isAtIntersection(ghost)) {
      // Count the number of available directions.
      uint8_t new_dirs[NUM_SPRITE_DIRS];
      uint8_t num_available_dirs = 0;
      for (uint8_t dir = 0; dir < NUM_SPRITE_DIRS; ++dir) {
        if (getOppositeDir(ghost.dir) == dir)  // Do not go backwards.
          continue;

        // Otherwise, if the new direction is not blocked, add it to the list of
        // possible directions.
        const Vector& dir_vector = getDirVector(dir);
        if (isEmptyTile(getTileX(ghost.x) + dir_vector.x,
                        getTileY(ghost.y) + dir_vector.y)) {
          new_dirs[num_available_dirs++] = dir;
        }
      }

      // Choose a direction from the list.
      if (num_available_dirs > 0) {
        ghost.dir = new_dirs[rand() % num_available_dirs];
      } else {
        printf("Unable to find a new direction for ghost at (%u, %u)\n",
               ghost.x, ghost.y);
      }
    }

    // Update ghost location.
    updateSprite(&ghost, GHOST_MOVEMENT_SPEED);

    // Update ghost sprite frame.
    setGhostFrame(&ghost);
  }
}

// Handle player.
static void updatePlayer() {
  // Read user input.
  GamepadState gamepad = DC.Gamepad.readGamepad();

  bool changed_dir = false;

  // Handle directional pad input.
  if ((gamepad.x != 0) || (gamepad.y != 0)) {
    uint8_t new_dir;
    if (gamepad.x < 0) {           // User is pressing left.
      new_dir = SPRITE_LEFT;
    } else if (gamepad.x > 0) {    // User is pressing right.
      new_dir = SPRITE_RIGHT;
    } else if (gamepad.y < 0) {    // User is pressing up.
      new_dir = SPRITE_UP;
    } else if (gamepad.y > 0) {    // User is pressing down.
      new_dir = SPRITE_DOWN;
    }

    bool is_valid_dir = false;

    // Don't bother if the direction is the same.
    if (new_dir == g_player.dir) {
      is_valid_dir = false;
    } else if (getOppositeDir(g_player.dir) == new_dir) {
      // Player can always turn around to the opposite direction.
      is_valid_dir = true;
    } else if (isAtIntersection(g_player)) {
      // Otherwise, player can change directions if at an intersection and there
      // is a path in the new direction.
      const Vector& dir_vector = getDirVector(new_dir);
      if (isEmptyTile(getTileX(g_player.x) + dir_vector.x,
                      getTileY(g_player.y) + dir_vector.y)) {
        is_valid_dir = true;
      }
    }

    // Use the new direction if appropriate.
    if (is_valid_dir) {
      g_player.dir = new_dir;
      changed_dir = true;
    }
  }

  // Update player sprite if it can continue to move.
  const Vector& dir_vector = getDirVector(g_player.dir);
  if (changed_dir ||
      !isAtIntersection(g_player) ||
      isEmptyTile(getTileX(g_player.x) + dir_vector.x,
                  getTileY(g_player.y) + dir_vector.y)) {
    updateSprite(&g_player, PLAYER_MOVEMENT_SPEED);
    setPlayerFrame(&g_player);

    // Eat a dot if one is available.
    if (isAlignedToTileGrid(g_player)) {
      DC.Core.writeWord(REG_MEM_BANK, TILEMAP_BANK);
      DC.Core.writeWord(TILEMAP(DOTS_TILEMAP_INDEX) +
                        (getTileX(g_player.x) +
                         getTileY(g_player.y) * TILEMAP_WIDTH) *
                            TILEMAP_ENTRY_SIZE,
                        DEFAULT_EMPTY_TILE_VALUE);
      DC.Core.writeWord(REG_MEM_BANK, 0);
    }
  }
}

// Scroll world camera based on player location.
static void scrollCamera() {
  // Center the camera on the player if possible, but try to keep the camera
  // within the map.
  if (g_player.y < SCREEN_HEIGHT / 2) {
    // Camera at top.
    DC.Core.writeWord(REG_SCROLL_Y, 0);
  } else if (MAP_HEIGHT - g_player.y < SCREEN_HEIGHT / 2) {
    // Camera at bottom.
    DC.Core.writeWord(REG_SCROLL_Y, MAP_HEIGHT - SCREEN_HEIGHT);
  } else {
    // Camera in the middle.
    DC.Core.writeWord(REG_SCROLL_Y, g_player.y - SCREEN_HEIGHT / 2);
  }

  // Center the camera horizontally.
  // TODO: This is based on the assumption that |MAP_WIDTH| <= |SCREEN_WIDTH|.
  // To make it more robust, add a check that this is the case.
  // Do something similar for vertical centering above.
  DC.Core.writeWord(REG_SCROLL_X, (MAP_WIDTH - SCREEN_WIDTH) / 2);
}

void setup() {
  Serial.begin(115200);
  DC.begin();

  loadResources();
  initSprites();

  setupLayers();
  setupSprites(g_sprites, sizeof(g_sprites) / sizeof(g_sprites[0]));

  scrollCamera();

  // Initialize directional unit vectors.
  setDirVector(SPRITE_UP, 0, -1);
  setDirVector(SPRITE_DOWN, 0, 1);
  setDirVector(SPRITE_LEFT, -1, 0);
  setDirVector(SPRITE_RIGHT, 1, 0);

  printf("Static data ends at 0x%04x\n", &__bss_end);
  printf("Stack is at 0x%04x\n", &__stack);

  // Initialize random generator with time.
  // TODO: Use a less deterministic seed value.
  srand(millis());
}

void loop() {
  // Wait for visible, non-vblanked region to do computations.
  while ((DC.Core.readWord(REG_OUTPUT_STATUS) & (1 << REG_VBLANK)));

  // Game logic goes here.
  updateGhosts();
  updatePlayer();

  // TODO: handle collisions.

  // Wait for Vblank to update rendering.
  while (!(DC.Core.readWord(REG_OUTPUT_STATUS) & (1 << REG_VBLANK)));

  scrollCamera();

  // Update sprite rendering.
  for (int i = 0; i < sizeof(g_sprites) / sizeof(g_sprites[0]); ++i) {
    const Sprite& sprite = g_sprites[i];

    // Update location.
    DC.Core.writeWord(SPRITE_REG(i, SPRITE_OFFSET_X), sprite.x);
    DC.Core.writeWord(SPRITE_REG(i, SPRITE_OFFSET_Y), sprite.y);

    // Update image.
    uint16_t ctrl0 = DC.Core.readWord(SPRITE_REG(i, SPRITE_CTRL_0));
    DC.Core.writeWord(SPRITE_REG(i, SPRITE_CTRL_0),
                      (ctrl0 & ~SPRITE_FLIP_MASK) | sprite.flip);
    DC.Core.writeWord(SPRITE_REG(i, SPRITE_DATA_OFFSET), sprite.get_offset());
  }
}
