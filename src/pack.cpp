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
#include "hardware/gpio.h"
#include "include/pack.h"
#include "include/module.h"
#include "mcp2515/mcp2515.h"
#include "settings.h"
#include "include/statemachine.h"
#include "include/bms.h"

#include "settings.h"


BatteryPack::BatteryPack() {}

BatteryPack::BatteryPack(int _id, int CANCSPin, int _contactorInhibitPin, int _contactorFeedbackPin,
        int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule, mutex_t* _canMutex, Bms* _bms) {

    id = _id;
    numModules = _numModules;
    numCellsPerModule = _numCellsPerModule;
    numTemperatureSensorsPerModule = _numTemperatureSensorsPerModule;
    canMutex = _canMutex;
    bms = _bms;

    // Initialise modules
    for ( int m = 0; m < numModules; m++ ) {
        modules[m] = BatteryModule(m, this, numCellsPerModule, numTemperatureSensorsPerModule);
    }

    // Set up dedicated CAN port for communicating with this pack
    printf("[pack%d] creating CAN port\n", id);
    //mutex_enter_timeout_ms(canMutex, 3000);
    CAN = new MCP2515(spi0, CANCSPin, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
    printf("[pack%d] memory address of CAN port : %p\n", id, CAN);
    MCP2515::ERROR response;
    printf("[pack%d] resetting battery CAN port\n", id);
    response = CAN->reset();
    if ( response != MCP2515::ERROR_OK ) {
        printf("[pack%d] WARNING problem resetting battery CAN port: %d\n", id, response);
    }
    response = CAN->setBitrate(CAN_500KBPS, MCP_8MHZ);
    if ( response != MCP2515::ERROR_OK ) {
        printf("[pack%d] WARNING problem setting bitrate on battery CAN port : %d\n", id, response);
    }
    response = CAN->setNormalMode();
    if ( response != MCP2515::ERROR_OK ) {
        printf("[pack%d] WARNING problem setting normal mode on battery CAN port : %d\n", id, response);
    }
    //mutex_exit(canMutex);
    
    printf("[pack%d] CAN port status : %d\n", id, CAN->getStatus());

    can_frame testFrame;
    testFrame.can_id = 0x000;
    testFrame.can_dlc = 8;
    for ( int i = 0; i < 8; i++ ) {
        testFrame.data[i] = 0;
    }
    printf("[pack%d] sending 10 test messages\n", id);
    for ( int i = 0; i < 10; i++ ) {
        if ( !send_frame(&testFrame) ) {
            printf("[pack%d] ERROR sending test message %d\n", id, i);
        }
    }

    // Set last update time to now
    lastUpdate = get_absolute_time();

    voltage = 0.0000f;
    cellDelta = 0;

    // Set up contactor control.
    contactorInhibitPin = _contactorInhibitPin;
    printf("[pack%d] setting up contactor control\n");
    gpio_init(contactorInhibitPin);
    gpio_set_dir(contactorInhibitPin, GPIO_OUT);
    gpio_put(contactorInhibitPin, 0);

    // Set up contactor feedback
    contactorFeedbackPin = _contactorFeedbackPin;
    gpio_init(NEG_CONTACTOR_FEEDBACK_PIN);
    gpio_set_dir(NEG_CONTACTOR_FEEDBACK_PIN, GPIO_IN);
    gpio_set_irq_enabled(NEG_CONTACTOR_FEEDBACK_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Set next balance time to 10 seconds from now
    nextBalanceTime = delayed_by_us(get_absolute_time(), 10000);

    inStartup = true;
    modulePollingCycle = 0;

    crc8.begin();

    printf("[pack%d] setup complete\n", id);
}

void BatteryPack::print() {
    printf("[pack%d] %3.2fV : Hi %d : Lo %d : %dmV\n", id, (voltage/1000), get_highest_cell_voltage(), get_lowest_cell_voltage(), cellDelta);
    for ( int m = 0; m < numModules; m++ ) {
        modules[m].print();
    }
}

uint8_t BatteryPack::getcheck(can_frame &msg, int id) {
    unsigned char canmes[11];
    int meslen = msg.can_dlc + 1;  // remove one for crc and add two for id bytes
    canmes[1] = msg.can_id;
    canmes[0] = msg.can_id >> 8;

    for (int i = 0; i < (msg.can_dlc - 1); i++) {
        canmes[i + 2] = msg.data[i];
    }
    return (crc8.get_crc8(canmes, meslen, finalxor[id]));
}

int8_t BatteryPack::get_module_liveness(int8_t moduleId) {
    return modules[moduleId].is_alive();
}

/*
 *
 * Contents of message
 *   byte 0 : balance data
 *   byte 1 : balance data
 *   byte 2 : 0x00
 *   byte 3 : 0x00
 *   byte 4 : 
 *   byte 5 : 0x01
 *   byte 6 :
 *   byte 7 : checksum
 */

void BatteryPack::request_data() {
    if ( modulePollingCycle == 0xF ) {
        modulePollingCycle = 0;
    }
    for ( int m = 0; m < numModules; m++ ) {
        pollModuleFrame.can_id = 0x080 | (m);
        pollModuleFrame.can_dlc = 8;
        pollModuleFrame.data[0] = 0xC7;
        pollModuleFrame.data[1] = 0x10;
        pollModuleFrame.data[2] = 0x00;
        pollModuleFrame.data[3] = 0x00;
        if ( inStartup ) {
            pollModuleFrame.data[4] = 0x20;
            pollModuleFrame.data[5] = 0x00;
        } else {
            pollModuleFrame.data[4] = 0x40;
            pollModuleFrame.data[5] = 0x01;
        }
        pollModuleFrame.data[6] = modulePollingCycle << 4;
        if ( inStartup && modulePollingCycle == 2 ) {
            pollModuleFrame.data[6] = pollModuleFrame.data[6] + 0x04;
        }
        pollModuleFrame.data[7] = getcheck(pollModuleFrame, m);
        if ( !send_frame(&pollModuleFrame) ) {
            printf("[pack%d][request_data] ERROR sending poll message to module %d\n", id, m);
        }
    }
    if ( inStartup && modulePollingCycle == 2 ) {
        inStartup = false;
    }
    modulePollingCycle++;
    return;
}

/*
 * Check for message from battery modules, parse as required.
 */
void BatteryPack::read_message() {
    extern mutex_t canMutex;
    can_frame frame;

    // Try to get the mutex. If we can't, we'll try again next time.
    if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
        printf("[pack%d][read_message] failed to get battery pack CAN mutex\n", this->id);
        return;
    }

    // Check for message
    MCP2515::ERROR result = CAN->readMessage(&frame);
    mutex_exit(&canMutex);

    // Return if we don't have a message to process
    if ( result != MCP2515::ERROR_OK ) {
        return;
    }

    // printf("[pack%d][read_message] received message 0x%03X : ", this->id, frame.can_id);
    // for ( int i = 0; i < frame.can_dlc; i++ ) {
    //     printf("%02X ", frame.data[i]);
    // }
    // printf("\n");

    // Temperature messages
    if ( (frame.can_id & 0xFF0) == 0x180 ) {
        decode_temperatures(&frame);
        this->battery->process_temperature_update();
        this->bms->send_event(E_TEMPERATURE_UPDATE);
    }
    // Voltage messages
    if (frame.can_id > 0x99 && frame.can_id < 0x180) {
        decode_voltages(&frame);
        this->battery->process_voltage_update();
        this->bms->send_event(E_CELL_VOLTAGE_UPDATE);
    }
}

bool BatteryPack::send_frame(can_frame *frame) {
    extern mutex_t canMutex;
    for ( int i = 0; i < SEND_FRAME_RETRIES; i++ ) {

        // printf("[pack%d][send_frame] 0x%03X  [ ", this->id, frame->can_id);
        // for ( int i = 0; i < frame->can_dlc; i++ ) {
        //     printf("%02X ", frame->data[i]);
        // }
        // printf("]\n");

        // Try to get the mutex. If we can't, we'll try again next time.
        if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
            printf("[pack%d][send_frame] failed to get battery pack CAN mutex\n", this->id);
            continue;
        }
        
        MCP2515::ERROR result = CAN->sendMessage(frame);
        mutex_exit(&canMutex);

        switch (result) {
            case MCP2515::ERROR_FAIL:
                printf("[pack%d][send_frame] ERROR sending message to battery pack (ERROR_FAIL)\n", this->id);
                //sleep_ms(2);
            case MCP2515::ERROR_ALLTXBUSY:
                printf("[pack%d][send_frame] ERROR sending message to battery pack (ALLTXBUSY)\n", this->id);
                //sleep_ms(2);
            case MCP2515::ERROR_FAILINIT:
                printf("[pack%d][send_frame] ERROR sending message to battery pack (FAILINIT)\n", this->id);
                //sleep_ms(2);
            case MCP2515::ERROR_FAILTX:
                printf("[pack%d][send_frame] ERROR sending message to battery pack (FAILTX)\n", this->id);
                //sleep_ms(2);
        }

        if ( result == MCP2515::ERROR_OK) {
            return true;
        }
    }
    return false;
}

