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

#include <stdio.h>

#include "include/statemachine.h"
#include "include/battery.h"
#include "include/led.h"


extern Battery battery;
extern Bms bms;
extern Shunt shunt;


/* 
 * ~~ Note 1 ~~
 *
 * If we're driving around with some of the packs inhibited, and we want to go
 * directly into charge mode, dealing with the contactors is too awkward. While
 * driving, the high pack(s) will be enabled and the low pack(s) will be disabled.
 * However, when charging we want that to be the other way around. There's no
 * clean way to do this. So lets not try. Just go into fault mode. The same is
 * the case if we're charging and want to go directly into drive mode.
 *
 * ~~ Note 2 ~~
 *
 * The inverter-controlled contactors are potentially open in the standby,
 * batteryEmtpy, and overTempFault states only. So we can only change the
 * inhibition of the battery contactors from either of these states.
 */


/*
 * State               : standby
 * Ignition            : off
 * Battery contactors  : open + inhibited / not inhibited
 * Inverter contactors : open
 * CHARGE_INHIBIT      : on / off
 * CHARGE_ENABLE       : off
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : off
 *
 * Notes:
 * - CHARGE_INHIBIT can be on in this state either because of full battery or 
 *   battery too hot.
 */
void state_standby(Event event) {
    // Safeties
    bms.disable_heater();
    bms.do_welding_checks();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            bms.enable_charge_inhibit("[S01] too cold to charge", R_TOO_COLD);
            break;
        case E_TEMPERATURE_OK:
            if ( ! battery.has_full_cell() ) {
                bms.disable_charge_inhibit("[S02] no longer too cold to charge");
            }
            break;
        case E_TOO_HOT:
            bms.enable_drive_inhibit("[S03] battery too hot", R_TOO_HOT);
            bms.enable_charge_inhibit("[S04] battery too hot", R_TOO_HOT);
            bms.set_state(&state_overTempFault, "battery too hot");
            break;
        case E_BATTERY_EMPTY:
            bms.enable_drive_inhibit("[S05] empty battery", R_BATTERY_EMPTY);
            bms.set_state(&state_batteryEmpty, "empty battery");
            break;
        case E_BATTERY_NOT_EMPTY:
            if ( ! battery.too_hot() ) {
                bms.disable_charge_inhibit("[S06] battery not full");
            }
            break;
        case E_BATTERY_FULL:
            bms.enable_charge_inhibit("[S07] full battery", R_BATTERY_FULL);
            break;
        case E_PACKS_IMBALANCED:
            /* The contactors are currently open. We don't want to allow the
             * contactors to close when the packs have different voltages. So we
             * inhibit the contactors on all packs here. When we switch into
             * another state we'll decide which contactors to allow to close
             * then. This will depend on which state we switch into. */
            battery.enable_inhibit_contactor_close();
            break;
        case E_PACKS_NOT_IMBALANCED:
            battery.disable_inhibit_contactor_close();
            break;
        case E_IGNITION_ON:
            /* If packs are imbalanced, decide which contactor(s) to allow to 
             * close. Since we're going into drive mode, we want to pick the
             * high pack(s). */
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_contactors_for_drive();
            }
            bms.set_state(&state_drive, "ignition turned on");
            break;
        case E_IGNITION_OFF:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_IGNITION_OFF while in standby state\n");
            break;
        case E_CHARGING_INITIATED:
            // deal with battery contactor inhibition as we're switching on the inverter contactors
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_contactors_for_charge();
            }
            // Drive away protection
            bms.enable_drive_inhibit("[S08] charge requested", R_CHARGING);
            /* If the batteries are not warm enough to be charged, turn on the
             * battery heater, and disallow charging until they're warm enough. */
            if ( battery.too_cold_to_charge() ) {
                bms.enable_heater();
                bms.enable_charge_inhibit("[S09] too cold to charge", R_TOO_COLD);
                bms.set_state(&state_batteryHeating, "charge requested, but too cold to charge");
                break;
            }
            bms.set_state(&state_charging, "charge requested");
            break;
        case E_CHARGING_TERMINATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_TERMINATED while in standby state\n");
            break;
        case E_MODULE_UNRESPONSIVE:
            bms.enable_charge_inhibit("[S10] dead module", R_MODULE_UNRESPONSIVE);
            bms.enable_drive_inhibit("[S11] dead module", R_MODULE_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead module");
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            bms.enable_charge_inhibit("[S12] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.enable_drive_inhibit("[S13] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead shunt");
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State               : drive
 * Ignition            : on
 * Battery contactors  : closed + inhibited / not inhibited
 * Inverter contactors : closed
 * CHARGE_INHIBIT      : on / off
 * CHARGE_ENABLE       : off
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : off
 * 
 * Notes:
 * - CHARGE_INHIBIT can be on in this state either because of full battery or 
 *   battery too hot.
 */
void state_drive(Event event) {
    // Safeties
    bms.disable_drive_inhibit("[D00] driving");
    bms.disable_heater();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            bms.enable_charge_inhibit("[D01] too cold to charge", R_TOO_COLD);
            break;
        case E_TEMPERATURE_OK:
            if ( ! battery.has_full_cell() ) {
                bms.disable_charge_inhibit("[D02] not too cold to charge");
            }
            break;
        case E_TOO_HOT:
            bms.enable_drive_inhibit("[D03] battery too hot", R_TOO_HOT);
            bms.enable_charge_inhibit("[D04] battery too hot", R_TOO_HOT);
            bms.set_state(&state_overTempFault, "battery too hot");
            break;
        case E_BATTERY_EMPTY:
            bms.enable_drive_inhibit("[D05] empty battery", R_BATTERY_EMPTY);
            bms.set_state(&state_batteryEmpty, "empty battery");
            break;
        case E_BATTERY_NOT_EMPTY:
            if ( ! battery.too_cold_to_charge() ) {
                bms.disable_charge_inhibit("[D06] battery not empty");
            }
            break;
        case E_BATTERY_FULL:
            bms.enable_charge_inhibit("[D07] full battery", R_BATTERY_FULL);
            break;
        case E_PACKS_IMBALANCED:
            /* FIXME Need to consider opening the contactors for the low pack
             * only here. Guard against scenario where a single cell goes bad
             * (fails closed or reverses)? */
            break;
        case E_PACKS_NOT_IMBALANCED:
            break;  // Valid event, but we don't need to do anything with it.
        case E_IGNITION_ON:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_IGNITION_ON while in drive state\n");
            break;
        case E_IGNITION_OFF:
            // If we're driving on a subset of packs, we need to inhibit all contactors again.
            if ( bms.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            bms.set_state(&state_standby, "ignition turned off");
            break;
        case E_CHARGING_INITIATED:
            // Drive away protection
            bms.enable_drive_inhibit("[D08] imbalanced packs", R_CHARGING);
            /* Cannot go straight from drive mode to charge mode when packs are
            * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() ) {
                bms.enable_charge_inhibit("[D09] imbalanced packs", R_ILLEGAL_STATE_TRANSITION);
                bms.set_illegal_state_transition();
                bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from drive to charge with imbalanced packs");
                break;
            }
            bms.set_state(&state_charging, "charge requested");
            break;
        case E_CHARGING_TERMINATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_TERMINATED while in drive state\n");
            break;
        case E_MODULE_UNRESPONSIVE:
            /* Disallow charging. Consider disallowing driving. */
            bms.enable_charge_inhibit("[D10] dead module", R_MODULE_UNRESPONSIVE);
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            /* Disallow charging. Consider disallowing driving. */
            bms.enable_charge_inhibit("[D11] dead shunt", R_SHUNT_UNRESPONSIVE);
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State               : batteryHeating
 * Ignition            : on / off
 * Battery contactors  : closed + inhibited / not inhibited
 * Inverter contactors : closed
 * CHARGE_INHIBIT      : on
 * CHARGE_ENABLE       : on
 * HEATER_ENABLE       : on
 * DRIVE_INHIBIT       : on
 * 
 * Notes:
 * - Inverter contactors are always closed in this state.
 *   
 */
void state_batteryHeating(Event event) {
    // Safeties
    bms.enable_charge_inhibit("[H00] battery heating", R_TOO_COLD);
    bms.enable_drive_inhibit("[H00] battery heating", R_CHARGING);
    bms.enable_heater();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_TEMPERATURE_OK:
            bms.disable_heater();
            bms.disable_charge_inhibit("[H01] battery warmed to minimum charging temperature");
            bms.set_state(&state_charging, "battery warmed to minimum charging temperature");
            break;
        case E_TOO_HOT:
            bms.disable_heater();
            bms.enable_charge_inhibit("[H02] battery too hot", R_TOO_HOT);
            bms.set_state(&state_overTempFault, "battery too hot");
            break;
        case E_BATTERY_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_NOT_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_FULL:
            bms.disable_heater();
            bms.set_state(&state_charging, "battery full");
            break;
        case E_PACKS_IMBALANCED:
            /* Current flow should be minimal. OK to open some contactors. */
            battery.disable_inhibit_contactors_for_charge();
            break;
        case E_PACKS_NOT_IMBALANCED:
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_contactor_close();
            }
            break;
        case E_IGNITION_ON:
            break;  // Valid event, but we don't need to do anything with it.
        case E_IGNITION_OFF:
            break;  // Valid event, but we don't need to do anything with it.
        case E_CHARGING_INITIATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_INITIATED while in batteryHeating state\n");
            break;
        case E_CHARGING_TERMINATED:
            // We're no longer seeking to charge. No need to continue heating.
            bms.disable_heater();
            /* Cannot go straight from charge mode to drive mode when packs are
             * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() && bms.ignition_is_on() ) {
                bms.set_illegal_state_transition();
                bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from charge to drive with imbalanced packs");
                break;
            }
            // Battery empty
            if ( battery.has_empty_cell() ) {
                bms.disable_charge_inhibit("[H03] charge terminated but battery still empty");
                bms.set_state(&state_batteryEmpty, "charge terminated but battery still empty");
                break;
            }
            // Drive mode
            if ( bms.ignition_is_on() ) {
                bms.disable_charge_inhibit("[H04] charing terminated + ignition on");
                bms.disable_drive_inhibit("[H05] charging terminated + ignition on");
                bms.set_state(&state_drive, "charging terminated + ignition on");
                break;
            }
            // Standby mode
            bms.disable_drive_inhibit("[H06] ignition off");
            bms.disable_charge_inhibit("[H07] ignition off");
            bms.set_state(&state_standby, "charging terminated");
            break;
        case E_MODULE_UNRESPONSIVE:
            bms.enable_charge_inhibit("[H08] dead module", R_MODULE_UNRESPONSIVE);
            bms.enable_drive_inhibit("[H09] dead module", R_MODULE_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead module");
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            bms.enable_charge_inhibit("[H10] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.enable_drive_inhibit("[H11] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead shunt");
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");

    }
}

/*
 * State               : charging
 * Ignition            : on / off
 * Battery contactors  : closed + inhibited / not inhibited
 * Inverter contactors : closed
 * CHARGE_INHIBIT      : on / off
 * CHARGE_ENABLE       : on
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : on
 */
void state_charging(Event event) {
    // Safeties
    bms.enable_drive_inhibit("[C00] charging", R_CHARGING);
    bms.disable_charge_inhibit("[C00] charging");
    bms.disable_heater();
    // Recalculate max charging current
    bms.update_max_charge_current();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            bms.enable_heater();
            bms.enable_charge_inhibit("[C01] too cold to charge", R_TOO_COLD);
            bms.set_state(&state_batteryHeating, "too cold to charge");
            break;
        case E_TEMPERATURE_OK:
            break;  // Valid event, but we don't need to do anything with it.
        case E_TOO_HOT:
            bms.enable_charge_inhibit("[C02] battery too hot", R_TOO_HOT);
            bms.set_state(&state_overTempFault, "battery too hot");
            break;
        case E_BATTERY_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_NOT_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_FULL:
            /* Tell the charger to stop, but don't switch out of this state
             * until we get a CHARGING_TERMINATED event. */
            bms.enable_charge_inhibit("[C03] full battery", R_BATTERY_FULL);
            break;
        case E_PACKS_IMBALANCED:
            battery.disable_inhibit_contactors_for_charge();
            break;
        case E_PACKS_NOT_IMBALANCED:
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_contactor_close();
            }
            break;
        case E_IGNITION_ON:
            break;  // Valid event, but we don't need to do anything with it.
        case E_IGNITION_OFF:
            break;  // Valid event, but we don't need to do anything with it.
        case E_CHARGING_INITIATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_INITIATED while in charging state\n");
            break;
        case E_CHARGING_TERMINATED:
            /* If battery is full (i.e., we've charged to 100%), reset kWh/Ah
             * counters on the ISA shunt. */
            if ( battery.has_full_cell() ) {
                bms.send_shunt_reset_message();
            }
            /* Cannot go straight from charge mode to drive mode when packs are
             * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() && bms.ignition_is_on() ) {
                bms.enable_charge_inhibit("[C04] imbalanced packs", R_ILLEGAL_STATE_TRANSITION);
                bms.set_illegal_state_transition();
                bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from charge to drive with imbalanced packs");
                break;
            }
            /* Did we start charging with an empty battery, but cancel the
             * charge before actually putting any energy into the battery? */
            if ( battery.has_empty_cell() ) {
                bms.set_state(&state_batteryEmpty, "charge terminated but battery still empty");
                break;
            }
            // If ignition is already on, switch directly to drive mode
            if ( bms.ignition_is_on() ) {
                bms.disable_drive_inhibit("[C05] charging terminated + ignition on");
                bms.set_state(&state_drive, "charging terminated + ignition on");
                break;
            }
            bms.disable_drive_inhibit("[C06] charging terminated");
            bms.set_state(&state_standby, "charging terminated");
            break;
        case E_MODULE_UNRESPONSIVE:
            bms.enable_charge_inhibit("[C07] dead module", R_MODULE_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead module");
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            bms.enable_charge_inhibit("[C08] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead shunt");
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in charging state\n");
    }
}


/*
 * State               : batteryEmpty
 * Ignition            : on / off
 * Battery contactors  : open / closed + inhibited / not inhibited
 * Inverter contactors : open
 * CHARGE_INHIBIT      : on / off
 * CHARGE_ENABLE       : off
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : on
 * 
 * Notes:
 * - Chaging the battery contactor inhibition is allowed in this state. It is
 *   dependent only on the ignition state. If the ignition is off, we can change
 *   the battery contactor inhibition. If the ignition is on, we cannot.
 * 
 */
void state_batteryEmpty(Event event) {
    // Safeties
    bms.enable_drive_inhibit("[E00] battery empty", R_BATTERY_EMPTY);
    bms.disable_heater();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            bms.enable_charge_inhibit("[E01] too cold to charge", R_TOO_COLD);
            break;
        case E_TEMPERATURE_OK:
            bms.disable_charge_inhibit("[E02] no longer too cold to charge");
            break;
        case E_TOO_HOT:
            bms.enable_charge_inhibit("[E03] battery too hot", R_TOO_HOT);
            bms.set_state(&state_overTempFault, "battery too hot");
            break;
        case E_BATTERY_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_NOT_EMPTY:
            bms.disable_drive_inhibit("[E04] battery not empty");
            if ( bms.ignition_is_on() ) {
                bms.set_state(&state_drive, "battery level rose");
                break;
            }
            if ( battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            bms.set_state(&state_standby, "battery level rose");
            break;
        case E_BATTERY_FULL:
            bms.disable_drive_inhibit("[E05] battery not empty");
            bms.enable_charge_inhibit("[E06] full battery", R_BATTERY_FULL);
            if ( bms.ignition_is_on() ) {
                bms.set_state(&state_drive, "battery full");
                break;
            }
            if ( battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            bms.set_state(&state_standby, "battery full");
            break;
        case E_PACKS_IMBALANCED:
            if ( ! bms.ignition_is_on() ) {
                battery.enable_inhibit_contactor_close();
            }
            break;
        case E_PACKS_NOT_IMBALANCED:
            if ( ! bms.ignition_is_on() ) {
                battery.disable_inhibit_contactor_close();
            }
            break;
        case E_IGNITION_ON:
            if ( battery.packs_are_imbalanced() ) {
                battery.disable_inhibit_contactors_for_drive();
            }
            break;
        case E_IGNITION_OFF:
            if ( battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            break;
        case E_CHARGING_INITIATED:
            if ( ! bms.ignition_is_on() && battery.packs_are_imbalanced() ) {
                battery.disable_inhibit_contactors_for_charge();
            }
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[E07] too cold to charge", R_TOO_COLD);
                bms.enable_heater();
                bms.set_state(&state_batteryHeating, "charge requested, but too cold to charge");
                break;
            }
            bms.set_state(&state_charging, "charge requested");
            break;
        case E_CHARGING_TERMINATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_TERMINATED while in batteryEmpty state\n");
            break;
        case E_MODULE_UNRESPONSIVE:
            bms.enable_charge_inhibit("[E08] dead module", R_MODULE_UNRESPONSIVE);
            bms.enable_drive_inhibit("[E09] dead module", R_MODULE_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead module");
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            bms.enable_charge_inhibit("[E10] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.enable_drive_inhibit("[E11] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead shunt");
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State               : overTempFault
 * Ignition            : on / off
 * Battery contactors  : open / closed + inhibited / not inhibited
 * Inverter contactors : open / closed
 * CHARGE_INHIBIT      : on / off
 * CHARGE_ENABLE       : on / off
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : on
 *
 * Notes:
 * 1. The only way to get out of this state is for the battery to cool down.
 * 2. If a charge is requested while in this state, we stay here and just
 *    disallow. We don't switch over to the charging state for safety reasons. 
 */
void state_overTempFault(Event event) {
    // Safeties
    bms.enable_drive_inhibit("[T00] battery too hot", R_TOO_HOT);
    bms.enable_charge_inhibit("[T00] battery too hot", R_TOO_HOT);
    bms.disable_heater();
    bms.update_max_charge_current();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            /* If we're too cold to charge, then we cannot be too hot any more */
            bms.enable_charge_inhibit("[T01] too cold to charge", R_TOO_COLD);
            if ( bms.charge_is_enabled() ) {
                bms.set_state(&state_batteryHeating, "no longer too hot");
                break;
            }
            if ( bms.ignition_is_on() ) {
                bms.set_state(&state_drive, "no longer too hot");
                break;
            }
            if ( battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            bms.set_state(&state_standby, "no longer too hot");
            break;
        case E_TEMPERATURE_OK:
            // Charge mode overrides drive mode
            if ( bms.charge_is_enabled() ) {
                bms.disable_charge_inhibit("[T02] battery has cooled");
                bms.set_state(&state_charging, "battery has cooled");
                break;
            }
            // Drive mode
            if ( bms.ignition_is_on() ) {
                bms.disable_charge_inhibit("[T03] battery has cooled");
                bms.disable_drive_inhibit("[T04] battery has cooled");
                bms.set_state(&state_drive, "battery has cooled");
                break;
            }
            // Standby mode
            bms.disable_drive_inhibit("[T05] battery has cooled");
            bms.disable_charge_inhibit("[T06] battery has cooled");
            if ( battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            bms.set_state(&state_standby, "battery has cooled");
            break;
        case E_TOO_HOT:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_NOT_EMPTY:
            break;  // Valid event, but we don't need to do anything with it.
        case E_BATTERY_FULL:
            break; // Valid event, but we don't need to do anything with it.
        case E_PACKS_IMBALANCED:
            if ( ! bms.ignition_is_on() && ! bms.charge_is_enabled() ) {
                battery.enable_inhibit_contactor_close();
            }
            break;
        case E_PACKS_NOT_IMBALANCED:
            if ( ! bms.ignition_is_on() && ! bms.charge_is_enabled() ) {
                battery.disable_inhibit_contactor_close();
            }
            break;
        case E_IGNITION_ON:
            if ( ! bms.charge_is_enabled() && battery.packs_are_imbalanced() ) {
                battery.disable_inhibit_contactors_for_drive();
            }
            break;
        case E_IGNITION_OFF:
            if ( ! bms.charge_is_enabled() && battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            break;
        case E_CHARGING_INITIATED:
            if ( ! bms.ignition_is_on() && battery.packs_are_imbalanced() ) {
                battery.disable_inhibit_contactors_for_charge();
            }
            break;
        case E_CHARGING_TERMINATED:
            if ( ! bms.ignition_is_on() && battery.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            }
            break;
        case E_MODULE_UNRESPONSIVE:
            if (! bms.ignition_is_on() && ! bms.charge_is_enabled() ) {
                battery.disable_inhibit_contactor_close();
            }
            bms.set_state(&state_criticalFault, "dead module");
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            if (! bms.ignition_is_on() && ! bms.charge_is_enabled() ) {
                battery.disable_inhibit_contactor_close();
            }
            bms.set_state(&state_criticalFault, "dead shunt");
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}


/*
 * State               : illegalStateTransitionFault
 * Ignition            : on / off
 * Battery contactors  : open / closed + inhibited / not inhibited
 * Inverter contactors : closed
 * CHARGE_INHIBIT      : on
 * CHARGE_ENABLE       : on / off
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : on
 *
 * Reasons we can be in this state:
 *   - We tried to go straight from drive to charge with imbalanced packs
 */
void state_illegalStateTransitionFault(Event event) {
    // Safeties
    bms.enable_drive_inhibit("[I00] illegal state transition", R_ILLEGAL_STATE_TRANSITION);
    bms.enable_charge_inhibit("[I00] illegal state transition", R_ILLEGAL_STATE_TRANSITION);
    bms.disable_heater();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            break;
        case E_TEMPERATURE_OK:
            break;
        case E_TOO_HOT:
            break;
        case E_BATTERY_EMPTY:
            break;
        case E_BATTERY_NOT_EMPTY:
            break;
        case E_BATTERY_FULL:
            break;
        case E_PACKS_IMBALANCED:
            break;
        case E_PACKS_NOT_IMBALANCED:
            break;
        case E_IGNITION_ON:
            break;
        case E_IGNITION_OFF:
            if ( ! bms.charge_is_enabled() ) {
                bms.clear_illegal_state_transition();
                bms.set_state(&state_standby, "ignition and charging off");
            }
            break;
        case E_CHARGING_INITIATED:
            break;
        case E_CHARGING_TERMINATED:
            if ( ! bms.ignition_is_on() ) {
                bms.clear_illegal_state_transition();
                bms.set_state(&state_standby, "ignition and charging off");
            }
            break;
        case E_MODULE_UNRESPONSIVE:
            bms.enable_charge_inhibit("[I01] dead module", R_MODULE_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead module");
            break;
        case E_MODULES_ALL_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        case E_SHUNT_UNRESPONSIVE:
            bms.enable_charge_inhibit("[I02] dead shunt", R_SHUNT_UNRESPONSIVE);
            bms.set_state(&state_criticalFault, "dead shunt");
            break;
        case E_SHUNT_RESPONSIVE:
            break;  // Valid event, but we don't need to do anything with it.
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State               : criticalFault
 * Ignition            : on / off
 * Battery contactors  : open / closed + inhibited / not inhibited
 * Inverter contactors : open / closed
 * CHARGE_INHIBIT      : on
 * CHARGE_ENABLE       : on / off
 * HEATER_ENABLE       : off
 * DRIVE_INHIBIT       : on
 * 
 * Notes:
 * - The only way to exit this state is for the shunt and all modules to be
 *   responsive again.
 */
//------------------------------------------------------------------------------
void state_criticalFault(Event event) {
    // Safeties
    bms.enable_drive_inhibit("[T00] critical fault", R_CRITICAL_FAULT);
    bms.enable_charge_inhibit("[T00] critical fault", R_CRITICAL_FAULT);
    bms.disable_heater();

    switch (event) {
        case E_TOO_COLD_TO_CHARGE:
            break;
        case E_TEMPERATURE_OK:
            break;
        case E_TOO_HOT:
            break;
        case E_BATTERY_EMPTY:
            break;
        case E_BATTERY_NOT_EMPTY:
            break;
        case E_BATTERY_FULL:
            break;
        case E_PACKS_IMBALANCED:
            break;
        case E_PACKS_NOT_IMBALANCED:
            break;
        case E_IGNITION_ON:
            break;
        case E_IGNITION_OFF:
            break;
        case E_CHARGING_INITIATED:
            break;
        case E_CHARGING_TERMINATED:
            break;
        case E_MODULE_UNRESPONSIVE:
            break;
        case E_MODULES_ALL_RESPONSIVE:
            if ( ! shunt.is_dead() ) {
                if ( bms.charge_is_enabled() ) {
                    bms.disable_charge_inhibit("[CF01] critical fault cleared");
                    bms.set_state(&state_charging, "critical fault cleared");
                    break;
                }
                if ( bms.ignition_is_on() ) {
                    bms.set_state(&state_drive, "critical fault cleared");
                    break;
                }
                if ( battery.packs_are_imbalanced() ) {
                    battery.enable_inhibit_contactor_close();
                }
                bms.set_state(&state_standby, "critical fault cleared");
            }
            break;
        case E_SHUNT_UNRESPONSIVE:
            break;
        case E_SHUNT_RESPONSIVE:
            if ( battery.is_alive() ) {
                if ( bms.charge_is_enabled() ) {
                    bms.disable_charge_inhibit("[CF02] critical fault cleared");
                    bms.set_state(&state_charging, "critical fault cleared");
                    break;
                }
                if ( bms.ignition_is_on() ) {
                    bms.set_state(&state_drive, "critical fault cleared");
                    break;
                }
                if ( battery.packs_are_imbalanced() ) {
                    battery.enable_inhibit_contactor_close();
                }
                bms.set_state(&state_standby, "critical fault cleared");
            }
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in criticalFault state\n");
    }
}

// Mapping between state functions and their names

typedef struct stateName {
    State state;
    const char * stateName;
} stateName;

struct stateName stateNames[8] = {
    {state_standby, "standby"},
    {state_drive, "drive"},
    {state_batteryHeating, "batteryHeating"},
    {state_charging, "charging"},
    {state_batteryEmpty, "batteryEmpty"},
    {state_overTempFault, "overTempFault"},
    {state_illegalStateTransitionFault, "illegalStateTransistionFault"},
    {state_criticalFault, "criticalFault"}
};

// Return the name of the current state
const char* get_state_name(State state) {
    for ( int i=0; i < 8; i++ ) {
        if ( state == stateNames[i].state ) {
            return stateNames[i].stateName;
        }
    }
    return "unknownState";
}
