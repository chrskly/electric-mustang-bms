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
 * Test cases relating to contactor control
 */

/*
 * Test case 101
 * -----------------------------------------------------------------------------
 * Description: When in standby state, and the pack voltages differ, the BMS
 *              should inhibit the battery contactor close.
 * Preconditions
 *   1. BMS state == STANDBY
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 *   4. batt1 inhibit off
 *   5. batt2 inhibit off
 * Actions:
 *   1. Set pack 1 to 25% soc
 * Postconditions:
 *   1. BMS state == STANDBY
 *   1. batt1 inhibit on
 *   2. batt2 inhibit on
 *   3. packsImbalanced flag set in BMS CAN messages
*/
bool test_case_101(Battery* battery, Bms* bms) {
    printf("Running test [test_case_101] : inhibit battery contactor close when pack voltages differ, from standby state\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // Execute test - set pack 1 to 25% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(25);
    printf("    > Setting all cell voltages to %dmV (25%% soc) for pack 1\n", newCellVoltage);
    battery->get_pack(0)->set_all_cell_voltages(newCellVoltage);

    // Make sure both packs are inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 5000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Make sure CAN messages show the imbalanced state
    if ( ! wait_for_packs_imbalanced_state(bms, true, 2000) ) { // BmsState, 2 second timeout
        printf("    > BMS did not flag the packsImbalanced state in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;
}

/*
 * Test case 102
 * -----------------------------------------------------------------------------
 * Description: When in drive state, and the pack voltages differ, the BMS 
 *              should not inhibit the battery contactor close. I.e., don't open
 *              the contactors when driving.
 * Preconditions
 *   1. BMS state == DRIVE
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 *   4. batt1 inhibit off
 *   5. batt2 inhibit off
 * Actions:
 *   1. Set batt1 to 25% soc
 * Postconditions:
 *   1. BMS state == DRIVE
 *   2. batt1 inhibit off
 *   3. batt2 inhibit off
 *   4. packsImbalanced flag set in BMS CAN messages
*/
bool test_case_102(Battery* battery, Bms* bms) {
    printf("Running test [test_case_102] : do not inhibit battery contactor close when pack voltages differ and ignition is on\n");

    if ( ! transition_to_drive_state(bms) ) { 
        return false;
    }

    // Execute test - set pack 1 to 25% soc
    uint16_t newCellVoltage = battery->get_voltage_from_soc(25);
    printf("    > Setting all cell voltages to %dmV (25%% soc) for pack 1\n", newCellVoltage);
    battery->get_pack(0)->set_all_cell_voltages(newCellVoltage);

    // Make sure neither pack is inhibited
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        printf("    > Waiting 5s to ensure BATT%d_INHIBIT does not activate\n", p+1);
        if ( wait_for_batt_inhibit_state(battery, p, true, 5000) ) {
            printf("    > BATT%d_INHIBIT activated when it should not have\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Make sure CAN messages show the imbalanced state
    if ( ! wait_for_packs_imbalanced_state(bms, true, 2000) ) { // BmsState, 2 second timeout
        printf("    > BMS did not flag the packsImbalanced state in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 103
 * -----------------------------------------------------------------------------
 * Description: When packs are imbalanced, and we go into drive mode from
 *              standby, only high pack should be enabled. Low pack should
 *              remain inhibited.
 * Preconditions
 *   1. BMS state == STANDBY
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 *   4. batt1 inhibit on
 *   5. batt2 inhibit on
 * Actions:
 *   1. Turn ignition on
 * Postconditions:
 *   1. BMS state == DRIVE
 *   1. batt1 inhibit on
 *   2. batt2 inhibit off
 *   3. packsImbalanced flag set in BMS CAN messages
 */
bool test_case_103(Battery* battery, Bms* bms) {
    printf("Running test [test_case_103] : ignition turned on when battery contactors are inhibited\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Turn ignition on
    printf("    > Turning ignition on\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should still be inhibitited and pack 1 should not
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}


/*
 * Test case 104
 * -----------------------------------------------------------------------------
 * Description: When packs are imbalanced, and we go into standby mode from
 *              drive mode, both packs should be inhibited.
 * Preconditions
 *   1. BMS state == DRIVE
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 *   4. batt1 inhibit off
 *   5. batt2 inhibit off
 * Actions:
 *   1. Turn ignition off
 * Postconditions:
 *   1. BMS state == STANDBY
 *   2. batt1 inhibit on
 *   3. batt2 inhibit on
 *   4. packsImbalanced flag set in BMS CAN messages
 */
bool test_case_104(Battery* battery, Bms* bms) {
    printf("Running test [test_case_104] : ignition turned off when battery contactors are inhibited\n");

    if ( ! transition_to_standby_state(bms) ) {
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Turn ignition on
    printf("    > Turning ignition on\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should still be inhibitited and pack 1 should not
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Turn ignition off
    printf("    > Turning ignition off\n");
    set_ignition_state(false);
    if ( ! wait_for_bms_state(bms, STATE_STANDBY, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 105
 * -----------------------------------------------------------------------------
 * Description: When packs are imbalanced, and we go into charging mode from
 *              standby mode, only the low pack should be enabled. High pack
 *              should remain inhibited.
 * Preconditions
 *   1. BMS state == STANDBY
 *   2. DRIVE_INHIBIT signal is inactive
 *   3. Temperature is normal
 *   4. batt1 inhibit on
 *   5. batt2 inhibit on
 * Actions:
 *   1. Start charging
 * Postconditions:
 *   1. batt1 inhibit off
 *   2. batt2 inhibit on
 *   3. packsImbalanced flag set in BMS CAN messages
 *   4. BMS state == CHARGING
 *   5. DRIVE_INHIBIT signal is active
 */
bool test_case_105(Battery* battery, Bms* bms) {
    printf("Running test [test_case_105] : start charging when battery contactors are inhibited\n");

    transition_to_standby_state(bms);

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should NOT be inhibitited (low pack)
    // Pack 1 should be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, false, 5000) ) {
        printf("    > BATT1_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is activated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, true, 5000) ) {
        printf("    > BATT2_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 106
 * -----------------------------------------------------------------------------
 * Description: When packs are imbalanced, and we stop charging, both packs
 *              should be inhibited.
 * Preconditions:
 *   1. BMS state == CHARGING
 *   2. Packs are imbalanced
 * Actions:
 *   1. Stop charging
 * Postconditions:
 *   1. batt1 inhibit on
 *   2. batt2 inhibit on
 */
bool test_case_106(Battery* battery, Bms* bms) {
    printf("Running test [test_case_106] : stop charging when battery contactors are inhibited\n");

    transition_to_standby_state(bms);

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should NOT be inhibitited (low pack)
    // Pack 1 should be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, false, 2000) ) {
        printf("    > BATT1_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is activated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, true, 2000) ) {
        printf("    > BATT2_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Stop charging
    printf("    > Stop charging\n");
    set_charge_enable_state(false);
    if ( ! wait_for_bms_state(bms, STATE_STANDBY, 2000) ) {
        printf("    > BMS state did not change to 'idle' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    printf("    > Test PASSED\n");
    return true;

}


/*
 * Test case 107
 * -----------------------------------------------------------------------------
 * Description: When we are charging on imbalanced packs, and the voltages
 *              equalise, both packs should be uninhibited.
 * Preconditions:
 *   1. BMS state == CHARGING
 *   2. Packs are imbalanced
 * Actions:
 *   1. Set voltages of both packs to be equal
 * Postconditions:
 *   1. batt1 inhibit off
 *   2. batt2 inhibit off
 */
bool test_case_107(Battery* battery, Bms* bms) {
    printf("Running test [test_case_107] : charging on one pack and voltage equalises\n");

    transition_to_standby_state(bms);

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should NOT be inhibitited (low pack)
    // Pack 1 should be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, false, 2000) ) {
        printf("    > BATT1_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is activated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, true, 2000) ) {
        printf("    > BATT2_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Set pack 0 to 50% soc
    printf("    > Setting pack 0 to 50%% soc\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be uninhibited
    printf("    > Waiting for BATT_INHIBIT to deactivate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    printf("    > Test PASSED\n");
    return true;

}


/*
 * Test case 108
 * -----------------------------------------------------------------------------
 * Description: When we are driving on imbalanced packs, and the voltages
 *              equalise, both packs should be uninhibited.
 * Preconditions:
 *   1. BMS state == DRIVE
 *   2. Packs are imbalanced
 * Actions:
 *   1. Set voltages of both packs to be equal
 * Postconditions:
 *   1. batt1 inhibit off
 *   2. batt2 inhibit off
 */
bool test_case_108(Battery* battery, Bms* bms) {
    printf("Running test [test_case_108] : driving on one pack and voltage equalises\n");

    transition_to_standby_state(bms);

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Start driving
    printf("    > Start driving\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should be inhibitited (low pack)
    // Pack 1 should NOT be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Set pack 1 to 25% soc
    printf("    > Setting pack 1 to 25%% soc\n");
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(25));

    // Both packs should be uninhibited
    printf("    > Waiting for BATT_INHIBIT to deactivate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    printf("    > Test PASSED\n");
    return true;

}

/*
 * Test case 109
 * -----------------------------------------------------------------------------
 * Description: When we are driving on imbalanced packs, and we start charging,
 *              the BMS should inhibit the charging and driving and go into an
 *              error state (illegal state transition fault).
 * Preconditions:
 *   1. BMS state == DRIVE
 *   2. Packs are imbalanced
 * Actions:
 *   1. Start charging
 * Postconditions:
 *   1. DRIVE_INHIBIT signal is active
 *   2. CHARGE_INHIBIT signal is active
 *   3. BMS state == ILLEGAL_STATE_TRANSITION_FAULT
 */
bool test_case_109(Battery* battery, Bms* bms) {
    printf("Running test [test_case_109] : driving on one pack then begin charging while ignition still on\n");

    transition_to_standby_state(bms);

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Start driving
    printf("    > Turn ignition on\n");
    set_ignition_state(true);
    if ( ! wait_for_bms_state(bms, STATE_DRIVE, 2000) ) {
        printf("    > BMS state did not change to 'drive' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Pack 0 should be inhibitited (low pack)
    // Pack 1 should NOT be inhibitied (high pack)
    printf("    > Ensuring BATT_INHIBIT is activated for pack 0\n");
    if ( ! wait_for_batt_inhibit_state(battery, 0, true, 2000) ) {
        printf("    > BATT1_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Ensuring BATT_INHIBIT is deactivated for pack 1\n");
    if ( ! wait_for_batt_inhibit_state(battery, 1, false, 2000) ) {
        printf("    > BATT2_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // CHARGE_INHIBIT should activate
    printf("    > Ensuring CHARGE_INHIBIT is activated\n");
    if ( ! wait_for_charge_inhibit_state(bms, true, 2000) ) {
        printf("    > CHARGE_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // DRIVE_INHIBIT should activate
    printf("    > Ensuring DRIVE_INHIBIT is activated\n");
    if ( ! wait_for_drive_inhibit_state(bms, true, 2000) ) {
        printf("    > DRIVE_INHIBIT did not activate in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // BMS should go into error state
    printf("    > Ensuring BMS goes into illegalStateTransitionFault state\n");
    if ( ! wait_for_bms_state(bms, STATE_ILLEGAL_STATE_TRANSITION_FAULT, 2000) ) {
        printf("    > BMS state did not change to 'illegalStateTransitionFault' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    printf("    > Test PASSED\n");
    return true;

}
/*
 * Test case 110
 * -----------------------------------------------------------------------------
 * Description: When in standby state, with imabalanced packs, and the voltages
 *              equalise, the BMS should disable the contactor inhibition.
 * Preconditions:
 *   1. BMS state == STANDBY
 *   2. Packs are imbalanced
 * Actions:
 *   1. Set voltages of both packs to be equal
 * Postconditions:
 *   1. batt1 inhibit off
 *   2. batt2 inhibit off
 */
bool test_case_110(Battery* battery, Bms* bms) {
    printf("Running test [test_case_110] : imbalanced packs equalise while in standby\n");

    transition_to_standby_state(bms);

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be inhibited
    printf("    > Waiting for BATT_INHIBIT to activate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, true, 2000) ) {
            printf("    > BATT%d_INHIBIT did not activate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    // Set pack 0 to 50% soc
    printf("    > Setting pack 0 to 50%% soc\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should be uninhibited
    printf("    > Waiting for BATT_INHIBIT to deactivate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    printf("    > Test PASSED\n");
    return true;
}

/*
 * Test case 111
 * -----------------------------------------------------------------------------
 * Description: When charging, and packs go into imbalanced state, the BMS
 *              should not inhibit contactor close. I.e., don't open the
 *              contactors when charging.
 */
bool test_case_111(Battery* battery, Bms* bms) {
    printf("Running test [test_case_111] : do not inhibit battery contactor close when pack voltages differ and charging\n");

    transition_to_standby_state(bms);

    // Start charging
    printf("    > Start charging\n");
    set_charge_enable_state(true);
    if ( ! wait_for_bms_state(bms, STATE_CHARGING, 2000) ) {
        printf("    > BMS state did not change to 'charging' in time\n");
        printf("    > Test FAILED\n");
        return false;
    }

    // Set packs imbalanced (pack 0 low, pack 1 high)
    printf("    > Setting packs imbalanced\n");
    battery->get_pack(0)->set_all_cell_voltages(battery->get_voltage_from_soc(25));
    battery->get_pack(1)->set_all_cell_voltages(battery->get_voltage_from_soc(50));

    // Both packs should NOT be inhibited
    printf("    > Waiting for BATT_INHIBIT to deactivate on both packs\n");
    for ( int p = 0; p < battery->get_num_packs(); p++ ) {
        if ( ! wait_for_batt_inhibit_state(battery, p, false, 2000) ) {
            printf("    > BATT%d_INHIBIT did not deactivate in time\n", p+1);
            printf("    > Test FAILED\n");
            return false;
        }
    }

    printf("    > Test PASSED\n");
    return true;
}