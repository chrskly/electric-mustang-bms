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
#include <cstdint>
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "include/bms.h"
#include "include/battery.h"
#include "include/util.h"
#include "mcp2515/mcp2515.h"
//#include "include/shunt.h"
#include "include/util.h"


struct repeating_timer handleMainCANMessageTimer;

bool handle_main_CAN_messages(struct repeating_timer *t) {
    extern Bms bms;
    can_frame m;
    zero_frame(&m);
    if ( bms.read_frame(&m) ){
        //printf("Message received on mainCAN : %d\n", m.can_id);
        //print_frame(&m);
        // If we got a message, handle it
        switch ( m.can_id ) {
            // BMS state message
            case 0x352:
                bms.set_state(m.data[0]);
            case 0x521:
                break;
            default:
                break;
        }
    }
    return true;
}

void enable_handle_main_CAN_messages() {
    add_repeating_timer_ms(10, handle_main_CAN_messages, NULL, &handleMainCANMessageTimer);
}


Bms::Bms() {}

Bms::Bms(Shunt* _shunt) {
    extern mutex_t canMutex;
    shunt = _shunt;

    printf("[bms] setting up main CAN port\n");
    mutex_enter_timeout_ms(&canMutex, 10000);
    CAN = new MCP2515(SPI_PORT, MAIN_CAN_CS, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
    MCP2515::ERROR result = CAN->reset();
    if ( result != MCP2515::ERROR_OK ) {
        printf("[bms] error resetting main CAN port : %d\n", result);
    }
    result = CAN->setBitrate(CAN_500KBPS, MCP_8MHZ);
    if ( result != MCP2515::ERROR_OK ) {
        printf("[bms] WARNING setting bitrate on main CAN port : %d\n", result);
    }
    result = CAN->setNormalMode();
    if ( result != MCP2515::ERROR_OK ) {
        printf("[bms] WARNING setting normal mode on main CAN port : %d\n", result);
    }
    mutex_exit(&canMutex);
    printf("[bms] main CAN port memory address : %p\n", CAN);

    printf("[bms] sending 5 test messages\n");
    for ( int i = 0; i < 5; i++ ) {
        //printf(" * [BMS] Main CAN port status : %d\n", CAN.getStatus());
        can_frame m;
        m.can_id = 0x100 + i;
        m.can_dlc = 8;
        for ( int j = 0; j < 8; j++ ) {
            m.data[j] = j;
        }
        this->send_frame(&m);
    }

    shunt->set_CAN_port(CAN);
    shunt->enable();

    printf("[bms] enabling CAN message handlers\n");
    // main CAN (in)
    add_repeating_timer_ms(10, handle_main_CAN_messages, NULL, &handleMainCANMessageTimer);
}

void Bms::set_state(uint8_t new_state) {
    state = static_cast<BmsState>(new_state);
}

BmsState Bms::get_state() {
    return state;
}

void Bms::set_internalError(bool new_internalError) {
    internalError = new_internalError;
}

void Bms::set_packsImbalanced(bool new_packsImbalanced) {
    packsAreImbalanced = new_packsImbalanced;
}

bool Bms::get_packsImbalanced() {
    return packsAreImbalanced;
}

void Bms::set_inhibitCharge(bool new_inhibitCharge) {
    inhibitCharge = new_inhibitCharge;
}

bool Bms::get_inhibitCharge() {
    return inhibitCharge;
}

void Bms::set_inhibitDrive(bool new_inhibitDrive) {
    inhibitDrive = new_inhibitDrive;
}

bool Bms::get_inhibitDrive() {
    return inhibitDrive;
}

void Bms::set_heaterEnabled(bool new_heaterEnabled) {
    heaterEnabled = new_heaterEnabled;
}

bool Bms::get_heaterEnabled() {
    return heaterEnabled;
}

void Bms::set_ignitionOn(bool new_ignitionOn) {
    ignitionOn = new_ignitionOn;
}

void Bms::set_chargeEnable(bool new_chargeEnable) {
    chargeEnable = new_chargeEnable;
}

void Bms::set_soc(int16_t new_soc) {
    soc = new_soc;
}

int8_t Bms::get_soc() {
    return soc;
}

void Bms::set_voltage(uint16_t new_voltage) {
    voltage = new_voltage;
}

void Bms::set_amps(uint16_t new_amps) {
    amps = new_amps;
}

void Bms::set_temperature(uint16_t new_temperature) {
    temperature = new_temperature;
}

void Bms::set_maxVoltage(uint16_t new_maxVoltage) {
    maxVoltage = new_maxVoltage;
}

void Bms::set_minVoltage(uint16_t new_minVoltage) {
    minVoltage = new_minVoltage;
}

void Bms::set_maxChargeCurrent(int16_t new_maxChargeCurrent) {
    maxChargeCurrent = new_maxChargeCurrent;
}

void Bms::set_maxDischargeCurrent(int16_t new_maxDischargeCurrent) {
    maxDischargeCurrent = new_maxDischargeCurrent;
}

void Bms::set_moduleLiveness(int64_t new_moduleLiveness) {
    moduleLiveness = new_moduleLiveness;
}

Battery* Bms::get_battery() {
    return battery;
}

void Bms::set_battery(Battery* _battery) {
    battery = _battery;
}

// Comms

bool Bms::send_frame(can_frame* frame) {
    extern mutex_t canMutex;
    //printf(" [send_frame] Beginning to send frame\n");
    for ( int t = 0; t < SEND_FRAME_RETRIES; t++ ) {

        // printf("[bms][send_frame] 0x%03X %d [ ", frame->can_id, frame->can_dlc);
        // for ( int i = 0; i < frame->can_dlc; i++ ) {
        //     printf("%02X ", frame->data[i]);
        // }
        // printf("]\n");

        // Try to get the mutex.
        //printf(" [send_frame %d] Trying to get the mutex\n", t);
        if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
            continue;
        }
        // Send the message
        //printf(" [send_frame %d] Sending message\n", t);
        MCP2515::ERROR result = this->CAN->sendMessage(frame);
        //printf(" [send_frame %d] Release mutex\n", t);
        mutex_exit(&canMutex);
        // Sending failed, try again
        if ( result != MCP2515::ERROR_OK ) {

            if ( result == MCP2515::ERROR_FAIL ) {
                printf("[bms][send_frame %d] ERROR_FAIL, try again\n", t);
            } else if ( result == MCP2515::ERROR_ALLTXBUSY ) {
                printf("[bms][send_frame %d] ERROR_ALLTXBUSY, try again\n", t);
            } else if ( result == MCP2515::ERROR_FAILINIT ) {
                printf("[bms][send_frame %d] ERROR_FAILINIT, try again\n", t);
            } else if ( result == MCP2515::ERROR_FAILTX ) {
                printf("[bms][send_frame %d] ERROR_FAILTX, try again\n", t);
            } else if ( result == MCP2515::ERROR_NOMSG ) {
                printf("[bms][send_frame %d] ERROR_NOMSG, try again\n", t);
            }

            //printf(" [send_frame %d] Sending failed, try again\n", t);
            //sleep_ms(1);
            continue;
        }
        //printf("[bms][send_frame %d] Frame sent\n", t);
        return true;
    }
    // Failed to send after all retries
    return false;
}

