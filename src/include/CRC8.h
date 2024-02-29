/*
 * This file is part of the ev mustang bms project.
 *
 * Copyright (C) 2022 Christian Kelly <chrskly@chrskly.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// This code was directly borrowed from the SimpBMS project over here :
//   https://github.com/Tom-evnut/BMWPhevBMS.git


#ifndef BMS_SRC_INCLUDE_CRC8_H_
#define BMS_SRC_INCLUDE_CRC8_H_

#include <stdint.h>

typedef uint8_t crc;
#define POLYNOMIAL 0x1D
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))

class CRC8 {
 public:
    CRC8();
    void begin();
    crc get_crc8(uint8_t const message[], int nBytes, uint8_t final);
 private:
    uint8_t crcTable[256];
};

#endif  // BMS_SRC_INCLUDE_CRC8_H_
