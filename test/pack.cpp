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
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "include/pack.h"
#include "include/module.h"
#include "include/battery.h"
#include "include/bms.h"
#include "include/util.h"
#include "settings.h"


BatteryPack::BatteryPack() {}

BatteryPack::BatteryPack(int _id, int CANCSPin, int _contactorInhibitPin, int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule) {
    extern mutex_t canMutex;
    id = _id;

    printf("[pack%d] Initialising pack\n", id);

    numModules = _numModules;
    numCellsPerModule = _numCellsPerModule;
    numTemperatureSensorsPerModule = _numTemperatureSensorsPerModule;
    contactorInhibitPin = _contactorInhibitPin;

    // Initialise modules
    for ( int m = 0; m < numModules; m++ ) {
        modules[m] = BatteryModule(m, this, numCellsPerModule, numTemperatureSensorsPerModule);
    }

    // Set up CAN port
    printf("[pack%d] creating CAN port (cs:%d, miso:%d, mosi:%d, clk:%d)\n", id, CANCSPin, SPI_MISO, SPI_MOSI, SPI_CLK);
    mutex_enter_timeout_ms(&canMutex, 10000);
    CAN = new MCP2515(spi0, CANCSPin, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
    printf("[pack%d] memory address of CAN port : %p\n", id, CAN);
    MCP2515::ERROR response;
    response = CAN->reset();
    if ( response != MCP2515::ERROR_OK ) {
        printf("[pack%d] WARNING problem resetting battery CAN port : %d\n", id, response);
    }
    response = CAN->setBitrate(CAN_500KBPS, MCP_8MHZ);
    if ( response != MCP2515::ERROR_OK ) {
        printf("[pack%d] WARNING problem setting bitrate on battery CAN port : %d\n", id, response);
    }
    response = CAN->setNormalMode();
    if ( response != MCP2515::ERROR_OK ) {
        printf("[pack%d] WARNING problem setting normal mode on battery CAN port : %d\n", id, response);
    }
    mutex_exit(&canMutex);

    can_frame testFrame;
    testFrame.can_id = 0x000;
    testFrame.can_dlc = 8;
    for ( int i = 0; i < 8; i++ ) {
        testFrame.data[i] = 0;
    }
    printf("[pack%d] sending test message from battery pack\n", id);
    if ( !send_frame(&testFrame) ) {
        printf("[pack%d] ERROR sending test message from battery pack\n", id);
    }

    set_inhibit(gpio_get(contactorInhibitPin));

    printf("[pack%d] pack setup complete\n", id);
}

void BatteryPack::print() {
    for ( int m = 0; m < numModules; m++ ) {
        printf("  Module %d\n", m);
        modules[m].print();
    }
}

bool BatteryPack::send_frame(can_frame* frame) {
    extern mutex_t canMutex;

    // printf("[pack%d][send_frame] 0x%03X : ", id, frame->can_id);
    // for ( int i = 0; i < frame->can_dlc; i++ ) {
    //     printf("%02X ", frame->data[i]);
    // }
    // printf("\n");

    // Try to get the mutex. If we can't, we'll try again next time.
    if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
        printf("Error getting CAN mutex\n");
        return false;
    }
    // Send the message
    MCP2515::ERROR result;
    for ( int i = 0; i < SEND_FRAME_RETRIES; i++ ) {
        result = CAN->sendMessage(frame);
        if ( result == MCP2515::ERROR_OK ) {
            break;
        }
    }
    mutex_exit(&canMutex);
    // Return sending result
    return ( result == MCP2515::ERROR_OK );
}

