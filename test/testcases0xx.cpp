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

/*
 * Test cases relating battery empty/full events
 */


/*
 * Test case 001
 * -----------------------------------------------------------------------------
 * Description: Ensure car cannot be driven when battery is empty, starting in
 *              STANDBY state.
 * Preconditions:
 *   1. Battery is not empty
 *   2. BMS is in STANDBY state
 *   3. DRIVE_INHIBIT signal is inactive
 *   4. Temperature is normal
 *   5. Ignition is off
 * Actions:
 *   1. Set cell voltage for one cell to minV (i.e., set battery empty)
 * Postconditions:
 *   1. BMS is in batteryEmpty state
 *   2. DRIVE_INHIBIT signal is active
 */
bool test_case_001(Battery* battery, Bms* bms) {
    printf("Running test [test_case_001] : inhibit drive when battery empty, from idle state\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // 0% SoC
    uint16_t newCellVoltage = static_cast<uint16_t>(CELL_EMPTY_VOLTAGE);
    printf("    > Setting all cell voltages to %dmV (0%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure DRIVE_INHIBIT goes high
    printf("    > Waiting for DRIVE_INHIBIT to activate\n");
    if ( ! assert_drive_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to batteryEmpty state
    printf("    > Waiting for BMS state to change to batteryEmpty\n");
    if ( ! assert_bms_state(bms, STATE_BATTERY_EMPTY) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;
}

/*
 * Test case 002
 * -----------------------------------------------------------------------------
 * Description: Ensure car cannot be driven when battery is empty, starting in
 *              DRIVE state.
 * Preconditions:
 *   1. Battery is not empty
 *   2. BMS is in DRIVE state
 *   3. DRIVE_INHIBIT signal is inactive
 *   4. Temperature is normal
 *   5. Ignition is on
 * Actions:
 *   1. Set cell voltage for one cell to minV
 * Postconditions:
 *   1. BMS is in batteryEmpty state
 *   2. DRIVE_INHIBIT signal is active
 */
bool test_case_002(Battery* battery, Bms* bms) {
    printf("Running test [test_case_002] : inhibit drive when battery empty, from drive state\n");

    if ( ! transition_to_drive_state(bms) ) {
        return false;
    }

    // 0% SoC
    uint16_t newCellVoltage = static_cast<uint16_t>(CELL_EMPTY_VOLTAGE);
    printf("    > Setting all cell voltages to %dmV (0%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure DRIVE_INHIBIT goes high
    printf("    > Waiting for DRIVE_INHIBIT to activate\n");
    if ( ! assert_drive_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to batteryEmpty state
    printf("    > Waiting for BMS state to change to batteryEmpty\n");
    if ( ! assert_bms_state(bms, STATE_BATTERY_EMPTY) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 003
 * -----------------------------------------------------------------------------
 * Description: When battery is empty and overheating, but the temperature
 *              drops, the car should still not be drivable as the battery is
 *              still empty.
 * Preconditions:
 *   1. Battery is empty
 *   2. BMS is in overTemp state
 *   3. DRIVE_INHIBIT signal is active
 *   4. Temperature is high
 * Actions:
 *   1. Set temperature to normal value
 * Postconditions:
 *   1. BMS is in batteryEmpty state
 *   2. DRIVE_INHIBIT signal is still active
 */
bool test_case_003(Battery* battery, Bms* bms) {
    printf("Running test [test_case_003] : empty battery, high temp drops\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // Set the temperature to 51C
    printf("    > Setting all temperatures to 51C\n");
    battery->set_all_temperatures(51);

    // wait for overTemp state
    printf("    > Waiting for BMS state to change to overTemp\n");
    if ( ! assert_bms_state(bms, STATE_OVER_TEMP_FAULT) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // 0% SoC
    uint16_t newCellVoltage = static_cast<uint16_t>(CELL_EMPTY_VOLTAGE);
    printf("    > Setting all cell voltages to %dmV (0%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure DRIVE_INHIBIT goes high
    printf("    > Waiting for DRIVE_INHIBIT to activate\n");
    if ( ! assert_drive_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to batteryEmpty state
    printf("    > Waiting for BMS state to change to batteryEmpty\n");
    if ( ! assert_bms_state(bms, STATE_BATTERY_EMPTY) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 004
 * -----------------------------------------------------------------------------
 * Description: When battery is empty and we're charging, but the charging
 *              stops, the car should still not be drivable as the battery is
 *              still empty.
 * Preconditions:
 *   1. Battery is empty
 *   2. BMS is in STANDBY state
 *   3. DRIVE_INHIBIT signal is inactive
 *   4. Temperature is normal
 * Actions:
 *   1. Start charging
 *   2. Stop charging
 * Postconditions:
 *   1. BMS is in batteryEmpty state
 *   2. DRIVE_INHIBIT signal is still active
 */
bool test_case_004(Battery* battery, Bms* bms) {
    printf("Running test [test_case_004] : empty battery, charging terminates\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // 0% SoC
    uint16_t newCellVoltage = static_cast<uint16_t>(CELL_EMPTY_VOLTAGE);
    printf("    > Setting all cell voltages to %dmV (0%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure DRIVE_INHIBIT goes high
    printf("    > Waiting for DRIVE_INHIBIT to activate\n");
    if ( ! assert_drive_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to batteryEmpty state
    printf("    > Waiting for BMS state to change to batteryEmpty\n");
    if ( ! assert_bms_state(bms, STATE_BATTERY_EMPTY) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Start charging
    set_charge_enable_state(true);

    // wait for BMS to go into CHARGING state
    printf("    > Waiting for BMS state to change to CHARGING\n");
    if ( ! assert_bms_state(bms, STATE_CHARGING) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Stop charging
    set_charge_enable_state(false);

    // wait for BMS to go into batteryEmpty state
    printf("    > Waiting for BMS state to change to batteryEmpty\n");
    if ( ! assert_bms_state(bms, STATE_BATTERY_EMPTY) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure DRIVE_INHIBIT is still active
    if ( ! assert_drive_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 005
 * -----------------------------------------------------------------------------
 * Description: When battery is full, the car should not be allowed to charge.
 * Preconditions:
 *   1. BMS is in STANDBY state
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 * Actions:
 *   1. Set cell voltage for one cell to maxV (i.e., set battery full)
 * Postconditions:
 *   1. BMS is in STANDBY state
 *   2. CHARGE_INHIBIT signal is active
 */
bool test_case_005(Battery* battery, Bms* bms) {
    printf("Running test [test_case_005] : battery full, disallow charge, from idle state\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // 100% SoC
    uint16_t newCellVoltage = static_cast<uint16_t>(CELL_FULL_VOLTAGE+10);
    printf("    > Setting all cell voltages to %dmV (100%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure CHARGE_INHIBIT goes high
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! assert_charge_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    return true;
}

/*
 * Test case 006
 * -----------------------------------------------------------------------------
 * Description: When battery is full, the car should not be allowed to charge.
 * Preconditions:
 *   1. BMS is in DRIVE state
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 * Actions:
 *   1. Set cell voltage for one cell to maxV (i.e., set battery full)
 * Postconditions:
 *   1. BMS is in DRIVE state
 *   2. CHARGE_INHIBIT signal is active
 */
bool test_case_006(Battery* battery, Bms* bms) {
    printf("Running test [test_case_006] : battery full disallow charge, from drive state\n");

    if ( ! transition_to_drive_state(bms) ) {
        return false;
    }

    // 100% SoC
    uint16_t newCellVoltage = static_cast<uint16_t>(CELL_FULL_VOLTAGE+10);
    printf("    > Setting all cell voltages to %dmV (100%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Make sure CHARGE_INHIBIT goes high
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! assert_charge_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;
}

// when charging, if the battery is full, the charge inhibit signal should be active