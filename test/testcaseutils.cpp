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
#include <string>
#include "include/util.h"
#include "include/bms.h"
#include "include/battery.h"
#include "include/io.h"

void transition_to_standby_state(Bms* bms) {
    // Set SoC to 50%
    uint16_t newCellVoltage = bms->get_battery()->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dmV (approx 50%% soc)\n", newCellVoltage);
    bms->get_battery()->set_all_cell_voltages(newCellVoltage);
    // Turn off ignition and charging
    printf("    > Setting ignition and charge enable to false\n");
    set_ignition_state(false);
    set_charge_enable_state(false);
    // Ensure DRIVE_INHIBIT is disabled
    printf("    > Waiting for DRIVE_INHIBIT to deactivate\n");
    if ( ! wait_for_drive_inhibit_state(bms, false, 2000) ) {
        printf("    > DRIVE_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    } else {
        printf("    > DRIVE_INHIBIT deactivated\n");
    }
    // Set temperature to something normal
    printf("    > Setting all temperatures to 20C\n");
    bms->get_battery()->set_all_temperatures(20);
    wait_for_bms_state(bms, STATE_BATTERY_STANDBY, 2000);
}

void transition_to_drive_state(Bms* bms) {
    // Set SoC to 50%
    uint16_t newCellVoltage = bms->get_battery()->get_voltage_from_soc(50);
    printf("    > Setting all cell voltages to %dmV (approx 50%% soc)\n", newCellVoltage);
    bms->get_battery()->set_all_cell_voltages(newCellVoltage);
    // Turn on ignition
    printf("    > Turining on ignition\n");
    set_ignition_state(true);
    printf("    > Turning off charge\n");
    set_charge_enable_state(false);
    // Ensure DRIVE_INHIBIT is disabled
    printf("    > Waiting for DRIVE_INHIBIT to deactivate\n");
    if ( ! wait_for_drive_inhibit_state(bms, false, 2000) ) {
        printf("    > DRIVE_INHIBIT did not deactivate in time\n");
        printf("    > Test FAILED\n");
        return false;
    } else {
        printf("    > DRIVE_INHIBIT deactivated\n");
    }
    // Set temperature to something normal
    printf("    > Setting all temperatures to 20C\n");
    bms->get_battery()->set_all_temperatures(20);
    wait_for_bms_state(bms, STATE_BATTERY_DRIVE, 2000);
}

bool wait_for_soc(Bms* bms, int soc, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_soc() < soc) {
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_drive_inhibit_state(Bms* bms, bool state, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_inhibitDrive() != state) {
        std::string currentDriveState = bms->get_inhibitDrive() ? "true" : "false";
        std::string currentChargeState = bms->get_inhibitCharge() ? "true" : "false";
        if ( get_clock() % 1000 == 0 ){
            printf("    * Drive inhibit state: %s\n", currentDriveState.c_str());
            printf("    * Charge inhibit state: %s\n", currentChargeState.c_str());
        }
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_charge_inhibit_state(Bms* bms, bool state, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_inhibitCharge() != state) {
        std::string currentDriveState = bms->get_inhibitDrive() ? "true" : "false";
        std::string currentChargeState = bms->get_inhibitCharge() ? "true" : "false";
        if ( get_clock() % 1000 == 0 ){
            printf("    * Drive inhibit state: %s\n", currentDriveState.c_str());
            printf("    * Charge inhibit state: %s\n", currentChargeState.c_str());
        }
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_bms_state(Bms* bms, BmsState state, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_state() != state) {
        //printf("Comparing %d to %d\n", bms->get_state(), state);
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_batt_inhibit_state(Battery* battery, int packId, bool state, int timeout) {
    clock_t startTime = get_clock();
    while (battery->get_pack(packId)->get_inhibit() != state) {
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_packs_imbalanced_state(Bms* bms, bool state, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_packsImbalanced() != state) {
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}

bool wait_for_heater_enable_state(Bms* bms, bool state, int timeout) {
    clock_t startTime = get_clock();
    while (bms->get_heaterEnabled() != state) {
        if (get_clock() - startTime > timeout) {
            return false;
        }
    }
    return true;
}