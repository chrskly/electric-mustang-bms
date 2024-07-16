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

#ifndef BMS_SRC_INCLUDE_PACK_H_
#define BMS_SRC_INCLUDE_PACK_H_

#include "pico/multicore.h"

#include "hardware/timer.h"
#include "mcp2515/mcp2515.h"
#include "include/module.h"
#include "include/CRC8.h"
#include "settings.h"

class Battery;
class Bms;

const uint8_t finalxor[12] = { 0xCF, 0xF5, 0xBB, 0x81, 0x27, 0x1D, 0x53, 0x69, 0x02, 0x38, 0x76, 0x4C };

class BatteryPack {

   public:
      int id;

      BatteryPack();
      BatteryPack(int _id, int CANCSPin, int _contactorPin, int _numModules, 
            int _numCellsPerModule, int _numTemperatureSensorsPerModule, mutex_t* _canMutex, Bms* _bms);

      void set_battery(Battery* battery) { this->battery = battery; }

      void print();
      uint8_t getcheck(can_frame &msg, int id);
      int8_t get_module_liveness(int8_t moduleId);
      void request_data();
      void read_message();
      bool send_frame(can_frame *frame);

      void set_pack_error_status(int newErrorStatus);
      int get_pack_error_status();
      void set_pack_balance_status(int newBalanceStatus);
      int get_pack_balance_status();
      bool pack_is_due_to_be_balanced();
      void reset_balance_timer();

      // Voltage
      float get_voltage();
      void recalculate_total_voltage();
      uint16_t get_lowest_cell_voltage();
      bool has_empty_cell();
      uint16_t get_highest_cell_voltage();
      bool has_full_cell();
      void set_cell_voltage(int moduleIndex, int cellIndex, uint32_t newCellVoltage);
      void decode_voltages(can_frame *frame);
      void recalculate_cell_delta();
      void process_voltage_update();

      // Temperature
      bool has_temperature_sensor_over_max();
      int8_t get_lowest_temperature();
      int8_t get_highest_temperature();
      void decode_temperatures(can_frame *temperatureMessageFrame);
      void process_temperature_update();

      // Contactors
      void enable_inhibit_contactor_close();
      void disable_inhibit_contactor_close();
      bool contactors_are_inhibited();

      int16_t get_max_discharge_current();
      int16_t get_max_charge_current();

   private:
      MCP2515* CAN;                                     // CAN bus connection to this pack
      mutex_t* canMutex;
      Bms* bms;
      absolute_time_t lastUpdate;                      // Time we received last update from BMS
      int numModules;                                  //
      int numCellsPerModule;                           //
      int numTemperatureSensorsPerModule;              //
      Battery* battery;                                // The parent Battery that contains this BatteryPack
      float voltage;                                   // Voltage of the total pack
      int cellDelta;                                   // Difference in voltage between high and low cell, in mV

      // contactors
      int contactorInhibitPin;                         // Pin on the pico which controls contactors for this pack

      int balanceStatus;                               //
      int errorStatus;
      absolute_time_t nextBalanceTime;                 // Time that the next balance should occur.
      uint8_t pollMessageId;                           //
      bool initialised;
      BatteryModule modules[MODULES_PER_PACK];         // The child modules that make up this BatteryPack
      CRC8 crc8;

      bool inStartup;
      uint8_t modulePollingCycle;
      can_frame pollModuleFrame;

      uint8_t dischargeCurve[50] = {
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // -10C to -1C
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0C to 9C
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 10C to 19C
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 20C to 29C
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 30C to 39C
      };

      // C = 26Ah
      // -10° => +39° => whole charging range
      // below -10° => no charging, try and heat the battery
      // -10° to -1° => 3A to 6A
      //   0° to 15° => 4A to 125A
      //  16° to 35° => 125A
      //  36° to 39° => 50A
      // above 40° => no charging
      uint8_t chargeCurrentMax[50] = {
         3, 3, 3, 4, 4, 4, 5, 5, 6, 6,  // -10° to -1°
         13, 20, 27, 34, 41, 48, 55, 62, 69, 76, 83, 90, 97, 104, 111, 118,  // 0° to 15°
         125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125, 125,  // 16° to 35°
         50, 50, 50, 50,  // 36° to 39°
      };

      clock_t lastTemperatureSampleTime;
      int8_t lastTemperatureSample;
      int8_t temperatureDelta;
};

#endif  // BMS_SRC_INCLUDE_PACK_H_

