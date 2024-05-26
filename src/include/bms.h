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

#ifndef BMS_SRC_INCLUDE_BMS_H_
#define BMS_SRC_INCLUDE_BMS_H_

#include <stdio.h>
#include <string>
#include "include/statemachine.h"
#include "include/io.h"
#include "include/led.h"
#include "include/battery.h"


class Io;

class Bms {
    private:
        Battery* battery;
        State state;
        Io* io;
        StatusLight statusLight;
        int8_t maxChargeCurrent;     //
        int8_t maxDischargeCurrent;  //
        uint8_t soc;
        bool internalError;
        bool watchdogReboot;

    public:
        Bms(Battery* battery, Io* io);

        // State and events
        void set_state(State _state, std::string reason);
        State get_state();
        void send_event(Event event);
        void print();

        // Watchdog
        void set_watchdog_reboot(bool value);

        // DRIVE_INHIBIT
        void enable_drive_inhibit(std::string context);
        void disable_drive_inhibit(std::string context);
        bool drive_is_inhibited();

        // CHARGE_INHIBIT
        void enable_charge_inhibit(std::string context);
        void disable_charge_inhibit(std::string context);
        bool charge_is_inhibited();

        // HEATER
        void enable_heater();
        void disable_heater();
        bool heater_is_enabled();

        // IGNITION
        bool ignition_is_on();

        // CHARGE_ENABLE
        bool charge_is_enabled();

        // SOC
        uint8_t get_soc();
        void recalculate_soc();

        // Error
        void set_internal_error();
        void clear_internal_error();

        uint8_t get_error_byte();
        uint8_t get_status_byte();

        // Charger
        void update_max_charge_current();
        int8_t get_max_charge_current();
        void update_max_discharge_current();
        int8_t get_max_discharge_current();

        // Status light
        void led_blink();
};

#endif  // BMS_SRC_INCLUDE_BMS_H_
