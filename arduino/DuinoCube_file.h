// Copyright (C) 2013 Simon Que
//
// This file is part of ChronoCube.
//
// ChronoCube is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ChronoCube is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with ChronoCube.  If not, see <http://www.gnu.org/licenses/>.

// DuinoCube File I/O library for Arduino.

#ifndef __DUINOCUBE_FILE_H__
#define __DUINOCUBE_FILE_H__

#include <stdint.h>

class DuinoCubeFile {
 public:
  // File I/O functions.
  static uint16_t open(const char* filename, uint16_t mode);
  static void close(uint16_t handle);
  static uint16_t read(uint16_t handle, uint16_t dst_addr, uint16_t size);
  static uint16_t write(uint16_t handle,
                               uint16_t src_addr, uint16_t size);
};

#endif  // __DUINOCUBE_FILE_H__