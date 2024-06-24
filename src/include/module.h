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

#ifndef BMS_SRC_INCLUDE_MODULE_H_
#define BMS_SRC_INCLUDE_MODULE_H_

#include "settings.h"

class BatteryPack;

class BatteryModule {
   private:
      int id;
      int numCells;                              // Number of cells in this module
      int numTemperatureSensors;                 // Number of temperature sensors in this module
      uint16_t cellVoltage[CELLS_PER_MODULE];    // Voltages of each cell
      int8_t cellTemperature[TEMPS_PER_MODULE];  // Temperatures of each cell
      bool allModuleDataPopulated;               // True when we have voltage/temp information for all cells
      clock_t lastHeartbeat;                     // Time when we last got an update from this module
      BatteryPack* pack;                         // The parent BatteryPack that contains this module

   public:
      BatteryModule();
      BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors);
      void print();

      // Voltage
      uint32_t get_voltage();
      uint16_t get_lowest_cell_voltage();
      uint16_t get_highest_cell_voltage();
      void set_cell_voltage(int cellIndex, uint16_t newCellVoltage);
      bool has_empty_cell();
      bool has_full_cell();

      // Module status
      bool all_module_data_populated();
      void check_if_module_data_is_populated();
      bool is_alive();
      void heartbeat();

      // Temperature
      void update_temperature(int tempSensorId, uint8_t newTemperature);
      int8_t get_lowest_temperature();
      int8_t get_highest_temperature();
      bool has_temperature_sensor_over_max();
      bool temperature_at_warning_level();

};

#endif  // BMS_SRC_INCLUDE_MODULE_H_