// Set specific cell voltage
void BatteryPack::set_all_cell_voltages(uint16_t newVoltage) {
    //printf("Set voltages for pack : %d\n", id);
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
    if ( !send_frame(&voltage_frame) ) {
        printf("[pack%d] ERROR sending voltage frame for pack %d, module %d, cells 0, 1, and 2\n", id, id, moduleId);
    }

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
    if ( !send_frame(&voltage_frame) ) {
        printf("[pack%d] ERROR sending voltage frame for pack %d, module %d, cells 3, 4, and 5\n", id, id, moduleId);
    }

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
    if ( !send_frame(&voltage_frame) ) {
        printf("[pack%d] ERROR sending voltage frame for pack %d, module %d, cells 6, 7, and 8\n", id, id, moduleId);
    }

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
    if ( !send_frame(&voltage_frame) ) {
        printf("[pack%d] ERROR sending voltage frame for pack %d, module %d, cells 9, 10, and 11\n", id, id, moduleId);
    }

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
    if ( !send_frame(&voltage_frame) ) {
        printf("[pack%d] ERROR sending voltage frame for pack %d, module %d, cells 12, 13, and 14\n", id, id, moduleId);
    }

    voltage_frame.can_id = 0x170 | moduleId;
    // cell 15
    voltage_frame.data[0] = modules[moduleId].get_cell_voltage(15) & 0xFF;
    voltage_frame.data[1] = modules[moduleId].get_cell_voltage(15) >> 8;
    voltage_frame.data[2] = 0x00;
    voltage_frame.data[3] = 0x00;
    voltage_frame.data[4] = 0x00;
    voltage_frame.data[5] = 0x00;
    if ( !send_frame(&voltage_frame) ) {
        printf("[pack%d] ERROR sending voltage frame for pack %d, module %d, cell 15\n", id, id, moduleId);
    }
}

void BatteryPack::send_module_temperatures(uint8_t moduleId) {
    can_frame temperature_frame;
    temperature_frame.can_dlc = 8;
    temperature_frame.can_id = (0x180 | moduleId);
    temperature_frame.data[0] = modules[moduleId].get_cell_temperature(0) + 40;
    temperature_frame.data[1] = modules[moduleId].get_cell_temperature(1) + 40;
    temperature_frame.data[2] = modules[moduleId].get_cell_temperature(2) + 40;
    temperature_frame.data[3] = modules[moduleId].get_cell_temperature(3) + 40;
    send_frame(&temperature_frame);
}

// Check for message from BMS for battery

void BatteryPack::read_frame() {
    //printf("[pack%d][read_frame] Reading messages from battery pack\n", this->id);
    extern mutex_t canMutex;
    can_frame frame;

    //printf("[pack%d][read_frame] Acquiring CAN mutex\n", this->id);
    if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
        printf("[pack%d][read_frame] WARNING could not acquire CAN mutex within timeout\n", this->id);
        return;
    }

    //printf("[pack%d][read_frame] debug : %p\n", this->CAN);

    //printf("[pack%d][read_frame] Reading message from pack\n", this->id);
    MCP2515::ERROR result = CAN->readMessage(&frame);
    mutex_exit(&canMutex);

    if ( result == MCP2515::ERROR_FAIL ) {
        printf("[pack%d][read_frame] ERROR Failed to read message from pack\n", this->id);
        return;
    }

    if ( result == MCP2515::ERROR_OK ) {
        //printf("[pack%d][read_frame] Received CAN frame 0x%X03\n", this->id, frame.can_id);

        // printf("[pack%d][read_frame] 0x%03X : ", this->id, frame.can_id);
        // for ( int i = 0; i < frame.can_dlc; i++ ) {
        //     printf("%02X ", frame.data[i]);
        // }
        // printf("\n");

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

        Bms* bms = battery->get_bms();

        // BMS dis/charge limits message
        if ( frame.can_id == 0x351 ) {
            bms->set_maxVoltage((frame.data[0] + (frame.data[1] << 8)) / 10);
            bms->set_maxChargeCurrent((frame.data[2] + (frame.data[3] << 8)) / 10);
            bms->set_maxDischargeCurrent((frame.data[4] + (frame.data[5] << 8)) / 10);
            bms->set_minVoltage((frame.data[6] + (frame.data[7] << 8)) / 10);
        }

        // BMS state message
        if ( frame.can_id == 0x352 ) {
            //printf("Received CAN fram 0x352 : %d\n", static_cast<BmsState>(frame.data[0]));
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

// bool BatteryPack::send_message(can_frame *frame) {
//     std::lock_guard<std::mutex> lock(canMutex);
//     return CAN.sendMessage(frame) == MCP2515::ERROR_OK;
// }

bool BatteryPack::get_inhibit() {
    return contactorsAreInhibited;
}

void BatteryPack::set_inhibit(bool inhibit) {
    contactorsAreInhibited = inhibit;
}

void BatteryPack::set_all_temperatures(int8_t newTemperature) {
    for ( int m = 0; m < numModules; m++ ) {
        modules[m].set_all_temperatures(newTemperature);
    }
}