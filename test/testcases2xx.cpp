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
 * Description: when charge in inhibited due to the battery being too cold, if
 *              the battery warms up sufficiently, the BMS should allow charging
 *              to resume.
 * 
 * Preconditions:
 *   1. BMS is in batteryHeating state
 *   2. CHARGE_INHIBIT signal is active
 *   3. Temperature is -20C
 * Actions:
 *   1. Set all temperatures to normal (20C)
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is inactive
 *   2. BMS is in CHARGING state
 */
bool test_case_203(Battery* battery, Bms* bms) {
    printf("Running test [test_case_203] : battery warm enough to charge again\n");

    transition_to_charging_state(bms);

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

    // FIXME check for heater enabled

    // Set the temperature to 10C
    printf("    > Setting all temperatures to 10C\n");

    // Wait for CHARGE_INHIBIT to deactivate
    printf("    > Waiting for CHARGE_INHIBIT to deactivate\n");
    if ( ! wait_for_charge_inhibit_state(bms, false, 2000) ) {
        printf("    > CHARGE_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 204
 * -----------------------------------------------------------------------------
 * Description: When the battery is too cold to charge, the BMS should inhibit
 *              charging. (standby state)
 * Preconditions:
 *   1. BMS is in STANDBY state
 *   2. CHARGE_INHIBIT signal is inactive
 *   3. Temperature is normal
 * Actions:
 *   1. Set all temperatures to -20C
 * Postconditions:
 *   1. CHARGE_INHIBIT signal is active
 */
bool test_case_204(Battery* battery, Bms* bms) {
    printf("Running test [test_case_204] : too cold to charge but charge requested\n");

    transition_to_standby_state(bms);

    // Set the temperature to -1C
    printf("    > Setting all temperatures to -20C\n");
    battery->set_all_temperatures(-20);

    // Wait for CHARGE_INHIBIT to activate
    printf("    > Waiting for CHARGE_INHIBIT to activate\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Wait for HEATER_ENABLE to activate
    printf("    > Waiting for HEATER_ENABLE to activate\n");
    if ( ! wait_for_heater_enable_state(bms, true, 2000) ) {
        printf("    > HEATER_ENABLE did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 205
 * -----------------------------------------------------------------------------
 * Description: When the battery is too hot to charge, the BMS should inhibit
 *              charging.
 * Preconditions:
 *   1. BMS is in standby state
 *   2. CHARGE_INHIBIT signal is inactive
 */
bool test_case_205(Battery* battery, Bms* bms) {
    printf("Running test [test_case_205] : battery too hot to charge\n");

    if ( ! transition_to_standby_state(bms) ) {
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

    printf("  > Test PASSED\n");
    return true;

}