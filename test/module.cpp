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

#include "include/module.h"

BatteryModule::BatteryModule() {}

BatteryModule::BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors) {
    // printf("Creating module (id:%d, pack:%d, cpm:%d, t:%d)\n", _id, _pack->id, _numCells, _numTemperatureSensors);
    id = _id;
    // Point back to parent pack
    pack = _pack;
    // Initialise all cell voltages to zero
    // numCells = _numCells;
    numCells = 16;
    for ( int c = 0; c < numCells; c++ ) {
        cellVoltage[c] = 3450 + c;
    }
    // Initialise temperature sensor readings to zero
    numTemperatureSensors = _numTemperatureSensors;
    for ( int t = 0; t < numTemperatureSensors; t++ ) {
        cellTemperature[t] = 10 + t;
    }
}

uint16_t BatteryModule::get_cell_voltage(int cellId) {
    return cellVoltage[cellId];
}

void BatteryModule::set_all_cell_voltages(uint16_t newVoltage) {
    for ( int c = 0; c < numCells; c++ ) {
        cellVoltage[c] = newVoltage;
    }
}

uint8_t BatteryModule::get_cell_temperature(int cellId) {
    return cellTemperature[cellId];
}

void BatteryModule::set_cell_temperature(int cellId, uint8_t temperature) {
    cellTemperature[cellId] = temperature;
}
