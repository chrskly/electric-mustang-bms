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


/* 
 * ~~ Note 1 ~~
 *
 * If we're driving around with some of the packs inhibited, and we want to go
 * directly into charge mode, dealing with the contactors is too awkward. While
 * driving, the high pack(s) will be enabled and the low pack(s) will be disabled.
 * However, when charging we want that to be the other way around. There's no
 * clean way to do this. So lets not try. Just go into fault mode. The same is
 * the case if we're charging and want to go directly into drive mode.
 */


/*
 * State          : standby
 * Ignition       : off
 * Contactors     : open
 * Charging       : no
 * heater         : off
 * drive inhibit  : off
 * charge inhibit : off
 */
void state_standby(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            // We're not charging right now, but flag that it's too cold for later reference
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[S01] too cold to charge");
            } else if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[S02] full cell");
            } else {
                bms.disable_charge_inhibit("[S04] not too cold to charge and no full cell");
            }     
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                bms.enable_drive_inhibit("[S05] battery too hot");
                bms.enable_charge_inhibit("[S06] battery too hot");
                bms.set_state(&state_overTempFault, "battery too hot");
                break;
            }
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[S07] too cold to charge");
            } else if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[S08] full cell");
            } else {
                bms.disable_charge_inhibit("[S09] not too cold to charge and no full cell");
            }
            /* The contactors are currently open. We don't want to allow the
             * contactors to close when the packs have different voltages. So we
             * inhibit the contactors on all packs here. When we switch into
             * another state we'll decide which contactors to allow to close
             * then. This will depend on which state we switch into. */
            if ( bms.packs_are_imbalanced() ) {
                battery.enable_inhibit_contactor_close();
            } else {
                battery.disable_inhibit_contactor_close();
            }
            // Battery is empty. Disallow driving.
            if ( battery.has_empty_cell() ) {
                bms.enable_drive_inhibit("[S10] empty battery");
                bms.set_state(&state_batteryEmpty, "empty battery");
                break;
            }
            break;
        case E_IGNITION_ON:
            /* Packs are imbalanced. Decide which contactor(s) to allow to 
             * close. Since we're going into drive mode, we want to pick the
             * high pack(s). */
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_for_drive();
            }
            bms.set_state(&state_drive, "ignition turned on");
            break;
        case E_IGNITION_OFF:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_IGNITION_OFF while in standby state\n");
            break;
        case E_CHARGING_INITIATED:
            /* If the batteries are not warm enough to be charged, turn on the
             * battery heater, and disallow charging until they're warm enough. */
            if ( battery.too_cold_to_charge() ) {
                bms.enable_heater();
                bms.enable_charge_inhibit("[S11] too cold to charge");
                bms.enable_drive_inhibit("[S12] charge requested");
                bms.set_state(&state_batteryHeating, "charge requested, but too cold to charge");
                break;
            }
            bms.enable_drive_inhibit("[S13] charge requested");
            bms.set_state(&state_charging, "charge requested");
            break;
        case E_CHARGING_TERMINATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_TERMINATED while in standby state\n");
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State          : drive
 * Ignition       : on
 * Contactors     : closed / inhibited
 * Charging       : no
 * heater         : off
 * drive inhibit  : off
 * charge inhibit : off
 */
