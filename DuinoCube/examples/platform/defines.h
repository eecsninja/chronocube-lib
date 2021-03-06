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

// Global definitions for platform game.

#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <stdint.h>

enum {
  BG_TILEMAP_INDEX,
  MOON_TILEMAP_INDEX,
  LEVEL_TILEMAP_INDEX,
};

enum {
  BG_PALETTE_INDEX,
  SPRITE_PALETTE_INDEX,
};

// Used to compute the number of elements in a static array.
// TODO: Add this to the DuinoCube library.
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// TODO: Add this to the DuinoCube library.
#define DEFAULT_EMPTY_TILE_VALUE     0x1fff
#define DEFAULT_COLOR_KEY              0x00

// Sprite Z-depth. This sets it above all tile layers except for the clipping
// layer.
#define SPRITE_Z_DEPTH                    3

// TODO: Add this to DuinoCube library.
#define SPRITE_FLIP_MASK  \
    ((1 << SPRITE_FLIP_X) | (1 << SPRITE_FLIP_Y) | (1 << SPRITE_FLIP_XY))

// Tile dimensions in pixels.
#define TILE_WIDTH            16
#define TILE_HEIGHT           16
#define TILE_SIZE      (TILE_WIDTH * TILE_HEIGHT)

// Map dimensions in tiles.
#define TILEMAP_WIDTH              32
#define TILEMAP_HEIGHT             32
#define TILEMAP_ENTRY_SIZE       sizeof(uint16_t)

// Map dimensions in pixels.
#define MAP_WIDTH           (TILEMAP_WIDTH * TILE_WIDTH)
#define MAP_HEIGHT          (TILEMAP_WIDTH * TILE_HEIGHT)

// Screen dimensions.
// TODO: these should be read from DuinoCube registers.
#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       240

// Level dimensions in tiles.  This could be different from the size of the
// tilemap.
#define LEVEL_WIDTH          64
#define LEVEL_HEIGHT         32
#define LEVEL_SIZE           (LEVEL_WIDTH * LEVEL_HEIGHT * TILEMAP_ENTRY_SIZE)

// Level dimensions in pixels.
#define LEVEL_PIXEL_WIDTH    (LEVEL_WIDTH * TILE_WIDTH)
#define LEVEL_PIXEL_HEIGHT   (LEVEL_HEIGHT * TILE_HEIGHT)

// Bat sprite dimensions.
#define BAT_WIDTH            32
#define BAT_HEIGHT           32
#define BAT_SPRITE_SIZE     (BAT_WIDTH * BAT_HEIGHT)

// Chick sprite dimensions.
#define CHICK_WIDTH          48
#define CHICK_HEIGHT         48
#define CHICK_SPRITE_SIZE   (CHICK_WIDTH * CHICK_HEIGHT)
#define NUM_CHICK_SUBFRAMES   4

// Animation periods.
#define BAT_FRAME_PERIOD     12
#define PLAYER_FRAME_PERIOD   4

// Player movement parameters.
#define PLAYER_ACCEL_X        1
#define PLAYER_ACCEL_Y        1
#define PLAYER_MAX_VX         4
#define PLAYER_MAX_VY         4
#define PLAYER_JUMP_ACCEL    10
#define PLAYER_GRAVITY        1

// Scrolling parameters.
#define BG_X_SCROLL_FACTOR       2
#define BG_Y_SCROLL_FACTOR       8
#define MOON_X_SCROLL_FACTOR     1
#define MOON_Y_SCROLL_FACTOR    16

// Bat spawn parameters.
#define NUM_BATS                10
#define BAT_SPAWN_X_MIN          TILEMAP_WIDTH / 2     // In tile coordinates.
#define BAT_SPAWN_X_MAX          (TILEMAP_WIDTH - 1)
#define BAT_SPAWN_Y_MIN          1
#define BAT_SPAWN_Y_MAX         10
#define BAT_SPAWN_VX_MIN         1
#define BAT_SPAWN_VX_MAX         3
#define BAT_SPAWN_VY_MIN         0
#define BAT_SPAWN_VY_MAX         2

#endif  // __DEFINES_H__
