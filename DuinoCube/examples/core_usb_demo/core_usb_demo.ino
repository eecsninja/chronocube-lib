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

// Test DuinoCube tile layers, sprites, and USB input.

#include <DuinoCube.h>
#include <SPI.h>

#define CLOUD_LAYER             3       // Cloud layer index.
#define EMPTY_TILE         0x1fff       // Empty tile value.
#define COLOR_KEY            0xff       // Transparent pixel value.

#define PLAYER_SPRITE           0       // Index of player-controlled sprite.

#define MOVEMENT_STEP           8       // Amount by which to increment movement
                                        // counter each game cycle.

// Files to load.
const char* kImageFiles[] = {
  "data/tileset.raw",
  "data/clouds.raw",
  "data/sprite32.raw",
};

const char* kPaletteFiles[] = {
  "data/tileset.pal",
  "data/clouds.pal",
  "data/sprites.pal",
};

const char* kTilemapFiles[] = {
  "data/desert0.lay",
  "data/desert1.lay",
  "data/desert2.lay",
  "data/clouds.lay",
};

// The order of these should match the order of image data being loaded.
static uint16_t landscape_offset, clouds_offset, sprites_offset;
static uint16_t* vram_offsets[] = {
  &landscape_offset,
  &clouds_offset,
  &sprites_offset,
};

// The order of these should match the order of palette data being loaded.
static uint8_t landscape_pal, clouds_pal, sprites_pal;
static uint8_t* palettes[] = { &landscape_pal, &clouds_pal, &sprites_pal };

static void load() {
  // Load palettes.
  printf("Loading palettes.\n");
  for (int i = 0; i < ARRAY_SIZE(kPaletteFiles); ++i) {
    const char* filename = kPaletteFiles[i];
    if (!DC.Core.loadPalette(filename, i)) {
      printf("Error loading palette from file %s.\n", filename);
      continue;
    }
    *palettes[i] = i;
  }

  // Load layers.
  printf("Loading layers.\n");
  for (int i = 0; i < ARRAY_SIZE(kTilemapFiles); ++i) {
    const char* filename = kTilemapFiles[i];
    if (!DC.Core.loadTilemap(filename, i)) {
      printf("Error loading tilemap from file %s.\n", filename);
      continue;
    }
  }

  // Load images.
  printf("Loading images.\n");
  uint32_t vram_offset = 0;
  for (int i = 0; i < ARRAY_SIZE(kImageFiles); ++i) {
    const char* filename = kImageFiles[i];
    uint32_t size_read = DC.Core.loadImageData(filename, vram_offset);
    if (size_read == 0) {
      printf("Could not open file %s.\n", filename);
      continue;
    }
    *vram_offsets[i] = vram_offset;
    vram_offset += size_read;
  }
}

static void draw() {
  // Start camera at (0, 0).
  DC.Core.moveCamera(0, 0);

  // Set the sprite rendering depth to be just below the cloud layer.
  DC.Core.setSpriteDepth(CLOUD_LAYER - 1);

  for (int layer = 0; layer < ARRAY_SIZE(kTilemapFiles); ++layer) {
    uint8_t palette = (layer == CLOUD_LAYER) ? clouds_pal : landscape_pal;
    uint16_t offset = (layer == CLOUD_LAYER) ? clouds_offset : landscape_offset;

    // Set layer properties.
    DC.Core.setTileLayerProperty(layer, TILE_PROP_FLAGS,
                                 (TILE_FLAGS_ENABLE_EMPTY |
                                  TILE_FLAGS_ENABLE_TRANSP |
                                  TILE_FLAGS_ENABLE_FLIP));
    DC.Core.setTileLayerProperty(layer, TILE_PROP_PALETTE, palette);
    DC.Core.setTileLayerProperty(layer, TILE_PROP_EMPTY_VALUE, EMPTY_TILE);
    DC.Core.setTileLayerProperty(layer, TILE_PROP_TRANSP_VALUE, COLOR_KEY);
    DC.Core.setTileLayerProperty(layer, TILE_PROP_DATA_OFFSET, offset);
    DC.Core.moveTileLayer(layer, 0, 0);

    // Turn the layer.
    DC.Core.enableTileLayer(layer);
  }

  // Set up sprite.

  // Set to 32x32 size.
  DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_WIDTH, SPRITE_SIZE_32);
  DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_HEIGHT, SPRITE_SIZE_32);
  // Set image data offset.
  DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_DATA_OFFSET,
                            sprites_offset);
  // Set transparency.
  DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_TRANSP_VALUE, COLOR_KEY);
  // Set location.
  DC.Core.moveSprite(PLAYER_SPRITE, 0, 0);
  // Set palette.
  DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_PALETTE, sprites_pal);
  // Set flags.
  DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_FLAGS,
                            SPRITE_FLAGS_ENABLE_TRANSP);

  // Enable the sprite.
  DC.Core.enableSprite(PLAYER_SPRITE);
}

void setup() {
  Serial.begin(115200);

  DC.begin();

  load();
  draw();
}

#define SCREEN_WIDTH          320       // Screen dimensions.
#define SCREEN_HEIGHT         240       // TODO: de-hardcode this.

#define SPRITE_WIDTH           32       // Dimensions of 32x32 sprite.
#define SPRITE_HEIGHT          32
#define SPRITE_SIZE        (SPRITE_WIDTH * SPRITE_HEIGHT)
#define NUM_SPRITE_IMAGES       4

#define MAX_MOVEMENT_SPEED      3