bool Bms::read_frame(can_frame* frame) {
    extern mutex_t canMutex;
    for ( int t = 0; t < READ_FRAME_RETRIES; t++ ) {    
        if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
            return false;
        }
        MCP2515::ERROR result = this->CAN->readMessage(frame);
        mutex_exit(&canMutex);
        // Reading failed, try again
        if ( result != MCP2515::ERROR_OK ) {
            if ( result == MCP2515::ERROR_FAIL ) {
                printf(" [send_frame %d] ERROR_FAIL, try again\n", t);
            } else if ( result == MCP2515::ERROR_ALLTXBUSY ) {
                printf(" [send_frame %d] ERROR_ALLTXBUSY, try again\n", t);
            } else if ( result == MCP2515::ERROR_FAILINIT ) {
                printf(" [send_frame %d] ERROR_FAILINIT, try again\n", t);
            } else if ( result == MCP2515::ERROR_FAILTX ) {
                printf(" [send_frame %d] ERROR_FAILTX, try again\n", t);
            } else if ( result == MCP2515::ERROR_NOMSG ) {
                //printf(" [send_frame %d] ERROR_NOMSG, try again\n", t);
                return true;
            }
            continue;
        } else {
            // printf("[bms][read_frame] 0x%03X %d [ ", frame->can_id, frame->can_dlc);
            // for ( int i = 0; i < frame->can_dlc; i++ ) {
            //     printf("%02X ", frame->data[i]);
            // }
            // printf("]\n");
            return true;
        }
        return true;
    }
    // Failed to read after all retries
    return false;
}