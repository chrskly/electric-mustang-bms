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


#ifndef BMS_SRC_INCLUDE_LED_H_
#define BMS_SRC_INCLUDE_LED_H_

class Bms;

enum LED_MODE {
    STANDBY,
    DRIVE,
    CHARGING,
    FAULT
};

class StatusLight {
    private:
        bool on;
        int counter;
        int onDuration;
        int offDuration;
        Bms* bms;

    public:
        StatusLight() {};
        StatusLight(Bms* _bms);
        void set_mode(LED_MODE newMode);
        void led_blink();
};

#endif  // BMS_SRC_INCLUDE_LED_H_
