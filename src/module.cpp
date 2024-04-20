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

/*

    MODULE

*/

#include <stdio.h>
#include <time.h>

#include "include/module.h"
#include "include/pack.h"
#include "include/util.h"


BatteryModule::BatteryModule() {}

//
BatteryModule::BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors) {
    // printf("Creating module (id:%d, pack:%d, cpm:%d, t:%d)\n", _id, _pack->id, _numCells, _numTemperatureSensors);
    id = _id;
    // Point back to parent pack
    pack = _pack;
    // Initialise all cell voltages to zero
    numCells = _numCells;
    for ( int c = 0; c < numCells; c++ ) {
        cellVoltage[c] = 0;
    }
    // Initialise temperature sensor readings to zero
    numTemperatureSensors = _numTemperatureSensors;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        cellTemperature[t] = -128;
    }
    allModuleDataPopulated = false;
}

void BatteryModule::print() {
    printf("    Module id : %d (numCells : %d)\n", id, numCells);
    printf("        Cell Voltages : ");
    for ( int c = 0; c < numCells; c++ ) {
        printf("%d:%umV ", c, cellVoltage[c]);
    }
    printf("\n");
    printf("        Temperatures : ");
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        printf("%d:%dC ", t, cellTemperature[t]);
    }
    printf("\n");
}


//// ----
//
// Voltage
//
//// ----

// Return total module voltage by summing the cell voltages
uint32_t BatteryModule::get_voltage() {
    uint32_t voltage = 0;
    for ( int c = 0; c < numCells; c++ ) {
        voltage += cellVoltage[c];
    }
    return voltage;
}

// Return the voltage of the lowest cell voltage in the module
uint16_t BatteryModule::get_lowest_cell_voltage() {
    uint16_t lowestCellVoltage = 10000;
    for ( int c = 0; c < numCells; c++ ) {
        // printf("Comparing %3.3f and %3.3f\n", cellVoltage[c], lowestCellVoltage);
        if ( cellVoltage[c] < lowestCellVoltage ) {
            lowestCellVoltage = cellVoltage[c];
        }
    }
    return lowestCellVoltage;
}

// Return the voltage of the highest cell in the module
uint16_t BatteryModule::get_highest_cell_voltage() {
    uint16_t highestCellVoltage = 0;
    for ( int c = 0; c < numCells; c++ ) {
        // printf("module : comparing : %.4f to %.4f\n", cellVoltage[c], highestCellVoltage);
        if ( cellVoltage[c] > highestCellVoltage ) {
            highestCellVoltage = cellVoltage[c];
        }
    }
    return highestCellVoltage;
}

// Update the voltage for a single cell
void BatteryModule::set_cell_voltage(int cellIndex, uint16_t newCellVoltage) {
    // printf("module : update_cell_voltage : %d : %.4f\n", cellIndex, newCellVoltage);
    cellVoltage[cellIndex] = newCellVoltage;
}

// Return true if any of the cells in the module are under min voltage
bool BatteryModule::has_empty_cell() {
    for ( int c = 0; c < numCells; c++ ) {
        if ( cellVoltage[c] < CELL_EMPTY_VOLTAGE ) {
            return true;
        }
    }
    return false;
}

// Return true if any of the cells in the module are over max voltage
bool BatteryModule::has_full_cell() {
    for ( int c = 0; c < numCells; c++ ) {
        if ( cellVoltage[c] > CELL_FULL_VOLTAGE ) {
            return true;
        }
    }
    return false;
}

//// ----
//
// Status
//
//// ----

// Return true if we have voltage/temp information for all cells
bool BatteryModule::all_module_data_populated() {
    return allModuleDataPopulated;
}

void BatteryModule::check_if_module_data_is_populated() {
    bool voltageMissing = false;
    for ( int c = 0; c < numCells; c++ ) {
        if ( cellVoltage[c] == 0 ) {
            voltageMissing = true;
        }
    }
    bool temperatureMissing = false;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        if ( cellTemperature[t] == 0 ) {
            temperatureMissing = true;
        }
    }
    allModuleDataPopulated = voltageMissing && temperatureMissing;
}

bool BatteryModule::is_alive() {
    return ( ((double)(get_clock() - lastHeartbeat) / CLOCKS_PER_SEC) < MODULE_TTL );
}

void BatteryModule::heartbeat() {
    lastHeartbeat = get_clock();
}

//// ----
//
// Temperature
//
//// ----

// Update the value for one of the temperature sensors
void BatteryModule::update_temperature(int tempSensorId, uint8_t newTemperature) {
    cellTemperature[tempSensorId] = newTemperature;
}

// Return the temperature of the coldest sensor in the module
int8_t BatteryModule::get_lowest_temperature() {
    uint8_t lowestTemperature = 255;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        // Skip uninitialised readings
        if ( cellTemperature[t] < -127 ) {
            continue;
        }
        if ( cellTemperature[t] < lowestTemperature ) {
            lowestTemperature = cellTemperature[t];
        }
    }
    return lowestTemperature;
}

// Return the temperature of the hottest sensor in the module
int8_t BatteryModule::get_highest_temperature() {
    int8_t highestTemperature = -40;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        // Skip uninitialised readings
        if ( cellTemperature[t] < -127 ) {
            continue;
        }
        if ( cellTemperature[t] > highestTemperature ) {
            highestTemperature = cellTemperature[t];
        }
    }
    return highestTemperature;
}

// Return true if any temperature sensor is over the max temperature
bool BatteryModule::has_temperature_sensor_over_max() {
    return ( get_highest_temperature() > MAXIMUM_TEMPERATURE );
}



// returns true when any temperature sensor in this module is over the warning
// level, but below the critical level.
bool BatteryModule::temperature_at_warning_level() {
    for ( int c = 0; c < numCells; c++ ) {
        if ( cellTemperature[c] >= WARNING_TEMPERATURE && cellTemperature[c] < MAXIMUM_TEMPERATURE ) {
            return true;
        }
    }
    return false;
}


//// ----
//
// Charging
//
//// ----

// Return the maximum current the charger may push into the module
int BatteryModule::get_max_charge_current() {
    float highestTemperature = get_highest_temperature();
    if ( highestTemperature > CHARGE_THROTTLE_TEMP_LOW ) {
        float degreesOver = highestTemperature - CHARGE_THROTTLE_TEMP_LOW;
        float scaleFactor = 1 - (degreesOver / (CHARGE_THROTTLE_TEMP_HIGH - CHARGE_THROTTLE_TEMP_LOW));
        float chargeCurrent = (scaleFactor * (CHARGE_CURRENT_MAX - CHARGE_CURRENT_MIN)) + CHARGE_CURRENT_MIN;
        return static_cast<int>(chargeCurrent);
    } else {
        return static_cast<int>(CHARGE_CURRENT_MAX);
    }
}