void BatteryPack::set_pack_error_status(int newErrorStatus) {
    errorStatus = newErrorStatus;
}

int BatteryPack::get_pack_error_status() {
    return errorStatus;
}

void BatteryPack::set_pack_balance_status(int newBalanceStatus) {
    balanceStatus = newBalanceStatus;
}

int BatteryPack::get_pack_balance_status() {
    return balanceStatus;
}

// Return true if it's time for the pack to be balanced.
bool BatteryPack::pack_is_due_to_be_balanced() {
    return false;
    return ( absolute_time_diff_us(get_absolute_time(), nextBalanceTime) < 0 );
}

void BatteryPack::reset_balance_timer() {
    nextBalanceTime = delayed_by_us(get_absolute_time(), BALANCE_INTERVAL);
}


//// ----
//
// Voltage
//
//// ----

// Return the voltage of the whole pack
float BatteryPack::get_voltage() {
    return voltage;
}

// Update the pack voltage value by summing all of the cell voltages
void BatteryPack::recalculate_total_voltage() {
    float newVoltage = 0;
    for ( int m = 0; m < numModules; m++ ) {
        newVoltage += modules[m].get_voltage();
    }
    voltage = newVoltage;
}

// Return the voltage of the lowest cell in the pack
uint16_t BatteryPack::get_lowest_cell_voltage() {
    uint16_t lowestCellVoltage = 10000;
    for ( int m = 0; m < numModules; m++ ) {
        // skip modules with incomplete cell data
        if ( !modules[m].all_module_data_populated() ) {
            continue;
        }
        if ( modules[m].get_lowest_cell_voltage() < lowestCellVoltage ) {
            lowestCellVoltage = modules[m].get_lowest_cell_voltage();
        }
    }
    return lowestCellVoltage;
}

