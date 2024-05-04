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
--------------------------------------------------------------------------------
  Test that the inhibition of contactor close is working as intended.

  Inhibition should be allowed from standby state. Inhibition should not be
  allowed from any other state yet. FIXME : in certain circumstances, we may
  allow inhibition from other states.

  Preconditions
    1. We're in state standby
    2. DRIVE_INHIBIT signal is not active
    3. batt1 inhibit off
    4. batt2 inhibit off
*/
bool test_case_101_inhibit_battery_contactor_close_when_pack_voltages_differ(Battery* battery) {
    printf("Running test [test_case_101_inhibit_battery_contactor_close_when_pack_voltages_differ]\n");
    Bms* bms = battery->get_bms();

    // Get into idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to idle in time\n");
        return false;
    }

    // Ensure no delta between packs by setting all cell voltages to 50% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dV (approx 50%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Wait for all battery inhibit signals to disable
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        printf("    > Waiting for BATT%d_INHIBIT to deactivate\n", p+1);
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            return false;
        }
    }

    // Execute test - set pack 1 to 0% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(0);
    printf("    > Setting all cell voltages to %dmV (0%% soc) for pack 1\n", newCellVoltage);
    battery->get_pack(0).set_all_cell_voltages(newCellVoltage);

    // Make sure both packs are inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            return false;
        }
    }

    // Make sure CAN messages show the imbalanced state
    if ( ! wait_for_packs_imbalanced_state(bms, true, 2000) ) { // BmsState, 2 second timeout
        printf("    > BMS did not flag the packsImbalanced state in time\n");
        return false;
    }

    return true;
}

/*
--------------------------------------------------------------------------------
  The contactors should not open when in drive state.

  Preconditions
    1. Packs start balanced
    2. In drive mode
    3. batt1 inhibit off
    4. batt2 inhibit off
*/
bool test_case_102_do_not_inhibit_battery_contactor_close_when_pack_voltage_differ_and_ignition_is_on(Battery* battery) {
    printf("Running test [test_case_102_do_not_inhibit_battery_contactor_close_when_pack_voltage_differ_and_ignition_is_on]\n");
    Bms* bms = battery->get_bms();

    // Ensure no delta between packs by setting all cell voltages to 50% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dV (approx 50%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Wait for all battery inhibit signals to disable
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        printf("    > Waiting for BATT%d_INHIBIT to deactivate\n", p+1);
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            return false;
        }
    }

    // Go into the drive state
    printf("    > Moving to drive state\n");
    set_ignition_state(true);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to idle in time\n");
        return false;
    }

    // Execute test - set pack 1 to 0% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(0);
    printf("    > Setting all cell voltages to %dmV (0%% soc) for pack 1\n", newCellVoltage);
    battery->get_pack(0).set_all_cell_voltages(newCellVoltage);

    // Make sure neither pack is inhibited
    printf("    > Ensuring BATT_INHIBIT does not activate for either pack\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 5000) ) {
            printf("    > BATT%d_INHIBIT activated when it should not have\n", p+1);
            return false;
        }
    }

}

/*
--------------------------------------------------------------------------------
  When packs are imbalanced, and we go into drive mode, only high pack should be
  enabled. Low pack should remain inhibited.
*/
bool test_case_103_ignition_turned_on_when_battery_contactors_are_inhibited(Battery* battery) {
    printf("Running test [test_case_103_ignition_turned_on_when_battery_contactors_are_inhibited]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to idle in time\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->set_all_cell_voltages(battery->get_voltage_from_soc(50));
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            return false;
        }
    }

}