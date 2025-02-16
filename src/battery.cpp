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
#include <string>
#include "include/battery.h"
#include "include/pack.h"
#include "include/io.h"
#include "include/statemachine.h"
#include "settings.h"


struct repeating_timer pollPackTimer;

// Send request to each pack to ask for a data update
bool poll_packs_for_data(struct repeating_timer *t) {
    extern Battery battery;
    battery.request_data();
    return true;
}

// Handle the CAN messages that come back from the battery modules

struct repeating_timer handleInboundCANMessagesTimer;

bool handle_inbound_CAN_messages(struct repeating_timer *t) {
    extern Battery battery;
    battery.read_message();
    return true;
}


Battery::Battery(Io* _io) {
    voltage = 0;
    lowestCellVoltage = 0;
    highestCellVoltage = 0;
    lowestSensorTemperature = 0;
    highestSensorTemperature = 0;
    numPacks = NUM_PACKS;
    io = _io;
}

// Create all battery packs and modules
void Battery::initialise(Bms* _bms) {

    bms = _bms;

    for ( int p = 0; p < numPacks; p++ ) {
        printf("[battery] Initialising battery pack %d (CS:%d, inh:%d, mod/pack:%d, cell/mod:%d, T/mod:%d)\n", p, CS_PINS[p], INHIBIT_CONTACTOR_PINS[p], MODULES_PER_PACK, CELLS_PER_MODULE, TEMPS_PER_MODULE);
        packs[p] = BatteryPack(p, CS_PINS[p], INHIBIT_CONTACTOR_PINS[p], CONTACTOR_FEEDBACK_PINS[p], MODULES_PER_PACK, CELLS_PER_MODULE, TEMPS_PER_MODULE, canMutex, bms);
        packs[p].set_battery(this);
        printf("[battery] Initialisation of battery pack %d complete\n", p);
    }

    // Precalculate min/max battery voltages
    maximumBatteryVoltage = CELL_FULL_VOLTAGE * CELLS_PER_MODULE * MODULES_PER_PACK;
    minimumBatteryVoltage = CELL_EMPTY_VOLTAGE * CELLS_PER_MODULE * MODULES_PER_PACK;

    // Enable polling of packs for voltage/temperature data
    printf("[battery] Enabling polling of packs for data\n");
    add_repeating_timer_ms(100, poll_packs_for_data, NULL, &pollPackTimer);
    add_repeating_timer_ms(5, handle_inbound_CAN_messages, NULL, &handleInboundCANMessagesTimer);
}

//
int Battery::print() {
    for ( int p = 0; p < numPacks; p++ ) {
        packs[p].print();
    }
    return 0;
}

// Send messages to all packs to request voltage/temperature data
void Battery::request_data() {
    for ( int p = 0; p < numPacks; p++ ) {
        packs[p].request_data();
    }
}

/*
 * Check for and read messages from each pack
 */
void Battery::read_message() {
    for ( int p = 0; p < numPacks; p++ ) {
        packs[p].read_message();
    }
}

//
void Battery::send_test_message() {
    printf("[battery] Sending test messages to all packs\n");
    for ( int p = 0; p < numPacks; p++ ) {
        can_frame fr;
        fr.can_id = 0x000;
        fr.can_dlc = 3;
        fr.data[0] = 0x7E;
        fr.data[1] = 0x57;
        fr.data[2] = p;
        packs[p].send_frame(&fr);
    }
}


//// ----
//
// Voltage
//
//// ----

// Return the voltage of the whole battery
uint32_t Battery::get_voltage() {
    return voltage;
}

// Recompute and store the battery voltage based on current cell voltages
void Battery::recalculate_voltage() {
    uint32_t newVoltage = 0;
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[p].get_voltage() > newVoltage ) {
            newVoltage = packs[p].get_voltage();
        }
    }
    voltage = newVoltage;
}

// Recompute the difference between the highest and lowest cell voltage
void Battery::recalculate_cell_delta() {
    cellDelta = highestCellVoltage - lowestCellVoltage;
}

