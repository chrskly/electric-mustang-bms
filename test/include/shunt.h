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

#ifndef BMS_TEST_INCLUDE_SHUNT_H_
#define BMS_TEST_INCLUDE_SHUNT_H_

#include "mcp2515/mcp2515.h"

class Shunt {
   
   private:
      int32_t amps;
      int32_t shuntVoltage1;
      int32_t shuntVoltage2;
      int32_t shuntVoltage3;
      int32_t shuntTemperature;
      int32_t watts;
      int32_t ampSeconds;
      int32_t wattHours;
      struct can_frame frame;
      MCP2515* CAN;

   public:
      //Shunt(MCP2515* CAN);
      Shunt();
      void set_CAN_port(MCP2515* _CAN);
      void enable();
      void set_ampSeconds(int32_t newAmpSeconds);
      void send_ampSeconds();
      void set_wattHours(int32_t newWattHours);
      void send_wattHours();

};

#endif  // BMS_SRC_INCLUDE_SHUNT_H_
