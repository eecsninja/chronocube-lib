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

// DuinoCube coprocessor SPI functions.

#include <avr/io.h>

#include "defines.h"
#include "spi.h"

void spi_init(void) {
  DDRB |= (1 << PORTB3) |    // Enable MOSI.
          (1 << PORTB5);     // Enable SCK.
  SPCR = (1 << SPE) |     // Enable SPI.
         (1 << MSTR);     // SPI master mode.

  spi_set_bit_order(SPI_MSB_FIRST);

  // Clock  speed of CPU clock / 2. (Max possible)
  SPCR |= (0 << SPR1) | (0 << SPR0);
  SPSR |= (1 << SPI2X);

  // TODO: Either this bit needs to be cleared in its own subsystem module, or
  // all SPI select bits should be cleared together in one place.
  spi_clear_ss(SELECT_FLASH_BIT);
  DDRC |= (1 << SELECT_FLASH_BIT);
}

void spi_set_bit_order(uint8_t order) {
  switch (order) {
  case SPI_LSB_FIRST:
    SPCR |= (1 << DORD);
    break;
  case SPI_MSB_FIRST:
  default:
    SPCR &= ~(1 << DORD);
    break;
  }
}

uint8_t spi_tx(uint8_t value) {
  SPDR = value;
  while(!(SPSR & (1 << SPIF)));
  return SPDR;
}

void spi_write(const void* data, uint16_t size) {
  const uint8_t* buf = reinterpret_cast<const uint8_t*>(data);
  for (uint16_t i = 0; i < size; ++i) {
    SPDR = *buf++;
    while(!(SPSR & (1 << SPIF)));
  }
}

void spi_read(void* data, uint16_t size) {
  uint8_t* buf = reinterpret_cast<uint8_t*>(data);
  for (uint16_t i = 0; i < size; ++i) {
    SPDR = 0;
    while(!(SPSR & (1 << SPIF)));
    *buf++ = SPDR;
  }
}

void spi_set_ss(uint8_t bit) {
  // The SS pin is active low.
  PORTC &= ~(1 << bit);
}

void spi_clear_ss(uint8_t bit) {
  // The SS pin is active low.
  PORTC |= (1 << bit);
}