// Return the maximum allowed voltage of the whole battery
uint32_t Battery::get_max_voltage() {
    return maximumBatteryVoltage;
}

// Return the minimum allowed voltage of the whole battery
uint32_t Battery::get_min_voltage() {
    return minimumBatteryVoltage;
}

// Return the id of the pack that has the highest voltage
int Battery::get_index_of_high_pack() {
    int high_pack_index = 0;
    float high_pack_voltage = 0.0f;
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[p].get_voltage() > high_pack_voltage ) {
            high_pack_index = p;
            high_pack_voltage = packs[p].get_voltage();
        }
    }
    return high_pack_index;
}

// Return the id of the pack that has the lowest voltage
int Battery::get_index_of_low_pack() {
    int low_pack_index = 0;
    uint32_t low_pack_voltage = 1000;
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[0].get_voltage() < low_pack_voltage ) {
            low_pack_index = p;
            low_pack_voltage = packs[p].get_voltage();
        }
    }
    return low_pack_index;
}

/*
 * We have new cell voltage data. Process it.
 */
void Battery::process_voltage_update() {
    // Do processing for each pack
    for ( int p = 0; p < numPacks; p++ ) {
        packs[p].process_voltage_update();
    }
    // Do processing for overall battery
    recalculate_voltage();
    recalculate_cell_delta();
    recalculate_lowest_cell_voltage();
    recalculate_highest_cell_voltage();
    if ( !packs_are_imbalanced() ) {
        this->bms->pack_voltages_match_heartbeat();
    }
}


// Low cells

// Recompute the lowest cell voltage across the whole battery
void Battery::recalculate_lowest_cell_voltage() {
    uint16_t newLowestCellVoltage = 10000;
    for ( int p = 1; p < numPacks; p++ ) {
        if ( packs[p].get_lowest_cell_voltage() < newLowestCellVoltage ) {
            newLowestCellVoltage = packs[p].get_lowest_cell_voltage();
        }
    }
    // Safety checks
    if ( newLowestCellVoltage < CELL_EMPTY_VOLTAGE || newLowestCellVoltage > CELL_FULL_VOLTAGE ) {
        bms->set_internal_error();
    }
    lowestCellVoltage = newLowestCellVoltage;
}

uint16_t Battery::get_lowest_cell_voltage() {
    return lowestCellVoltage;
}

// Return true if any cell in the battery is below the minimum voltage level
bool Battery::has_empty_cell() {
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[p].has_empty_cell() ) {
            return true;
        }
    }
    return false;
}

// High cells

// Recompute the highest cell voltage
void Battery::recalculate_highest_cell_voltage() {
    uint16_t newHighestCellVoltage = 0;
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[p].get_highest_cell_voltage() > newHighestCellVoltage ) {
            newHighestCellVoltage = packs[p].get_highest_cell_voltage();
        }
    }
    // Safety checks
    if ( newHighestCellVoltage < CELL_EMPTY_VOLTAGE || newHighestCellVoltage > CELL_FULL_VOLTAGE ) {
        bms->set_internal_error();
    }
    highestCellVoltage = newHighestCellVoltage;
}

uint16_t Battery::get_highest_cell_voltage() {
    return highestCellVoltage;
}

// Return true if any cell in the battery is below the minimum voltage level
bool Battery::has_full_cell() {
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[p].has_full_cell() ) {
            return true;
        }
    }
    return false;
}

/*
 * Return the largest voltage difference between any two packs in this battery.
 */
uint32_t Battery::voltage_delta_between_packs() {
    uint32_t highestPackVoltage = 0;
    uint32_t lowestPackVoltage = 1000000; // 1000V
    for ( int p = 0; p < numPacks; p++ ) {
        float packVoltage = packs[p].get_voltage();
        if ( packVoltage > highestPackVoltage ) {
            highestPackVoltage = packVoltage;
        }
        if ( packVoltage < lowestPackVoltage ) {
            lowestPackVoltage = packVoltage;
        }
    }
    return highestPackVoltage - lowestPackVoltage;
}

