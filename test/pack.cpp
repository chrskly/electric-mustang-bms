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

// Send voltage and temp data from mock battery

void BatteryPack::send_module_voltages(uint8_t moduleId) {
    can_frame voltage_frame;
    voltage_frame.dlc = 8;
    // Send voltages for this module

    voltage_frame.can_id = 0x020 | moduleId;
    // cell 0
    voltage_frame.data[0] = ( modules[moduleId].get_cell_voltage[0] * 1000 / 256 ) >> 8;
    voltage_frame.data[1] = ( modules[moduleId].get_cell_voltage[0] * 1000 / 256 ) && 0x0F0;
    // cell 1
    voltage_frame.data[2] = ( modules[moduleId].get_cell_voltage[1] * 1000 / 256 ) >> 8;
    voltage_frame.data[3] = ( modules[moduleId].get_cell_voltage[1] * 1000 / 256 ) && 0x0F0;
    // cell 2
    voltage_frame.data[4] = ( modules[moduleId].get_cell_voltage[2] * 1000 / 256 ) >> 8;
    voltage_frame.data[5] = ( modules[moduleId].get_cell_voltage[2] * 1000 / 256 ) && 0x0F0;
    send_message(&can_frame);


    voltage_frame.can_id = 0x030 | moduleId;
    // cell 3
    voltage_frame.data[0] = ( modules[moduleId].get_cell_voltage[3] * 1000 / 256 ) >> 8;
    voltage_frame.data[1] = ( modules[moduleId].get_cell_voltage[3] * 1000 / 256 ) && 0x0F0;
    // cell 4
    voltage_frame.data[2] = ( modules[moduleId].get_cell_voltage[4] * 1000 / 256 ) >> 8;
    voltage_frame.data[3] = ( modules[moduleId].get_cell_voltage[4] * 1000 / 256 ) && 0x0F0;
    // cell 5
    voltage_frame.data[4] = ( modules[moduleId].get_cell_voltage[5] * 1000 / 256 ) >> 8;
    voltage_frame.data[5] = ( modules[moduleId].get_cell_voltage[5] * 1000 / 256 ) && 0x0F0;
    send_message(&can_frame);

    voltage_frame.can_id = 0x040 | moduleId;
    // cell 6
    // cell 7
    // cell 8
    send_message(&can_frame);

    voltage_frame.can_id = 0x050 | moduleId;
    // cell 9
    // cell 10
    // cell 11
    send_message(&can_frame);

    voltage_frame.can_id = 0x060 | moduleId;
    // cell 12
    // cell 13
    // cell 14
    send_message(&can_frame);


    voltage_frame.can_id = 0x070 | moduleId;
    // cell 15
    send_message(&can_frame);
}


// Check for message from BMS for battery

void BatteryPack::read_message() {
    can_frame frame;
    if ( CAN.readMessage(&frame) == MCP2515::ERROR_OK ) {
        // This is a module polling request from the BMS
        if ( (frame.can_id & 0xFF0) == 0x080 ) {
            uint8_t moduleId = (frame.can_id & 0x00F);
            send_module_voltages(moduleId);
        }
    }
}