void state_drive(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            // We're not charging right now, but flag that it's too cold for later reference
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[D01] too cold to charge");
            } else if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[D02] full cell");
            } else {
                bms.disable_charge_inhibit("[D03] not too cold to charge");
            }
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                bms.enable_drive_inhibit("[D04] battery too hot");
                bms.enable_charge_inhibit("[D05] battery too hot");
                bms.set_state(&state_overTempFault, "battery too hot");
                break;
            }
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            // FIXME can we disable regen when the battery is full?

            /* Battery is empty. Disallow driving (force car into neutral). We
             * cannot open the contactors as this could blow up the inverter. */
            if ( battery.has_empty_cell() ) {
                bms.enable_drive_inhibit("[D06] empty battery");
                bms.set_state(&state_batteryEmpty, "empty battery");
                break;
            }
            if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[D07] full cell\n");
            } else {
                bms.disable_charge_inhibit("[D08] no full cell");
            }
            /* If we're driving on a subset of pack(s), and we've driven down
             * the high pack(s) enough that its/their voltage matches the low
             * pack(s), allow the contactors to close on the low pack(s). */
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_for_drive();
            }
            break;
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
            /* Cannot go straight from drive mode to charge mode when packs are
            * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() ) {
                bms.enable_drive_inhibit("[D09] imbalanced packs");
                bms.enable_charge_inhibit("[D10] imbalanced packs");
                bms.set_illegal_state_transition();
                bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from drive to charge with imbalanced packs");
                break;
            }
            /* Lets assume that we're not in motion (hopefully a safe 
             * assumption). All contactors are already closed so we can just 
             * switch straight into charge mode. */
            bms.enable_drive_inhibit("[D11] charge mode");
            bms.set_state(&state_charging, "charge requested");
            break;
        case E_CHARGING_TERMINATED:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : E_CHARGING_TERMINATED while in drive state\n");
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State          : batteryHeating
 * Ignition       : on or off
 * Contactors     : closed
 * Charging       : no (but CHARGE_ENABLE is on, so waiting to charge)
 * heater         : on
 * drive inhibit  : on
 * charge inhibit : on
 */
void state_batteryHeating(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            /* Important to catch overheating first. We don't want to heat the
             * battery all the way into an overheat situation */
            if ( battery.has_temperature_sensor_over_max() ) {
                bms.enable_drive_inhibit("[H01] battery too hot");
                bms.enable_charge_inhibit("[H02] battery too hot");
                bms.set_state(&state_overTempFault, "battery too hot");
                break;
            }
            // If no longer too cold to charge, allow charging
            if ( !battery.too_cold_to_charge() && !battery.has_full_cell() ) {
                bms.disable_heater();
                bms.disable_charge_inhibit("[H03] not too cold to charge and cell not full");
                bms.set_state(&state_charging, "battery warmed to minimum charging temperature");
                break;
            }
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            /* There's no point in heating the battery to get it ready for charging if it's already full.
             * Charger should pick up on this and stop requesting charge. */
            if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[H04] full cell");
                bms.disable_heater();
                bms.set_state(&state_charging, "battery was heating, but is already full. No need to continue heating.");
                break;
            }
            break;
        case E_IGNITION_ON:
            // Valid event, but we don't need to do anything with it.
            break;
        case E_IGNITION_OFF:
            // Valid event, but we don't need to do anything with it.
            break;
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
                bms.enable_drive_inhibit("[H05] imbalanced packs");
                bms.enable_charge_inhibit("[H06] imbalanced packs");
                bms.set_illegal_state_transition();
                bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from charge to drive with imbalanced packs");
                break;
            }
            // Battery empty
            if ( battery.has_empty_cell() ) {
                bms.enable_drive_inhibit("[H07] empty battery");
                bms.set_state(&state_batteryEmpty, "charge terminated but battery still empty");
                break;
            }
            // Drive mode
            if ( bms.ignition_is_on() ) {
                bms.disable_charge_inhibit("[H08] ignition on");
                bms.disable_drive_inhibit("[H09] ignition on");
                bms.set_state(&state_drive, "charging terminated + ignition on");
                break;
            }
            // Standby mode
            bms.disable_drive_inhibit("[H09] ignition off");
            bms.disable_charge_inhibit("[H10] ignition off");
            bms.set_state(&state_standby, "charging terminated");
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");

    }
}

/*
 * State          : charging
 * Ignition       : on or off
 * Contactors     : closed / inhibited
 * Charging       : yes
 * heater         : off
 * drive inhibit  : on
 * charge inhibit : on or off
 */