// return the battery pack which has the highest voltage
BatteryPack* Battery::get_pack_with_highest_voltage() {
    BatteryPack* pack = &packs[0];
    for ( int p = 1; p < numPacks; p++ ) {
        if ( packs[p].get_voltage() > pack->get_voltage() ) {
            pack = &packs[p];
        }
    }
    return pack;
}

// Return true if the voltage difference between any two packs is too high and
// therefore it's unstafe to close the contactors.
bool Battery::packs_are_imbalanced() {
    return voltage_delta_between_packs() >= SAFE_VOLTAGE_DELTA_BETWEEN_PACKS;
}


//// ----
//
// Temperature
//
//// ----

void Battery::update_highest_sensor_temperature() {
    float newHighestSensorTemperature = packs[0].get_highest_temperature();
    for ( int p = 1; p < numPacks; p++ ) {
        if ( packs[p].get_highest_temperature() > newHighestSensorTemperature ) {
            newHighestSensorTemperature = packs[p].get_highest_temperature();
        }
    }
    // Saftey check
    if ( newHighestSensorTemperature < -20 || newHighestSensorTemperature > 50 ) {
        bms->set_internal_error();
    }
    this->highestSensorTemperature = newHighestSensorTemperature;
}

int8_t Battery::get_highest_sensor_temperature() {
    return highestSensorTemperature;
}

// Return true if any sensor in the pack is over the max temperature
bool Battery::too_hot() {
    return highestSensorTemperature >= MAXIMUM_TEMPERATURE;
}

void Battery::update_lowest_sensor_temperature() {
    float newLowestSensorTemperature = packs[0].get_lowest_temperature();
    for ( int p = 1; p < numPacks; p++ ) {
        if ( packs[p].get_lowest_temperature() < newLowestSensorTemperature ) {
            newLowestSensorTemperature = packs[p].get_lowest_temperature();
        }
    }
    // Safety check
    if ( newLowestSensorTemperature < -20 || newLowestSensorTemperature > 50 ) {
        this->bms->set_internal_error();
    }
    this->lowestSensorTemperature = newLowestSensorTemperature;
}

int8_t Battery::get_lowest_sensor_temperature() {
    return lowestSensorTemperature;
}

void Battery::process_temperature_update() {
    update_lowest_sensor_temperature();
    update_highest_sensor_temperature();
}


//// ----
//
// Charging
//
//// ----

bool Battery::too_cold_to_charge() {
    if ( get_lowest_sensor_temperature() < CHARGE_TEMPERATURE_MINIMUM ) {
        return true;
    }
    return false;
}

/* 
 * Return the maximum charge current that the whole battery can handle based on
 * temperature. Since we cannot control how much current each pack gets, this
 * will be determined by what the pack with the lowest max charge current can
 * handle. We also have to account for packs which are inhibited.
 */
uint16_t Battery::get_max_charge_current_by_temperature() {
    // Safeties
    if ( too_hot() || too_cold_to_charge() ) {
        return 0;
    }

    // Keep track of how many packs are not inhibited
    uint8_t activePacks = 1;

    // Get the smallest max charge current of all the packs
    uint16_t smallestMaxChargeCurrent = packs[0].get_max_charge_current_by_temperature();
    for ( int p = 1; p < numPacks; p++ ) {
        if ( packs[p].get_max_charge_current_by_temperature() < smallestMaxChargeCurrent ) {
            smallestMaxChargeCurrent = packs[p].get_max_charge_current_by_temperature();
        }
        if ( !packs[p].contactors_are_inhibited() ) {
            activePacks += 1;
        }
    }

    return smallestMaxChargeCurrent * activePacks;
}


//// ----
//
// Contactor control
//
//// ----

