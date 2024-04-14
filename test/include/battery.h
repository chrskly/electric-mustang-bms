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

#ifndef BMS_SRC_INCLUDE_BATTERY_H_
#define BMS_SRC_INCLUDE_BATTERY_H_

#include "include/pack.h"
#include "settings.h"

class Battery {
 private:
    BatteryPack packs[NUM_PACKS];
    int numPacks;                  // Number of battery packs in this battery
    float voltage;                 // Total voltage of whole battery
    float lowestCellVoltage;       // Voltage of cell with lowest voltage across whole battery
    float highestCellVoltage;      // Voltage of cell with highest voltage across whole battery
    int cellDelta;                 // FIXME todo
    float lowestCellTemperature;   //
    float highestCellTemperature;  //

    float maxChargeCurrent;        //
    float maxDischargeCurrent;     //

    // Outputs
    bool heaterEnabled;            // Indicates that the battery heater is currently enabled
    bool inhibitCharge;            // Indicates that the BMS INHIBIT_CHARGE signal is enabled
    bool inhibitDrive;             // Indicates that the BMS INHIBIT_DRIVE signal is enabled

    // Inputs
    bool ignitionOn;
    bool chargeEnable;             // Charger is asking to charge

    float soc;

    // Something has gone wrong with the BMS
    bool internalError;

    // The voltage between the two packs
    bool packsAreImbalanced;

 public:
    Battery(int _numPacks);
    void initialise();
    void read_message();
    

};

#endif  // BMS_SRC_INCLUDE_BATTERY_H_
