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


#include <time.h>

#include "include/util.h"
#include "include/bms.h"

bool wait_for_50_percent_soc(Bms* bms, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_soc() < 50) {
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_drive_inhibit_to_activate() {
    //
}

bool wait_for_bms_state_to_change_to_batteryEmpty() {
    //
}

bool wait_for_charge_inhibit_to_activate() {
    //
}