// Return true if any cell in the pack is under min voltage
bool BatteryPack::has_empty_cell() {
    for ( int m = 0; m < numModules; m++ ) {
        if ( modules[m].has_empty_cell() ) {
            return true;
        }
    }
    return false;
}

// Return the voltage of the highest cell in the pack
uint16_t BatteryPack::get_highest_cell_voltage() {
    uint16_t highestCellVoltage = 0;
    for ( int m = 0; m < numModules; m++ ) {
        // skip modules with incomplete cell data
        if ( !modules[m].all_module_data_populated() ) {
            continue;
        }
        if ( modules[m].get_highest_cell_voltage() > highestCellVoltage ) {
            highestCellVoltage = modules[m].get_highest_cell_voltage();
        }
    }
    return highestCellVoltage;
}

// Return true if any cell in the pack is over max voltage
bool BatteryPack::has_full_cell() {
    for ( int m = 0; m < numModules; m++ ) {
        if ( modules[m].has_full_cell() ) {
            return true;
        }
    }
    return false;
}

// Update the value for the voltage of an individual cell in a pack
void BatteryPack::set_cell_voltage(int moduleId, int cellIndex, uint32_t newCellVoltage) {
    modules[moduleId].set_cell_voltage(cellIndex, newCellVoltage);
}

