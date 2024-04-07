/*
 * This file is part of the ev mustang bms project.
 *
 * Copyright (C) 2024 Christian Kelly <chrskly@chrskly.com>
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

#include "include/CRC8.h"

CRC8::CRC8(void) { }

void CRC8::begin(void) {
    crc remainder;
    for (int dividend = 0; dividend < 256; ++dividend) {
        remainder = dividend << (WIDTH - 8);

        for (uint8_t bit = 8; bit > 0; --bit) {
            if (remainder & TOPBIT) {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
        crcTable[dividend] = remainder;
    }
}


crc CRC8::get_crc8(uint8_t const message[], int nBytes, uint8_t final) {
    uint8_t data;
    crc remainder = 0xFF;

    for (int byte = 0; byte < nBytes; ++byte) {
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    remainder = remainder^final;

    return (remainder);
}
