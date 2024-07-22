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

#ifndef BMS_SRC_INCLUDE_IO_H_
#define BMS_SRC_INCLUDE_IO_H_

#include "include/bms.h"

class Bms;

class Io {
    private:
        Bms* bms;
        // Inputs
        bool ignitionOn;
        bool chargeEnable;             // Charger is asking to charge
    public:
        Io();
        void enable_drive_inhibit(std::string context);
        void disable_drive_inhibit(std::string context);
        bool drive_is_inhibited();
        void enable_charge_inhibit(std::string context);
        void disable_charge_inhibit(std::string context);
        bool charge_is_inhibited();
        void enable_heater();
        void disable_heater();
        bool heater_is_enabled();

        bool ignition_is_on();
        bool charge_enable_is_on();
        bool pos_contactor_is_welded();
        bool neg_contactor_is_welded();
};

#endif  // BMS_SRC_INCLUDE_IO_H_
