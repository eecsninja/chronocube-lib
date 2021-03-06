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

// DuinoCube RPC subsystem library for Arduino.

#include "rpc.h"

#include <Arduino.h>
#include <SPI.h>

#include "core.h"
#include "pins.h"

#define NUM_RESET_CYCLES     4   // Atmega 328 requires 2.5 us reset pulse.
                                 // At 16 MHz with F = F_osc / 2, that's 2.5
                                 // SPI cycles.

extern SPIClass SPI;

namespace DuinoCube {

// For accessing memory.
static Mem mem;

void RPC::begin() {
  SET_PIN(RPC_CLIENT_COMMAND_DIR, OUTPUT);
  writeCommand(RPC_CMD_NONE);

  // Reset the RPC server using SPI cycles for timing.
  SET_PIN(RPC_RESET_DIR, OUTPUT);
  SET_PIN(RPC_RESET_PIN, LOW);
  for (uint8_t i = 0; i < NUM_RESET_CYCLES; ++i)
    SPI.transfer(0);
  SET_PIN(RPC_RESET_DIR, INPUT);
}

uint16_t RPC::hello(uint16_t buf_addr) {
  RPC_HelloArgs args;
  args.in.buf_addr = buf_addr;
  uint16_t status = exec(RPC_CMD_HELLO, &args.in, sizeof(args.in), NULL, 0);

  return status;
}

uint16_t RPC::invert(uint16_t buf_addr, uint16_t size) {
  RPC_InvertArgs args;
  args.in.buf_addr = buf_addr;
  args.in.size     = size;
  uint16_t status = exec(RPC_CMD_INVERT, &args.in, sizeof(args.in), NULL, 0);

  return status;
}

uint16_t RPC::readCoreID() {
  RPC_ReadCoreIDArgs args;
  uint16_t status = exec(RPC_CMD_READ_CORE_ID,
                         NULL, 0, &args.out, sizeof(args.out));
  return args.out.id;
}

void RPC::setCommandStatus(uint8_t status) {
  switch (status) {
  case RPC_CLIENT_COMMAND:
    SET_PIN(RPC_CLIENT_COMMAND_PIN, LOW);
    break;
  case RPC_CLIENT_NO_COMMAND:
    SET_PIN(RPC_CLIENT_COMMAND_PIN, HIGH);
    break;
  }
}

void RPC::writeCommand(uint8_t command) {
  if (command == RPC_CMD_NONE) {
    setCommandStatus(RPC_CLIENT_NO_COMMAND);
  } else {
    mem.write(RPC_COMMAND_ADDR, &command, sizeof(command));
    setCommandStatus(RPC_CLIENT_COMMAND);
  }
}

uint8_t RPC::readServerStatus() {
  switch (GET_PIN(RPC_SERVER_STATUS_PIN)) {
  case HIGH:
    return RPC_SERVER_BUSY;
  case LOW:
    return RPC_SERVER_IDLE;
  }
  return RPC_SERVER_IDLE;
}

void RPC::waitForServerStatus(uint8_t status) {
  while (readServerStatus() != status);
}

uint16_t RPC::exec(uint8_t command,
                            const void* in_args, uint8_t in_size,
                            void* out_args, uint8_t out_size) {
  // Wait for the server to be ready.
  // TODO: add a timeout mechanism or fail immediately if not ready?
  waitForServerStatus(RPC_SERVER_IDLE);

  // Write the command input args to memory.
  if (in_args && in_size > 0)
    mem.write(RPC_INPUT_ARG_ADDR, in_args, in_size);

  // Issue the command and wait for acknowledgment.
  writeCommand(command);
  waitForServerStatus(RPC_SERVER_BUSY);

  // Clear the command status register.
  writeCommand(RPC_CMD_NONE);

  // Now wait for the RPC to complete.
  waitForServerStatus(RPC_SERVER_IDLE);

  if (out_args && out_size > 0)
    mem.read(RPC_OUTPUT_ARG_ADDR, out_args, out_size);

  // TODO: implement status codes.
  return 0;
}

}  // namespace DuinoCube