void loop() {
  // Initialize the player sprite location and image address.
  int16_t player_x = 0;
  int16_t player_y = 0;
  uint16_t player_offset = sprites_offset;

  // Current camera location.
  int16_t scroll_x = 0;
  int16_t scroll_y = 0;

  // Keep a copy of the previous gamepad state to detect button press and
  // release events.
  GamepadState prev_gamepad;
  prev_gamepad.buttons = 0;
  prev_gamepad.x = 0;
  prev_gamepad.y = 0;

  // Keep track of changes in orientation.
  uint16_t old_flip_flags = SPRITE_FLIP_NONE;

  // Player movement speed based on gamepad input.
  int8_t dx = 0;
  int8_t dy = 0;

  // Adjustable sprite rendering depth relative to tile layers.
  uint8_t sprite_z = CLOUD_LAYER - 1;

  // Counter for moving the clouds.
  uint16_t movement_count = 0;

  // This loop runs forever. In a real game, set |done| to true at some point so
  // that the loop exits (e.g. to the main menu).
  bool done = false;
  while (!done) {
    // Wait for visible, non-vblanked region to do computations.
    DC.Core.waitForEvent(CORE_EVENT_VBLANK_END);

    // Read user input.
    GamepadState gamepad = DC.Gamepad.readGamepad();

    // The four main control buttons select the orientation.
    uint16_t new_flip_flags = old_flip_flags;
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_1)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_1))) {
      new_flip_flags = SPRITE_FLIP_NONE;
    }
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_2)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_2))) {
      new_flip_flags = (SPRITE_FLIP_VERT | SPRITE_FLIP_DIAG);
    }
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_3)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_3))) {
      new_flip_flags = SPRITE_FLIP_VERT;
    }
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_4)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_4))) {
      new_flip_flags = SPRITE_FLIP_DIAG;
    }

    // L1 and R1 cycle sprite through different images.
    int sprite_image_index =
        (player_offset - sprites_offset) / SPRITE_SIZE;
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_L1)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_L1))) {
      --sprite_image_index;
    }
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_R1)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_R1))) {
      ++sprite_image_index;
    }
    // Adjust for valid image index values.
    sprite_image_index =
        (sprite_image_index + NUM_SPRITE_IMAGES) % NUM_SPRITE_IMAGES;
    player_offset = sprites_offset + sprite_image_index * SPRITE_SIZE;

    // L2 and R2 buttons to change sprite Z-level.
    if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_R2)) &&
        !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_R2)) &&
        sprite_z < NUM_TILE_LAYERS - 1) {
      ++sprite_z;
    } else if ((gamepad.buttons & (1 << GAMEPAD_BUTTON_L2)) &&
               !(prev_gamepad.buttons & (1 << GAMEPAD_BUTTON_L2)) &&
               sprite_z > 0) {
      --sprite_z;
    }

    // Directional pad moves sprite.
    if (gamepad.x < 0) {
      // Use acceleration.
      if (prev_gamepad.x != gamepad.x)
        dx = -1;
      else if (dx > -MAX_MOVEMENT_SPEED)
        --dx;
      player_x += dx;
    }
    else if (gamepad.x > 0) {
      // Use acceleration.
      if (prev_gamepad.x != gamepad.x)
        dx = 1;
      else if (dx < MAX_MOVEMENT_SPEED)
        ++dx;
      player_x += dx;
    }

    if (gamepad.y < 0) {
      // Use acceleration.
      if (prev_gamepad.y != gamepad.y)
        dy = -1;
      else if (dy > -MAX_MOVEMENT_SPEED)
        --dy;
      player_y += dy;
    }
    else if (gamepad.y > 0) {
      // Use acceleration.
      if (prev_gamepad.y != gamepad.y)
        dy = 1;
      else if (dy < MAX_MOVEMENT_SPEED)
        ++dy;
      player_y += dy;
    }

    // Save the current gamepad state for the next cycle.
    prev_gamepad = gamepad;

    if (player_x < scroll_x)
      scroll_x = player_x;
    else if (player_x >= scroll_x + SCREEN_WIDTH - SPRITE_WIDTH)
      scroll_x = player_x + SPRITE_WIDTH - SCREEN_WIDTH;

    if (player_y < scroll_y)
      scroll_y = player_y;
    else if (player_y >= scroll_y + SCREEN_HEIGHT - SPRITE_HEIGHT)
      scroll_y = player_y + SPRITE_HEIGHT - SCREEN_HEIGHT;

    // Update the cloud movement.
    uint16_t clouds_x = (movement_count / 8);
    uint16_t clouds_y = -(movement_count / 16);
    movement_count += MOVEMENT_STEP;

    // Wait for Vblank.
    DC.Core.waitForEvent(CORE_EVENT_VBLANK_BEGIN);

    // Scroll the camera.
    DC.Core.moveCamera(scroll_x, scroll_y);

    // Scroll the cloud layer independently.
    DC.Core.moveTileLayer(CLOUD_LAYER, clouds_x, clouds_y);

    // Update the sprite.
    if (new_flip_flags != old_flip_flags) {
      DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_ORIENTATION,
                                new_flip_flags);
      old_flip_flags = new_flip_flags;
    }
    DC.Core.setSpriteProperty(PLAYER_SPRITE, SPRITE_PROP_DATA_OFFSET,
                              player_offset);
    DC.Core.moveSprite(PLAYER_SPRITE, player_x, player_y);
    DC.Core.setSpriteDepth(sprite_z);
  }
}
