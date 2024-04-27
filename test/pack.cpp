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
#include "include/battery.h"

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


// Set specific cell voltage
void BatteryPack::set_all_cell_voltages(uint16_t newVoltage) {
    for ( int m = 0; m < numModules; m++ ) {
        modules[m].set_all_cell_voltages(newVoltage);
    }
}


// Send voltage and temp data from mock battery

void BatteryPack::send_module_voltages(uint8_t moduleId) {
    can_frame voltage_frame;
    voltage_frame.can_dlc = 8;
    // Send voltages for this module

    voltage_frame.can_id = 0x120 | moduleId;
    // cell 0
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(0) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(0) >> 8;
    // cell 1
    voltage_frame.data[2] = modules[moduleId].get_cell_voltage(1) & 0xFF;
    voltage_frame.data[3] = modules[moduleId].get_cell_voltage(1) >> 8;
    // cell 2
    voltage_frame.data[4] = modules[moduleId].get_cell_voltage(2) & 0xFF;
    voltage_frame.data[5] = modules[moduleId].get_cell_voltage(2) >> 8;
    send_message(&voltage_frame);


    voltage_frame.can_id = 0x130 | moduleId;
    // cell 3
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(3) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(3) >> 8;
    // cell 4
    voltage_frame.data[2] = modules[moduleId].get_cell_voltage(4) & 0xFF;
    voltage_frame.data[3] = modules[moduleId].get_cell_voltage(4) >> 8;
    // cell 5
    voltage_frame.data[4] = modules[moduleId].get_cell_voltage(5) & 0xFF;
    voltage_frame.data[5] = modules[moduleId].get_cell_voltage(5) >> 8;
    send_message(&voltage_frame);

    voltage_frame.can_id = 0x140 | moduleId;
    // cell 6
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(6) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(6) >> 8;
    // cell 7
    voltage_frame.data[2] = modules[moduleId].get_cell_voltage(7) & 0xFF;
    voltage_frame.data[3] = modules[moduleId].get_cell_voltage(7) >> 8;
    // cell 8
    voltage_frame.data[4] = modules[moduleId].get_cell_voltage(8) & 0xFF;
    voltage_frame.data[5] = modules[moduleId].get_cell_voltage(8) >> 8;
    send_message(&voltage_frame);

    voltage_frame.can_id = 0x150 | moduleId;
    // cell 9
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(9) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(9) >> 8;
    // cell 10
    voltage_frame.data[2] = modules[moduleId].get_cell_voltage(10) & 0xFF;
    voltage_frame.data[3] = modules[moduleId].get_cell_voltage(10) >> 8;
    // cell 11
    voltage_frame.data[4] = modules[moduleId].get_cell_voltage(11) & 0xFF;
    voltage_frame.data[5] = modules[moduleId].get_cell_voltage(11) >> 8;
    send_message(&voltage_frame);

    voltage_frame.can_id = 0x160 | moduleId;
    // cell 12
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(12) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(12) >> 8;
    // cell 13
    voltage_frame.data[2] = modules[moduleId].get_cell_voltage(13) & 0xFF;
    voltage_frame.data[3] = modules[moduleId].get_cell_voltage(13) >> 8;
    // cell 14
    voltage_frame.data[4] = modules[moduleId].get_cell_voltage(14) & 0xFF;
    voltage_frame.data[5] = modules[moduleId].get_cell_voltage(14) >> 8;
    send_message(&voltage_frame);

    voltage_frame.can_id = 0x170 | moduleId;
    // cell 15
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(15) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(15) >> 8;
    voltage_frame.data[2] = 0x00;
    voltage_frame.data[3] = 0x00;
    voltage_frame.data[4] = 0x00;
    voltage_frame.data[5] = 0x00;
    send_message(&voltage_frame);
}

void BatteryPack::send_module_temperatures(uint8_t moduleId) {
    can_frame temperature_frame;
    temperature_frame.can_dlc = 8;
    temperature_frame.can_id = (0x180 | moduleId);
    temperature_frame.data[0] = modules[moduleId].get_cell_temperature(0) + 40;
    temperature_frame.data[1] = modules[moduleId].get_cell_temperature(1) + 40;
    temperature_frame.data[2] = modules[moduleId].get_cell_temperature(2) + 40;
    temperature_frame.data[3] = modules[moduleId].get_cell_temperature(3) + 40;
    send_message(&temperature_frame);
}

// Check for message from BMS for battery

void BatteryPack::read_message() {
    can_frame frame;
    Bms* bms = battery->get_bms();
    if ( CAN.readMessage(&frame) == MCP2515::ERROR_OK ) {
        // This is a module polling request from the BMS
        /*if ( (frame.can_id & 0xFF0) == 0x080 ) {
            uint8_t moduleId = (frame.can_id & 0x00F);
            send_module_voltages(moduleId);
        }
        */
        for ( int m=0; m < numModules; m++ ) {
            send_module_voltages(m);
            send_module_temperatures(m);
        }

        // BMS dis/charge limits message
        if ( frame.can_id == 0x351 ) {
            bms->set_maxVoltage((frame.data[0] + (frame.data[1] << 8)) / 10);
            bms->set_maxChargeCurrent((frame.data[2] + (frame.data[3] << 8)) / 10);
            bms->set_maxDischargeCurrent((frame.data[4] + (frame.data[5] << 8)) / 10);
            bms->set_minVoltage((frame.data[6] + (frame.data[7] << 8)) / 10);
        }

        // BMS state message
        if ( frame.can_id == 0x352 ) {
            // State
            bms->set_state(static_cast<BmsState>(frame.data[0]));
            // Error bits
            bms->set_internalError(frame.data[1] & 0x01);
            bms->set_packsImbalanced((frame.data[1] > 1) & 0x02);
            // Status bits
            bms->set_inhibitCharge(frame.data[2] & 0x01);
            bms->set_inhibitDrive((frame.data[2] > 1) & 0x01);
            bms->set_heaterEnabled((frame.data[2] > 2)  & 0x01);
            bms->set_ignitionOn((frame.data[2] > 3) & 0x01);
            bms->set_chargeEnable((frame.data[2] > 4) & 0x01);
        }

        // Module liveness message
        if ( frame.can_id == 0x353 ) {
            bms->set_moduleLiveness(
                ((int64_t)frame.data[0]) | 
                ((int64_t)frame.data[1] << 8) | 
                ((int64_t)frame.data[2] << 16) |
                ((int64_t)frame.data[3] << 24) | 
                ((int64_t)frame.data[4] << 32) |
                ((int64_t)frame.data[5] << 40) |
                ((int64_t)frame.data[6] << 48) |
                ((int64_t)frame.data[7] << 56)
            );
        }

        // SoC message
        if ( frame.can_id == 0x355 ) {
            bms->set_soc(frame.data[0] + (frame.data[1] << 8));
        }

        // Status message
        if ( frame.can_id == 0x356 ) {
            bms->set_voltage((frame.data[0] + (frame.data[1] << 8)) / 100);
            bms->set_amps((frame.data[2] + (frame.data[3] << 8)) / 10);
            bms->set_temperature(frame.data[4] + (frame.data[5] << 8));
        }

        // Alarms message
        if ( frame.can_id == 0x35A ) {
            //
        }
    }
}

// Send CAN message to BMS on private battery network

void BatteryPack::send_message(can_frame *frame) {
    CAN.sendMessage(frame);
}