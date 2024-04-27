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

#ifndef BMS_TEST_INCLUDE_BATTERY_H_
#define BMS_TEST_INCLUDE_BATTERY_H_

#include "settings.h"
#include "include/pack.h"
#include "include/bms.h"

class Battery {
   private:
      int numPacks;                  // Number of battery packs in this battery
      float voltage;                 // Total voltage of whole battery
      float lowestCellVoltage;       // Voltage of cell with lowest voltage across whole battery
      float highestCellVoltage;      // Voltage of cell with highest voltage across whole battery
      int cellDelta;                 // FIXME todo
      float lowestCellTemperature;   //
      float highestCellTemperature;  //
      float maxChargeCurrent;        //
      float maxDischargeCurrent;     //
      int8_t soc;
      bool ignitionOn;
      BatteryPack packs[NUM_PACKS];
      Bms bms;

   public:
      Battery(int _numPacks);
      void initialise();
      void set_all_cell_voltages(uint16_t newCellVoltage);
      uint16_t get_voltage_from_soc(int8_t soc);
      void read_message();
      Bms* get_bms();
};

#endif  // BMS_TEST_INCLUDE_BATTERY_H_
