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

// Contains sprite definitions.

#ifndef __SPRITES_H__
#define __SPRITES_H__

#include <stdint.h>

#include <DuinoCube.h>

// Sprite definitions.
enum SpriteState {
  SPRITE_DEAD,
  SPRITE_ALIVE,
};

enum SpriteDirection {
  SPRITE_UP,
  SPRITE_DOWN,
  SPRITE_LEFT,
  SPRITE_RIGHT,
  NUM_SPRITE_DIRS,
};

// Sprite size values to write to DuinoCube Core.
// TODO: Make these part of the DuinoCube library.
enum {
  SPRITE_WIDTH_8 = (0 << SPRITE_HSIZE_0),
  SPRITE_WIDTH_16 = (1 << SPRITE_HSIZE_0),
  SPRITE_WIDTH_32 = (2 << SPRITE_HSIZE_0),
  SPRITE_WIDTH_64 = (3 << SPRITE_HSIZE_0),
};
enum {
  SPRITE_HEIGHT_8 = (0 << SPRITE_VSIZE_0),
  SPRITE_HEIGHT_16 = (1 << SPRITE_VSIZE_0),
  SPRITE_HEIGHT_32 = (2 << SPRITE_VSIZE_0),
  SPRITE_HEIGHT_64 = (3 << SPRITE_VSIZE_0),
};

// Sprite data structure definition.
struct Sprite {
  uint8_t state;                // Alive or dead.
  uint8_t dir;                  // Direction sprite is facing.
  int16_t x, y;                 // Location in pixels.
  uint8_t w, h;                 // Sprite dimensions.

  uint32_t base_offset;         // Base VRAM offset of sprite's frame images.
  uint16_t size;                // Size of each sprite frame image in bytes.
  uint8_t frame;                // Current frame image.
  uint16_t flip;                // Current frame flip flags.
  uint16_t counter;             // Animation counter.

  Sprite() : frame(0),
             flip(0),
             counter(0) {}

  // Compute the sprite's current VRAM offset.
  inline uint16_t get_offset() const {
    return (base_offset + frame * size) >> VRAM_DATA_OFFSET_SHIFT;
  }
};

// Used for planar copying of parts of frame image.
struct Rect {
  uint8_t x, y;     // Top left coordinates.
  uint8_t w, h;     // Dimensions.
};

#define MAX_NUM_SUBSPRITES      4

// A sprite that is composed of multiple sprite objects on-screen.
struct CompositeSprite {
  int16_t x, y;             // Location of main sprite.
  uint8_t w, h;             // Sprite dimensions.
  int16_t vx, vy;           // Velocity.
  uint8_t num_subsprites;   // Component count.
  uint8_t dir;              // Direction.
  uint8_t motion;           // Player motion type.

  uint8_t frame_counter;    // Counts the number of cycles in the current frame.
  uint8_t frame_index;      // Points to a frame of current animation sequence.
  uint8_t sprite_index;     // Points to the sprite frame indicated by
                            // |frame_index| in the current animation sequence.

  // Component sprites and locations/dimensions.
  Sprite* subsprites;
  const Rect* rects;
};

// Move and animate sprite.
void updateSprite(Sprite* sprite_ptr);

// Initialize sprite objects in DuinoCube Core.
void setupSprites(const Sprite* sprites, int num_sprites);

// Update sprite animation based on the current animation counter value.
// |frames| is an array of frame indexes |num_frames| long.
void animateSprite(Sprite* sprite_ptr, const uint8_t* frames,
                   uint8_t num_frames, uint8_t frame_period);

// Updates the location and orientation of the subsprites of a composite sprite.
void updateCompositeSprite(CompositeSprite* sprite);

#endif  // __SPRITES_H__
