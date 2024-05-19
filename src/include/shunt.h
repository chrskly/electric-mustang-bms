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

#ifndef BMS_SRC_INCLUDE_SHUNT_H_
#define BMS_SRC_INCLUDE_SHUNT_H_

class Shunt {
    private:
        clock_t lastHeartbeat;  // Time we last got an update from the ISA Shunt
        int32_t amps;
        int32_t voltage1;
        int32_t voltage2;
        int32_t voltage3;
        int32_t temperature;
        int32_t watts;
        int32_t ampSeconds;
        int32_t wattHours;
    public:
        Shunt();
        void heartbeat();
        bool is_dead();
        int32_t get_amps();
        void set_amps(int32_t _amps);
        int32_t get_voltage1();
        void set_voltage1(int32_t _voltage1);
        int32_t get_voltage2();
        void set_voltage2(int32_t _voltage2);
        int32_t get_voltage3();
        void set_voltage3(int32_t _voltage3);
        int32_t get_temperature();
        void set_temperature(int32_t _temperature);
        int32_t get_watts();
        void set_watts(int32_t _watts);
        int32_t get_ampSeconds();
        void set_ampSeconds(int32_t _ampSeconds);
        int32_t get_wattHours();
        void set_wattHours(int32_t _wattHours);
};

#endif  // BMS_SRC_INCLUDE_SHUNT_H_