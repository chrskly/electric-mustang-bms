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

bool test_case_001_ensure_car_cannot_be_driven_when_battery_is_empty() {
    /*
      Preconditions
        1. All cells are above Vmin
        2. DRIVE_INHIBIT signal is not active
     */
    set_battery_to_50_percent_soc();
    wait_for_50_percent_soc();

    if ( drive_inhibit_is_active() ) {
        // check state and fix?
    }

    // Execute test - drop cell voltage below Vmin
    set_battery_to_0_percent_soc();

    // Make sure DRIVE_INHIBIT goes high
    wait_for_drive_inhibit_to_activate();
}

bool test_case_002_ensure_battery_cannot_be_charged_when_full() {
    /*
      Preconditions
        1. All cells are below Vmax
        2. CHARGE_INHIBIT signal is not active
    */
    set_battery_to_50_percent_soc();
    wait_for_50_percent_soc();

    if ( charge_inhibit_is_active() ) {
        // check state and fix?
    }

    // Execute test - raise cell voltage to Vmax
    set_battery_to_100_percent_soc();

    // Make sure CHARGE_INHIBIT goes high
    wait_for_charge_inhibit_to_activate();
}