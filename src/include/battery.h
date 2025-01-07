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
#include "include/bms.h"
#include "settings.h"


class Io;

class Battery {
   private:
      BatteryPack packs[NUM_PACKS];
      int numPacks;                    // Number of battery packs in this battery
      uint32_t voltage;                // Total voltage of whole battery
      uint16_t lowestCellVoltage;      // Voltage of cell with lowest voltage across whole battery
      uint16_t highestCellVoltage;     // Voltage of cell with highest voltage across whole battery
      uint32_t minimumBatteryVoltage;  // Lowest permitted voltage of the whole battery
      uint32_t maximumBatteryVoltage;  // Highest permitted voltage of the whole battery
      uint8_t cellDelta;               // FIXME todo
      float lowestSensorTemperature;   //
      float highestSensorTemperature;  //
      Bms* bms;
      mutex_t* canMutex;
      Io* io;

   public:
      Battery() {};
      Battery(Io* _io);
      void initialise(Bms* _bms);
      int print();

      void request_data();
      void read_message();
      void send_test_message();

      // Voltage
      uint32_t get_voltage();
      void set_voltage(uint32_t voltage) { this->voltage = voltage; }
      void recalculate_voltage();
      void recalculate_cell_delta();
      uint32_t get_max_voltage();
      uint32_t get_min_voltage();
      int get_index_of_high_pack();
      int get_index_of_low_pack();
      void process_voltage_update();
      void recalculate_lowest_cell_voltage();
      uint16_t get_lowest_cell_voltage();
      bool has_empty_cell();
      void recalculate_highest_cell_voltage();
      uint16_t get_highest_cell_voltage();
      bool has_full_cell();
      uint32_t voltage_delta_between_packs();
      BatteryPack* get_pack_with_highest_voltage();
      bool packs_are_imbalanced(); 

      // Temperature
      void update_highest_sensor_temperature();
      int8_t get_highest_sensor_temperature();
      bool too_hot();
      void update_lowest_sensor_temperature();
      int8_t get_lowest_sensor_temperature();
      void process_temperature_update();
      bool too_cold_to_charge();
      int8_t get_max_charge_current_by_temperature();

      // Contactors
      void disable_inhibit_contactors_for_drive();
      void disable_inhibit_contactors_for_charge();
      void enable_inhibit_contactor_close();
      void disable_inhibit_contactor_close();
      bool one_or_more_contactors_inhibited();
      bool all_contactors_inhibited();

      int8_t get_module_liveness_byte(int8_t moduleId);
      bool contactor_is_welded(uint8_t packId);
};

#endif  // BMS_SRC_INCLUDE_BATTERY_H_
