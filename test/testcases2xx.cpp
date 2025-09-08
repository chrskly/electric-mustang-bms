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
 * Test cases relating to battery temperature
 *
 * 201 : When the battery is too cold to charge, the BMS should inhibit
 *       charging. Start in STANDBY state.
 * 202 : When the battery is too cold to charge, the BMS should inhibit
 *       charging. Start in DRIVE state.
 * 203 : When charge initiated, and the battery is too cold, heater should
 *       activate and charging should continue to be inhibited.
 * 204 : Charging has been initiated, but battery is too cold so heater is on.
 *       When temperature of battery goes over 0°C, disable battery heating.
 * 205 : If we're charging and some sensors are reading too cold to charge, but
 *       some sensors are reading too hot, then heater should not be enabled.
 * 206 : When charging, and the battery gets too hot, charging should be
 *       inhibited.
 */

/*
 * Test case 201
 * -----------------------------------------------------------------------------
 * Description: When the battery is too cold to charge, the BMS should inhibit
 *              charging. Start in STANDBY state.
 * Preconditions:
 *   1. BMS is in STANDBY state
 *   2. CHARGE_INHIBIT signal is inactive
 *   3. Temperature is normal
 * Actions:
 *   1. Set all temperatures to -20C
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is active
 */
bool test_case_201(Battery* battery, Bms* bms) {
    printf("Running test [test_case_201] : battery too cold to charge (standby)\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // Set the temperature to -20C
    printf("    > Setting all temperatures to -20C\n");
    battery->set_all_temperatures(-20);

    // Wait for CHARGE_INHIBIT to activate
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 202
 * -----------------------------------------------------------------------------
 * Description: When the battery is too cold to charge, the BMS should inhibit
 *              charging. Start in DRIVE state.
 * Preconditions:
 *   1. BMS state == DRIVE
 *   2. CHARGE_INHIBIT signal is inactive
 *   3. Temperature is normal
 * Actions:
 *   1. Set all temperatures to -20C
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is active
 */
bool test_case_202(Battery* battery, Bms* bms) {
    printf("Running test [test_case_202] : battery too cold to charge (drive)\n");

    if ( ! transition_to_drive_state(bms) ) {
        return false;
    }

    // Set the temperature to -20C
    printf("    > Setting all temperatures to -20C\n");
    battery->set_all_temperatures(-20);

    // Wait for CHARGE_INHIBIT to activate
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 203
 * -----------------------------------------------------------------------------
 * Description: When charge initiated, and the battery is too cold, heater
 *              should activate and charing should continue to be inhibited.
 * 
 * Preconditions:
 *   1. BMS is in STANDBY state
 *   2. CHARGE_INHIBIT signal is active
 *   3. Temperature is -20C
 * Actions:
 *   1. Initiate charging
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is inactive
 *   2. HEATER_ENABLE signal is active
 *   3. BMS is in BATTERY WARMING state
 */
bool test_case_203(Battery* battery, Bms* bms) {
    printf("Running test [test_case_203] : battery needs heating to charge\n");

    transition_to_standby_state(bms);

    // Set the temperature to -20C
    printf("    > Setting all temperatures to -20C\n");
    battery->set_all_temperatures(-20);

    // Make sure CHARGE_INHIBIT goes high
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! assert_charge_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Start charging
    set_charge_enable_state(true);

    // Make sure CHARGE_INHIBIT stays high
    printf("    > Ensuring CHARGE_INHIBIT stays active\n");
    if ( ! assert_charge_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure HEATER_ENABLE goes high
    printf("    > Ensuring HEATER_ENABLE is active\n");
    if ( ! assert_heater_enable_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to batteryHeating state
    printf("    > Waiting for BMS state to change to batteryHeating\n");
    if ( ! assert_bms_state(bms, STATE_BATTERY_HEATING) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 204
 * -----------------------------------------------------------------------------
 * Description: Charging has been initiated, but battery is too cold so heater
 *              is on. When temperature of battery goes over 0°C, disable
 *              battery heating.
 *
 * Preconditions:
 *   1. BMS is in batteryHeating state
 *   2. CHARGE_INHIBIT signal is active
 *   3. Temperature is -20°C
 * Actions:
 *   1. Set all temperatures to normal (1°C)
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is inactive
 *   2. BMS is in CHARGING state
 *   3. HEATER_ENABLE signal is inactive
 */
bool test_case_204(Battery* battery, Bms* bms) {
    printf("Running test [test_case_204] : battery warm enough to charge again\n");

    transition_to_charging_state(bms);

    // Set the temperature to -20°C
    printf("    > Setting all temperatures to -20°C\n");
    battery->set_all_temperatures(-20);

    // Make sure CHARGE_INHIBIT goes high
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! assert_charge_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Set the temperature to 1°C
    printf("    > Setting all temperatures to 1°C\n");
    battery->set_all_temperatures(1);

    // Make sure CHARGE_INHIBIT goes low
    printf("    > Waiting for CHARGE_INHIBIT to deactivate\n");
    if ( ! assert_charge_inhibit_state(bms, false) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure HEATER_ENABLE goes low
    printf("    > Waiting for HEATER_ENABLE to deactivate\n");
    if ( ! assert_heater_enable_state(bms, false) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 205
 * -----------------------------------------------------------------------------
 * Description: If we're charging and some sensors are reading too cold to
 *              charge, but some sensors are reading too hot, then heater should
 *              not be enabled.
 *
 * Preconditions:
 *   1. BMS is in IDLE state
 *   2. Temperature sensor in pack 1 is -20°C
 *   3. Temperature sensor in pack 2 is 50°C
 * Actions:
 *   1. Initiate charging
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is active
 *   2. HEATER_ENABLE signal is inactive
 *   3. BMS is in OVERTEMP state
 */
bool test_case_205(Battery* battery, Bms* bms) {
    printf("Running test [test_case_205] : battery simultaneously too hot and too cold\n");

    transition_to_standby_state(bms);

    // Set the temperature of pack 0 to -20°C, pack 1 to 50°C
    printf("    > Setting all temperatures to -20°C\n");
    battery->get_pack(0)->set_all_temperatures(-20);
    battery->get_pack(1)->set_all_temperatures(50);

    // Make sure CHARGE_INHIBIT goes high
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! assert_charge_inhibit_state(bms, true) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure HEATER_ENABLE goes low
    printf("    > Waiting for HEATER_ENABLE to deactivate\n");
    if ( ! assert_heater_enable_state(bms, false) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to overtemp state
    printf("    > Waiting for BMS state to change to overTempFault\n");
    if ( ! assert_bms_state(bms, STATE_OVER_TEMP_FAULT) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 206
 * -----------------------------------------------------------------------------
 * Description: When charging, and the battery gets too hot, charging should be
 *              inhibited.
 * Preconditions:
 *   1. BMS is in CHARGING state
 *   2. Temperature is set to 20°C
 *   3. CHARGE_INHIBIT signal is inactive
 * Actions:
 *   1. Set the temperature to 50°C
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is active
 *   2. BMS is in OVERTEMP state
 */
bool test_case_205(Battery* battery, Bms* bms) {
    printf("Running test [test_case_205] : battery overheats when charging\n");

    if ( ! transition_to_charging_state(bms) ) {
        return false;
    }

    // Set the temperature to 50C
    printf("    > Setting all temperatures to 50C\n");
    battery->set_all_temperatures(50);

    // Wait for CHARGE_INHIBIT to activate
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Make sure CAN messages show the car has switched to overtemp state
    printf("    > Waiting for BMS state to change to overTempFault\n");
    if ( ! assert_bms_state(bms, STATE_OVER_TEMP_FAULT) ) {
        printf("    > Test FAILED\n");
        return false;
    }

    printf("  > Test PASSED\n");
    return true;

}