void state_charging(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                bms.disable_heater();
                bms.enable_drive_inhibit("[C01] battery too hot");
                bms.enable_charge_inhibit("[C02] battery too hot");
                // We stay in the charging state and wait for the charge to fully terminate
            }
            // If we're waiting on the battery to warm, and it has, allow charging
            if ( bms.heater_is_enabled() && !battery.too_cold_to_charge() ) {
                bms.disable_heater();
                bms.disable_charge_inhibit("[C03] not too cold to charge");
            }
            /* Deal with the unlikely scenario where the battery gets too cold
             * mid-charge. Enable heater, block charging. */
            if ( !bms.heater_is_enabled() && battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[C04] too cold to charge");
                bms.enable_heater();
            }
            // Recalculate max charging current
            bms.update_max_charge_current();
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            /* Prevent cells from getting over-charged */
            if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[C05] full cell");
            }
            break;
        case E_IGNITION_ON:
            // Valid event, but we don't need to do anything with it.
            break;
        case E_IGNITION_OFF:
            // Valid event, but we don't need to do anything with it.
            break;
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
            // Battery is overheating, switch directly to overTempFault state
            if ( battery.has_temperature_sensor_over_max() ) {
                // drive/charge should alredy be inhibited at this point, but just in case
                bms.enable_drive_inhibit("[C06] battery too hot");
                bms.enable_charge_inhibit("[C07] battery too hot");
                bms.set_state(&state_overTempFault, "battery too hot");
                break;
            }
            /* Cannot go straight from drive mode to charge mode when packs are
             * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() && bms.ignition_is_on() ) {
                bms.enable_drive_inhibit("[C08] imbalanced packs");
                bms.enable_charge_inhibit("[C09] imbalanced packs");
                bms.set_illegal_state_transition();
                bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from charge to drive with imbalanced packs");
                break;
            }
            /* Did we start charging with an empty battery, but cancel the
             * charge before actually putting any energy into the battery? */
            if ( battery.has_empty_cell() ) {
                bms.enable_drive_inhibit("[C10] empty battery");
                bms.set_state(&state_batteryEmpty, "charge terminated but battery still empty");
                break;
            }
            // If ignition is already on, switch directly to drive mode
            if ( bms.ignition_is_on() ) {
                bms.set_state(&state_drive, "charging terminated + ignition on");
                break;
            }
            bms.set_state(&state_standby, "charging terminated");
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in charging state\n");
    }
}


/*
 * State          : batteryEmpty
 * Ignition       : on / off
 * Contactors     : open / closed / inhibited
 * Charging       : no
 * heater         : off
 * drive inhibit  : on
 * charge inhibit : on / off
 */
