 /*
  * This file is part of the ev mustang bms project.
  *
  * Copyright (C) 2022 Christian Kelly <chrskly@chrskly.com>
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

#ifndef BMS_SRC_INCLUDE_MODULE_H_
#define BMS_SRC_INCLUDE_MODULE_H_

#include "settings.h"

class BatteryPack;

class BatteryModule {
 private:
    int id;
    int numCells;                             // Number of cells in this module
    int numTemperatureSensors;                // Number of temperature sensors in this module
    float cellVoltage[CELLS_PER_MODULE];      // Voltages of each cell
    float cellTemperature[TEMPS_PER_MODULE];  // Temperatures of each cell

    BatteryPack* pack;                        // The parent BatteryPack that contains this module

 public:
    BatteryModule();
    BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors);
};

#endif  // BMS_SRC_INCLUDE_MODULE_H_