// Do not allow any contactors to close in any pack
void Battery::enable_inhibit_contactor_close() {
    if ( !all_contactors_inhibited() ) {
        printf("[battery][enable_inhibit_contactor_close] Enabling inhibit contactor close for all packs\n");
        for ( int p = 0; p < numPacks; p++ ) {
            packs[p].enable_inhibit_contactor_close();
        }
    }
}

void Battery::disable_inhibit_contactor_close() {
    if ( one_or_more_contactors_inhibited() ) {
        printf("[battery][disable_inhibit_contactor_close] Disabling inhibit contactor close for all packs\n");
        for ( int p = 0; p < numPacks; p++ ) {
            packs[p].disable_inhibit_contactor_close();
        }
    }
}

// If any of the packs have their contactors inhibited, return true
bool Battery::one_or_more_contactors_inhibited() {
    for ( int p = 0; p < numPacks; p++ ) {
        if ( packs[p].contactors_are_inhibited() ) {
            return true;
        }
    }
    return false;
}

bool Battery::all_contactors_inhibited() {
    for ( int p = 0; p < numPacks; p++ ) {
        if ( !packs[p].contactors_are_inhibited() ) {
            return false;
        }
    }
    return true;
}

// Allow contactors to close for the high pack and any other packs which are
// within SAFE_VOLTAGE_DELTA_BETWEEN_PACKS volts.
void Battery::disable_inhibit_contactors_for_drive() {
    int highPackId = get_index_of_high_pack();
    uint32_t highPackVoltage = packs[highPackId].get_voltage();
    uint32_t targetVoltage = highPackVoltage - SAFE_VOLTAGE_DELTA_BETWEEN_PACKS;
    for ( int p = 0; p < numPacks; p++ ) {
        if ( p == highPackId ) {
            packs[p].disable_inhibit_contactor_close();
            continue;
        }
        if ( packs[p].get_voltage() >= targetVoltage ) {
            packs[p].disable_inhibit_contactor_close();
        }
    }
}

// Allow contactors to close for the low pack and any other packs which are
// within SAFE_VOLTAGE_DELTA_BETWEEN_PACKS volts.
void Battery::disable_inhibit_contactors_for_charge() {
    int lowPackId = get_index_of_low_pack();
    uint32_t lowPackVoltage = packs[lowPackId].get_voltage();
    uint32_t targetVoltage = lowPackVoltage + SAFE_VOLTAGE_DELTA_BETWEEN_PACKS;
    for ( int p = 0; p < numPacks; p++ ) {
        if ( p == lowPackId ) {
            packs[p].disable_inhibit_contactor_close();
            continue;
        }
        if ( packs[p].get_voltage() <= targetVoltage ) {
            packs[p].disable_inhibit_contactor_close();
        }
    }
}

/* Returns a byte representing the liveness of modules starting at moduleId.
 * Zero is alive, one is dead. moduleId is index of the across the whole pack,
 * rather than indexed by pack and then module. */
int8_t Battery::get_module_liveness_byte(int8_t startModuleId) {
    int8_t livenessByte = 0;
    // If the module ID is out of range, return 0
    if ( startModuleId > ( NUM_PACKS * MODULES_PER_PACK ) ) {
        return livenessByte;
    }
    int8_t packId = ( NUM_PACKS * MODULES_PER_PACK ) / startModuleId;
    int8_t moduleId = ( NUM_PACKS * MODULES_PER_PACK ) % startModuleId;
    int8_t count = 0;
    while ( count < 8 ) {
        if ( !packs[packId].get_module_liveness(moduleId) ) {
            livenessByte |= 1 << count;
        }
        moduleId += 1;
        if ( moduleId >= MODULES_PER_PACK ) {
            moduleId = 0;
            packId += 1;
        }
        count += 1;
    }
    return livenessByte;
}

bool Battery::is_alive() {
    for ( int p = 0; p < numPacks; p++ ) {
        if ( !packs[p].is_alive() ) {
            return false;
        }
    }
    return true;
}

bool Battery::contactor_is_welded(uint8_t packId) {
    return packs[packId].contactors_are_welded();
}