void state_batteryEmpty(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[E01] too cold to charge");
            } else if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[E02] full cell");
            } else {
                bms.disable_charge_inhibit("[E03] not too cold to charge and no full cell");
            }
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                bms.enable_drive_inhibit("[E04] battery too hot");
                bms.enable_charge_inhibit("[E05] battery too hot");
                bms.set_state(&state_overTempFault, "battery too hot");
                break;
            }
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[E06] too cold to charge");
            } else if ( battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[E07] full cell");
            } else {
                bms.disable_charge_inhibit("[E08] not too cold to charge and no full cell");
            }
            // After resting for a while, the voltage may rise again slightly.
            if ( !battery.has_empty_cell() ) {
                /* We might have got here from either charge mode or drive mode.
                 * We cannot tell which. And we cannot go straight from charge
                 * to drive or vice versa (see note 1 above), so just go to
                 * illegal state if the ignition is on. Force the user to turn
                 * off the ignition, which will open the contactors. */
                if ( battery.one_or_more_contactors_inhibited() && bms.ignition_is_on() ) {
                    bms.enable_drive_inhibit("[E09] imbalanced packs");
                    bms.enable_charge_inhibit("[E10] imbalanced packs");
                    bms.increment_invalid_event_count();
                    bms.set_state(&state_illegalStateTransitionFault, "cannot switch directly from charge to drive with imbalanced packs");
                    break;
                }
                // allow driving again
                bms.disable_drive_inhibit("[E11] battery not empty");
                bms.set_state(&state_standby, "battery level rose");
                break;
            }
            break;
        case E_IGNITION_ON:
            // Valid event, but we don't need to do anything with it.
            break;
        case E_IGNITION_OFF:
            // Valid event, but we don't need to do anything with it.
            break;
        case E_CHARGING_INITIATED:
            // Is it too cold to charge?
            if ( battery.too_cold_to_charge() ) {
                bms.enable_charge_inhibit("[E12] too cold to charge");
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
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

/*
 * State          : overTempFault
 * Ignition       : on / off
 * Contactors     : open / closed / inhibited
 * Charging       : no
 * heater         : off
 * drive inhibit  : on
 * charge inhibit : on
 *
 * Reasons we can be in this state:
 *   - Batteries are too hot
 */
void state_overTempFault(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            /* Temperature has dropped below max limit. We need to figure out
             * which state to switch to based on the input signals */
            if ( !battery.has_temperature_sensor_over_max() ) {
                // Charge mode overrides drive mode
                if ( bms.charge_is_enabled() ) {
                    if ( battery.one_or_more_contactors_inhibited() ) {
                        battery.disable_inhibit_for_charge();
                    }
                    bms.disable_charge_inhibit("[T01] battery has cooled");
                    bms.set_state(&state_charging, "battery has cooled");
                    break;
                }
                // Drive mode
                if ( bms.ignition_is_on() ) {
                    if ( battery.one_or_more_contactors_inhibited() ) {
                        battery.disable_inhibit_for_drive();
                    }
                    bms.disable_charge_inhibit("[T02] battery has cooled");
                    bms.disable_drive_inhibit("[T03] battery has cooled");
                    bms.set_state(&state_drive, "battery has cooled");
                    break;
                }
                // Standby mode
                bms.disable_drive_inhibit("[T04] battery has cooled");
                bms.disable_charge_inhibit("[T05] battery has cooled");
                bms.set_state(&state_standby, "battery has cooled");
                break;
            }
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            break;
        case E_IGNITION_ON:
            // Valid event, but we don't need to do anything with it.
            break;
        case E_IGNITION_OFF:
            // Valid event, but we don't need to do anything with it.
            break;
        case E_CHARGING_INITIATED:
            // FIXME should we go into charge mode, but keep everything inhibited? Or stay here?
            break;
        case E_CHARGING_TERMINATED:
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}


/*
 * State          : illegalStateTransitionFault
 * Ignition       : on / off
 * Contactors     : open / closed / inhibited
 * Charging       : no
 * heater         : off
 * drive inhibit  : on
 * charge inhibit : on
 *
 * Reasons we can be in this state:
 *   - We tried to go straight from drive to charge with imbalanced packs
 */
void state_illegalStateTransitionFault(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            if ( !bms.charge_is_inhibited() && battery.has_full_cell() ) {
                bms.enable_charge_inhibit("[F01] full cell");
            }
            break;
        case E_IGNITION_ON:
            break;
        case E_IGNITION_OFF:
            if ( ! bms.charge_is_enabled() ) {
                bms.set_state(&state_standby, "ignition and charging off");
            }
            break;
        case E_CHARGING_INITIATED:
            break;
        case E_CHARGING_TERMINATED:
            if ( ! bms.ignition_is_on() ) {
                bms.set_state(&state_standby, "ignition and charging off");
            }
            break;
        default:
            bms.increment_invalid_event_count();
            printf("WARNING : invalid event : UNKNOWN while in standby state\n");
    }
}

// Mapping between state functions and their names

typedef struct stateName {
    State state;
    const char * stateName;
} stateName;

struct stateName stateNames[7] = {
    {state_standby, "standby"},
    {state_drive, "drive"},
    {state_batteryHeating, "batteryHeating"},
    {state_charging, "charging"},
    {state_batteryEmpty, "batteryEmpty"},
    {state_overTempFault, "overTempFault"},
    {state_illegalStateTransitionFault, "illegalStateTransistionFault"}
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