// Extract voltage readings from CAN message and update stored values
void BatteryPack::decode_voltages(can_frame *frame) {
    int messageId = (frame->can_id & 0x0F0);
    int moduleId = (frame->can_id & 0x00F);

    switch (messageId) {
        case 0x000:
            // error = msg.buf[0] + (msg.buf[1] << 8) + (msg.buf[2] << 16) + (msg.buf[3] << 24);
            set_pack_error_status(frame->data[0] + (frame->data[1] << 8) + (frame->data[2] << 16) + (frame->data[3] << 24));
            // balstat = (frame.data[5] << 8) + frame.data[4];
            set_pack_balance_status((frame->data[5] << 8) + frame->data[4]);
            break;
        case 0x020:
            modules[moduleId].set_cell_voltage(0, static_cast<uint16_t>(frame->data[0] + (frame->data[1] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(1, static_cast<uint16_t>(frame->data[2] + (frame->data[3] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(2, static_cast<uint16_t>(frame->data[4] + (frame->data[5] & 0x3F) * 256));
            break;
        case 0x030:
            modules[moduleId].set_cell_voltage(3, static_cast<uint16_t>(frame->data[0] + (frame->data[1] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(4, static_cast<uint16_t>(frame->data[2] + (frame->data[3] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(5, static_cast<uint16_t>(frame->data[4] + (frame->data[5] & 0x3F) * 256));
            break;
        case 0x040:
            modules[moduleId].set_cell_voltage(6, static_cast<uint16_t>(frame->data[0] + (frame->data[1] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(7, static_cast<uint16_t>(frame->data[2] + (frame->data[3] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(8, static_cast<uint16_t>(frame->data[4] + (frame->data[5] & 0x3F) * 256));
            break;
        case 0x050:
            modules[moduleId].set_cell_voltage(9, static_cast<uint16_t>(frame->data[0] + (frame->data[1] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(10, static_cast<uint16_t>(frame->data[2] + (frame->data[3] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(11, static_cast<uint16_t>(frame->data[4] + (frame->data[5] & 0x3F) * 256));
            break;
        case 0x060:
            modules[moduleId].set_cell_voltage(12, static_cast<uint16_t>(frame->data[0] + (frame->data[1] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(13, static_cast<uint16_t>(frame->data[2] + (frame->data[3] & 0x3F) * 256));
            modules[moduleId].set_cell_voltage(14, static_cast<uint16_t>(frame->data[4] + (frame->data[5] & 0x3F) * 256));
            break;
        case 0x070:
            modules[moduleId].set_cell_voltage(15, static_cast<uint16_t>(frame->data[0] + (frame->data[1] & 0x3F) * 256));
            break;
        default:
            break;
    }

    // Check if this update has left us with a complete set of voltage/temperature readings
    if ( !modules[moduleId].all_module_data_populated() ) {
        modules[moduleId].check_if_module_data_is_populated();
    }

    modules[moduleId].heartbeat();
}

// Update the cellDelta
void BatteryPack::recalculate_cell_delta() {
    cellDelta = get_highest_cell_voltage() - get_lowest_cell_voltage();
}

/*
 * We have new cell voltage data. Recalculate.
 */
void BatteryPack::process_voltage_update() {
    recalculate_total_voltage();
    recalculate_cell_delta();
}


//// ----
//
// Temperature
//
//// ----

// Return true if any cell in the pack is over max temperature
bool BatteryPack::has_temperature_sensor_over_max() {
    for ( int m = 0; m < numModules; m++ ) {
        if ( modules[m].has_temperature_sensor_over_max() ) {
            return true;
        }
    }
    return false;
}

// return the temperature of the lowest sensor in the pack
int8_t BatteryPack::get_lowest_temperature() {
    int8_t lowestModuleTemperature = 126;
    for ( int m = 0; m < numModules; m++ ) {
        if ( ! modules[m].all_module_data_populated() ) {
            continue;
        }
        if ( modules[m].get_lowest_temperature() < lowestModuleTemperature ) {
            lowestModuleTemperature = modules[m].get_lowest_temperature();
        }
    }
    return lowestModuleTemperature;
}

// return the temperature of the highest sensor in the pack
int8_t BatteryPack::get_highest_temperature() {
    int8_t highestModuleTemperature = -126;
    for ( int m = 0; m < numModules; m++ ) {
        if ( ! modules[m].all_module_data_populated() ) {
            continue;
        }
        if ( modules[m].get_highest_temperature() > highestModuleTemperature ) {
            highestModuleTemperature = modules[m].get_highest_temperature();
        }
    }
    return highestModuleTemperature;
}

// Extract temperature sensor readings from CAN frame and update stored values
void BatteryPack::decode_temperatures(can_frame *temperatureMessageFrame) {
    int moduleId = (temperatureMessageFrame->can_id & 0x00F);
    modules[moduleId].heartbeat();
    for ( int t = 0; t < numTemperatureSensorsPerModule; t++ ) {
        float temperature = temperatureMessageFrame->data[t] - 40;
        modules[moduleId].update_temperature(t, temperature);
    }
}

void BatteryPack::process_temperature_update() {
    if ( ( get_clock() - lastTemperatureSampleTime ) > PACK_TEMP_SAMPLE_INTERVAL ) {
        lastTemperatureSampleTime = get_clock();
        temperatureDelta = get_highest_temperature() - lastTemperatureSample;
        lastTemperatureSample = get_highest_temperature();
    }
}


//// ----
//
// Contactors
//
//// ----

// Prevent the contactors for this pack from closing
void BatteryPack::enable_inhibit_contactor_close() {
    if ( !contactors_are_inhibited() ) {
        printf("[pack%d][enable_inhibit_contactor_close] Enabling inhibit of contactor close for pack\n", id);
        gpio_put(INHIBIT_CONTACTOR_PINS[id], 1);
    }
}

// Allow the contactors for this pack to close
void BatteryPack::disable_inhibit_contactor_close() {
    if ( contactors_are_inhibited() ) {
        printf("[pack%d][disable_inhibit_contactor_close] Disabling inhibit of contactor close for pack\n", id);
        gpio_put(INHIBIT_CONTACTOR_PINS[id], 0);
    }
}

// Return true if the contactors for this pack are currently not allowed to close
bool BatteryPack::contactors_are_inhibited() {
    return gpio_get(INHIBIT_CONTACTOR_PINS[id]);
}

bool BatteryPack::contactors_are_welded() {
    return gpio_get(contactorFeedbackPin);
}



// Current

int16_t BatteryPack::get_max_discharge_current() {
    return 0;
}

/* Returns the maximum charge current as a function of battery temperature. */
int16_t BatteryPack::get_max_charge_current() {
    // Safety checks first
    if ( has_full_cell() ) {
        return 0;
    }
    if ( has_temperature_sensor_over_max() ) {
        return 0;
    }
    if ( get_lowest_temperature() < CHARGE_TEMPERATURE_MINIMUM) {
        return 0;
    }

    /* Allow predefined max current when the temperature is below
     * CHARGE_TEMPERATURE_DERATING_MINIMUM (15°C) */
    if ( get_highest_temperature() < CHARGE_TEMPERATURE_DERATING_MINIMUM ) {
        return chargeCurrentMax[static_cast<int>(get_highest_temperature() + 10)];
    }

    /* When battery temp is over CHARGE_TEMPERATURE_DERATING_MINIMUM (15°C), 
     * allow CHARGE_TEMPERATURE_DERATING_THRESHOLD (1°) of temperature increase
     * per minute. Scale back charge current by 10% for every degree over that.*/
    else {
        if ( temperatureDelta < CHARGE_TEMPERATURE_DERATING_THRESHOLD ) {
            return chargeCurrentMax[static_cast<int>(get_highest_temperature() + 10)];
        } else {
            if ( (temperatureDelta - CHARGE_TEMPERATURE_DERATING_THRESHOLD) >= 10 ) {
                return 0;
            } else {
                float derateScaleFactor = (10 - temperatureDelta - CHARGE_TEMPERATURE_DERATING_THRESHOLD) / 100;
                return chargeCurrentMax[static_cast<int>(get_highest_temperature() + 10)] * derateScaleFactor;
            }
        }
    }

    return 0;
}