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

#ifndef BMS_TEST_INCLUDE_PACK_H_
#define BMS_TEST_INCLUDE_PACK_H_

#include <stdio.h>
#include "mcp2515/mcp2515.h"
#include "include/module.h"

class Battery;

class BatteryModule;

class BatteryPack {
   public:
      int id;
      BatteryPack();
      BatteryPack(int _id, int CANCSPin, int _contactorPin, int _numModules, int _numCellsPerModule, int _numTemperatureSensorsPerModule);
      void print();
      bool send_frame(can_frame* frame);
      void set_all_cell_voltages(uint16_t newVoltage);
      void set_battery(Battery* battery) { this->battery = battery; }
      void send_module_voltages(uint8_t moduleId);
      void send_module_temperatures(uint8_t moduleId);
      void read_frame();
      bool send_message(can_frame *frame);
      bool get_inhibit();
      void set_inhibit(bool inhibit);
      void set_all_temperatures(int8_t newTemperature);

      int get_id() { return id; }

   private:
      MCP2515* CAN;                                // CAN bus connection to this pack
      int numModules;                                  //
      int numCellsPerModule;                           //
      int numTemperatureSensorsPerModule;              //

      // contactors
      int contactorInhibitPin;                         // Pin on the pico which controls contactors for this pack
      bool contactorsAreInhibited;                     // Is the
      Battery* battery;                                // The parent Battery that contains this BatteryPack
      BatteryModule modules[MODULES_PER_PACK];         // The child modules that make up this BatteryPack
};

#endif  // BMS_TEST_INCLUDE_PACK_H_