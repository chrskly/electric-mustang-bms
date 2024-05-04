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

#include "include/battery.h"
#include "include/io.h"
#include "include/testcaseutils.h"

bool test_case_201_battery_too_cold_to_charge(Battery* battery) {
    printf("Running test [test_case_201_battery_too_cold_to_charge]\n");
    Bms* bms = battery->get_bms();

    // It doesn't matter what state we are in, CHARGE_INHIBIT should always activate when temp is too low

    // Set the temperature to -1C
    printf("    > Setting all temperatures to -1C\n");
    battery->set_all_temperatures(-1);

    // Wait for CHARGE_INHIBIT to activate
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        return false;
    }

    printf("    > Test passed\n");
    return true;

}

bool test_case_202_battery_warm_enough_to_charge_again(Battery* battery) {
    printf("Running test [test_case_202_battery_warm_enough_to_charge_again]\n");
    Bms* bms = battery->get_bms();

    // Set the temperature to -1C
    printf("  > Setting all temperatures to -1C\n");
    battery->set_all_temperatures(-1);

    // Wait for CHARGE_INHIBIT to activate
    printf("  > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("  > CHARGE_INHIBIT did not activate in time\n");
        return false;
    }

    // Set the temperature to 10C
    printf("  > Setting all temperatures to 10C\n");
    battery->set_all_temperatures(10);

    // Wait for CHARGE_INHIBIT to deactivate
    printf("  > Waiting for CHARGE_INHIBIT to deactivate\n");
    if ( ! wait_for_charge_inhibit_state(bms, false, 2000) ) {
        printf("  > CHARGE_INHIBIT did not deactivate in time\n");
        return false;
    }

    printf("  > Test passed\n");
    return true;

}

bool test_case_203_too_cold_to_charge_but_charge_requested(Battery* battery) {
    printf("Running test [test_case_203_too_cold_to_charge_but_charge_requested]\n");
    Bms* bms = battery->get_bms();

    // Get into idle state
    printf("  > Setting state to idle\n");
    bms->set_state(STATE_IDLE);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("  > Could not get into idle state\n");
        return false;
    }

    // Set the temperature to -1C
    printf("  > Setting all temperatures to -1C\n");
    battery->set_all_temperatures(-1);

    // Wait for CHARGE_INHIBIT to activate
    printf("  > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("  > CHARGE_INHIBIT did not activate in time\n");
        return false;
    }

    // Wait for HEATER_ENABLE to activate
    printf("  > Waiting for HEATER_ENABLE to activate\n");
    if ( ! wait_for_heater_enable_state(bms, true, 2000) ) {
        printf("  > HEATER_ENABLE did not activate in time\n");
        return false;
    }

    printf("  > Test passed\n");
    return true;

}

bool test_case_204_battery_too_hot_to_charge(Battery* battery) {
    printf("Running test [test_case_204_battery_too_hot_to_charge]\n");
    Bms* bms = battery->get_bms();

    // Set the temperature to 50C
    printf("  > Setting all temperatures to 50C\n");
    battery->set_all_temperatures(50);

    // Wait for CHARGE_INHIBIT to activate
    printf("  > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("  > CHARGE_INHIBIT did not activate in time\n");
        return false;
    }

    printf("  > Test passed\n");
    return true;

}