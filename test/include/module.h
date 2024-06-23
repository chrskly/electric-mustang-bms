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

#ifndef BMS_TEST_INCLUDE_MODULE_H_
#define BMS_TEST_INCLUDE_MODULE_H_

#include <stdio.h>
#include <cstdint>
#include "settings.h"

class BatteryPack;

class BatteryModule {
   private:
      int id;
      int numCells;                              // Number of cells in this module
      int numTemperatureSensors;                 // Number of temperature sensors in this module
      uint16_t cellVoltage[CELLS_PER_MODULE];    // Voltages of each cell
      int8_t cellTemperature[TEMPS_PER_MODULE];  // Temperatures of each cell
      BatteryPack* pack;                         // The parent BatteryPack that contains this module

   public:
      BatteryModule();
      BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors);
      void print();
      uint16_t get_cell_voltage(int cellId);
      void set_all_cell_voltages(uint16_t newVoltage);
      int8_t get_cell_temperature(int cellId);
      void set_all_temperatures(uint8_t newTemperature);
      void set_temperature(int sensorId, uint8_t temperature);
};

#endif  // BMS_TEST_INCLUDE_MODULE_H_
