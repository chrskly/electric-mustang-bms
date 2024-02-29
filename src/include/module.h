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
    bool allModuleDataPopulated;              // True when we have voltage/temp information for all cells
    BatteryPack* pack;                        // The parent BatteryPack that contains this module

 public:
    BatteryModule();
    BatteryModule(int _id, BatteryPack* _pack, int _numCells, int _numTemperatureSensors);
    void print();

    // Voltage
    float get_voltage();
    float get_lowest_cell_voltage();
    float get_highest_cell_voltage();
    void update_cell_voltage(int cellIndex, float newCellVoltage);
    bool has_empty_cell();
    bool has_full_cell();

    bool all_module_data_populated();
    void check_if_module_data_is_populated();

    // Temperature
    void update_temperature(int tempSensorId, float newTemperature);
    float get_lowest_temperature();
    float get_highest_temperature();
    bool has_temperature_sensor_over_max();
    bool temperature_at_warning_level();

    // Charging
    int get_max_charge_current();
};

#endif  // BMS_SRC_INCLUDE_MODULE_H_
