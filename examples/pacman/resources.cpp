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

// Contains function to load resource files.

#include "resources.h"

#include <stdio.h>

#include <DuinoCube.h>

#include "defines.h"

// VRAM offsets of background and sprite image data.
uint16_t g_bg_offset;
uint16_t g_sprite_offset;

// Resources to load:
// 1. BG tilemap.
// 2. BG tileset.
// 3. BG palette.
// 4. Sprites.
// 5. Sprite palette.

struct File {
  const char* filename;
  uint16_t* vram_offset;  // For image data, compute and store VRAM offset here.
  uint16_t addr;          // For other data, store data here.
  uint16_t bank;          // Bank to use for |addr|.
  uint16_t max_size;      // Size checking to avoid overflow.
};

namespace {

const char kFilePath[] = "pacman";    // Base path of data files.

const File kFiles[] = {
  { "bg.lay", NULL, TILEMAP(BG_TILEMAP_INDEX), TILEMAP_BANK, TILEMAP_SIZE },
  { "dots.lay", NULL, TILEMAP(DOTS_TILEMAP_INDEX), TILEMAP_BANK, TILEMAP_SIZE },
  { "tileset.raw", &g_bg_offset, 0, 0, VRAM_BANK_SIZE },
  { "tileset.pal", NULL, PALETTE(BG_PALETTE_INDEX), 0, PALETTE_SIZE },
  { "sprites.raw", &g_sprite_offset, 0, 0, VRAM_BANK_SIZE },
  { "sprites.pal", NULL, PALETTE(SPRITE_PALETTE_INDEX), 0, PALETTE_SIZE },
};

}  // namespace

// Load image, palette, and tilemap data from file system.
void loadResources() {
  uint16_t vram_offset = 0;
  for (int i = 0; i < sizeof(kFiles) / sizeof(kFiles[0]); ++i) {
    const File& file = kFiles[i];

    char filename[256];
    sprintf(filename, "%s/%s", kFilePath, file.filename);

    // Open the file.
    uint16_t handle = DC.File.open(filename, FILE_READ_ONLY);
    if (!handle) {
      printf("Could not open file %s.\n", filename);
      continue;
    }

    uint16_t file_size = DC.File.size(handle);
    printf("File %s is 0x%x bytes\n", filename, file_size);

    if (file_size > file.max_size) {
      printf("File is too big!\n");
      DC.File.close(handle);
      continue;
    }

    // Compute write destination.
    uint16_t dest_addr;
    uint16_t dest_bank;

    if (file.vram_offset) {
      // Set up for VRAM write.

      // If this doesn't fit in the remaining part of the current bank, use the
      // next VRAM bank.
      if (vram_offset % VRAM_BANK_SIZE + file_size > VRAM_BANK_SIZE)
        vram_offset += VRAM_BANK_SIZE - (vram_offset % VRAM_BANK_SIZE);

      // Record VRAM offset.
      *file.vram_offset = vram_offset;

      // Determine the destination VRAM address and bank.
      dest_addr = VRAM_BASE + vram_offset % VRAM_BANK_SIZE;
      dest_bank = vram_offset / VRAM_BANK_SIZE + VRAM_BANK_BEGIN;
      DC.Core.writeWord(REG_SYS_CTRL, (1 << REG_SYS_CTRL_VRAM_ACCESS));

      // Update the VRAM offset.
      vram_offset += file_size;
    } else {
      // Set up for non-VRAM write.
      dest_addr = file.addr;
      dest_bank = file.bank;
    }

    printf("Writing to 0x%x with bank = %d\n", dest_addr, dest_bank);
    DC.Core.writeWord(REG_MEM_BANK, dest_bank);
    DC.File.readToCore(handle, dest_addr, file_size);

    DC.File.close(handle);
  }

  // Allow the graphics pipeline access to VRAM.
  DC.Core.writeWord(REG_SYS_CTRL, (0 << REG_SYS_CTRL_VRAM_ACCESS));
}