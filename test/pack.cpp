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

#include <stdio.h>

#include "include/pack.h"
#include "include/module.h"

BatteryPack::BatteryPack() {}

BatteryPack::BatteryPack(int _id, int CANCSPin, int _contactorInhibitPin, int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule) {
    id = _id;

    printf("Initialising BatteryPack %d\n", id);

    numModules = _numModules;
    numCellsPerModule = _numCellsPerModule;
    numTemperatureSensorsPerModule = _numTemperatureSensorsPerModule;

    // Initialise modules
    for ( int m = 0; m < numModules; m++ ) {
        modules[m] = BatteryModule(m, this, numCellsPerModule, numTemperatureSensorsPerModule);
    }

    // Set up CAN port
    printf("Creating CAN port (cs:%d, miso:%d, mosi:%d, clk:%d)\n", CANCSPin, SPI_MISO, SPI_MOSI, SPI_CLK);
    MCP2515 CAN(spi0, CANCSPin, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
    MCP2515::ERROR response;
    if ( CAN.reset() != MCP2515::ERROR_OK ) {
        printf("ERROR problem resetting battery CAN port %d\n", id);
    }
    if ( CAN.setBitrate(CAN_500KBPS, MCP_8MHZ) != MCP2515::ERROR_OK ) {
        printf("ERROR problem setting bitrate on battery CAN port %d\n", id);
    }
    if ( CAN.setNormalMode() != MCP2515::ERROR_OK ) {
        printf("ERROR problem setting normal mode on battery CAN port %d\n", id);
    }

    printf("Pack %d setup complete\n", id);
}