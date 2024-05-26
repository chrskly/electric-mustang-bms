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
        printf("    > Test failed\n");
        return false;
    }

    // Ensure no delta between packs by setting all cell voltages to 50% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dmV (approx 50%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Wait for all battery inhibit signals to disable
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        printf("    > Waiting for BATT%d_INHIBIT to deactivate\n", p+1);
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Execute test - set pack 1 to 0% soc
    newCellVoltage = battery->get_voltage_from_soc(0);
    printf("    > Setting all cell voltages to %dmV (0%% soc) for pack 1\n", newCellVoltage);
    battery->get_pack(0).set_all_cell_voltages(newCellVoltage);

    // Make sure both packs are inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Make sure CAN messages show the imbalanced state
    if ( ! wait_for_packs_imbalanced_state(bms, true, 2000) ) { // BmsState, 2 second timeout
        printf("    > BMS did not flag the packsImbalanced state in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Test passed\n");
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
    printf("    > Setting all cell voltages to %dmV (approx 50%% soc)\n", newCellVoltage);
    battery->set_all_cell_voltages(newCellVoltage);

    // Wait for all battery inhibit signals to disable
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        printf("    > Waiting for BATT%d_INHIBIT to deactivate\n", p+1);
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Go into the drive state
    printf("    > Moving to drive state\n");
    set_ignition_state(true);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to idle in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Execute test - set pack 1 to 0% soc
    newCellVoltage = battery->get_voltage_from_soc(0);
    printf("    > Setting all cell voltages to %dmV (0%% soc) for pack 1\n", newCellVoltage);
    battery->get_pack(0).set_all_cell_voltages(newCellVoltage);

    // Make sure neither pack is inhibited
    printf("    > Ensuring BATT_INHIBIT does not activate for either pack\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 5000) ) {
            printf("    > BATT%d_INHIBIT activated when it should not have\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    printf("    > Test passed\n");
    return true;

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
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    printf("    > Test passed\n");
    return true;

}


bool test_case_104_ignition_turned_off_when_battery_contactors_are_inhibited(Battery* battery) {
    printf("Running test [test_case_104_Ignition turned off when battery contactors are inhibited]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Turn ignition on
    printf("    > Turning ignition on\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Pack 0 should still be inhibitited and pack 1 should not
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Turn ignition off
    printf("    > Turning ignition off\n");
    set_ignition_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    printf("    > Test passed\n");
    return true;

}

bool test_case_105_start_charging_when_battery_contactors_are_inhibited(Battery* battery) {
    printf("Running test [test_case_105_start_charging_when_battery_contactors_are_inhibited]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Pack 0 should NOT be inhibitited (low pack)
    // Pack 1 should be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, false, 2000) ) {
        printf("    > BATT1_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is activated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, true, 2000) ) {
        printf("    > BATT2_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Test passed\n");
    return true;

}

bool test_case_106_stop_charging_when_battery_contactors_are_inhibited(Battery* battery) {
    printf("Running test [test_case_106_stop_charging_when_battery_contactors_are_inhibited]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Pack 0 should NOT be inhibitited (low pack)
    // Pack 1 should be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, false, 2000) ) {
        printf("    > BATT1_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is activated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, true, 2000) ) {
        printf("    > BATT2_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Stop charging
    printf("    > Stop charging\n");
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    printf("    > Test passed\n");
    return true;

}


bool test_case_107_charging_on_one_pack_and_voltage_equalises(Battery* battery) {
    printf("Running test [test_case_107_charging_on_one_pack_and_voltage_equalises]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Pack 0 should NOT be inhibitited (low pack)
    // Pack 1 should be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, false, 2000) ) {
        printf("    > BATT1_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is activated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, true, 2000) ) {
        printf("    > BATT2_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set pack 0 to 50% soc
    printf("    > Setting pack 0 to 50%% soc\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be uninhibited
    printf("    > Waiting for BATT_INHIBIT to deactivate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    printf("    > Test passed\n");
    return true;

}


bool test_case_108_driving_on_one_pack_and_voltage_equalises(Battery* battery) {
    printf("Running test [test_case_108_driving_on_one_pack_and_voltage_equalises]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Start driving
    printf("    > Start driving\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Pack 0 should be inhibitited (low pack)
    // Pack 1 should NOT be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set pack 1 to 25% soc
    printf("    > Setting pack 1 to 25%% soc\n");
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(25));

    // Both packs should be uninhibited
    printf("    > Waiting for BATT_INHIBIT to deactivate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    printf("    > Test passed\n");
    return true;

}

bool test_case_109_driving_on_one_pack_then_begin_charging_while_ignition_still_on(Battery* battery) {
    printf("Running test [test_case_109_driving_on_one_pack_then_begin_charging_while_ignition_still_on]\n");
    Bms* bms = battery->get_bms();

    // Start in idle state
    printf("    > Moving to idle state\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_IDLE, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0).set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1).set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test failed\n");
            return false;
        }
    }

    // Start driving
    printf("    > Turn ignition on\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Pack 0 should be inhibitited (low pack)
    // Pack 1 should NOT be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // CHARGE_INHIBIT should activate
    printf("    > Ensuring CHARGE_INHIBIT is activated\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // DRIVE_INHIBIT should activate
    printf("    > Ensuring DRIVE_INHIBIT is activated\n");
    if ( ! wait_for_drive_inhibit_state(bms, true, 2000) ) {
        printf("    > DRIVE_INHIBIT did not activate in time\n");
        printf("    > Test failed\n");
        return false;
    }

    // BMS should go into error state
    printf("    > Ensuring BMS goes into illegalStateTransitionFault state\n");
    if ( ! wait_for_bms_state(bms, STATE_ILLEGAL_STATE_TRANSITION_FAULT, 2000) ) {
        printf("    > BMS state did not change to 'illegalStateTransitionFault' in time\n");
        printf("    > Test failed\n");
        return false;
    }

    printf("    > Test passed\n");
    return true;

}