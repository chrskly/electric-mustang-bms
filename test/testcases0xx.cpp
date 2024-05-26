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
#include "include/testcaseutils.h"

bool test_case_001_ensure_car_cannot_be_driven_when_battery_is_empty(Battery* battery) {
    printf("Running test [test_case_001_ensure_car_cannot_be_driven_when_battery_is_empty]\n");
    Bms* bms = battery->get_bms();
    /*
      Preconditions
        1. All cells are above Vmin
        2. DRIVE_INHIBIT signal is not active
     */

    // Set SoC to 50%
    uint16_t newCellVoltage = battery->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dmV (approx 50%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    printf("    > Waiting for DRIVE_INHIBIT to deactivate\n");
    if ( ! wait_for_drive_inhibit_state(bms, false, 2000) ) {
        printf("    > DRIVE_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    } else {
        printf("    > DRIVE_INHIBIT deactivated\n");
    }

    // Execute test - drop cell voltage to Vmin

    // 0% SoC
    newCellVoltage = static_cast<uint16_t>(CELL_EMPTY_VOLTAGE);
    printf("    > Setting all cell voltages to %dmV (0%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure DRIVE_INHIBIT goes high
    printf("    > Waiting for DRIVE_INHIBIT to activate\n");
    if ( ! wait_for_drive_inhibit_state(bms, false, 2000) ) { // active, 2 second timeout
        printf("    > DRIVE_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    } else {
        printf("    > DRIVE_INHIBIT activated\n");
    }

    // Make sure CAN messages show the car has switched to batteryEmpty state
    printf("    > Waiting for BMS state to change to batteryEmpty\n");
    if ( ! wait_for_bms_state(bms, STATE_BATTERY_EMPTY, 2000) ) { // BmsState, 2 second timeout
        printf("    > BMS state did not change to batteryEmpty in time\n");
        printf("    > Test failed\n");
        return false;
    } else {
        printf("    > BMS state changed to batteryEmpty\n");
    }

    printf("    > Test passed\n");
    return true;

}

bool test_case_002_ensure_battery_cannot_be_charged_when_full(Battery* battery) {
    printf("Running test [test_case_002_ensure_battery_cannot_be_charged_when_full]\n");
    Bms* bms = battery->get_bms();
    /*
      Preconditions
        1. All cells are below Vmax
        2. CHARGE_INHIBIT signal is not active
    */

    // Set SoC to 50%
    uint16_t newCellVoltage = battery->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dmV (approx 50%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    printf("    > Waiting for CHARGE_INHIBIT to deactivate\n");
    if ( ! wait_for_charge_inhibit_state(bms, false, 5000) ) {
        printf("    > CHARGE_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    } else {
        printf("    > CHARGE_INHIBIT deactivated\n");
    }

    // Execute test - raise cell voltage to Vmax

    // 100% SoC
    newCellVoltage = static_cast<uint16_t>(CELL_FULL_VOLTAGE+10);
    printf("    > Setting all cell voltages to %dmV (100%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure CHARGE_INHIBIT goes high
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 5000) ) { // active, 2 second timeout
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    } else {
        printf("    > CHARGE_INHIBIT activated\n");
    }

    // No need to check BMS state, as it does not need to be a specific state

    printf("    > Test passed\n");
    return true;
}