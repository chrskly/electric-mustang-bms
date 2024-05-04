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
#include "include/comms.h"

extern Battery battery;
extern State state;
extern StatusLight statusLight;


--------------------------------------------------------------------------------
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
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                printf("Switching to state : overTempFault, reason : battery too hot\n");
                statusLight.led_set_mode(FAULT);
                state = state_overTempFault;
                break;
            }
            // We're not charging right now, but flag that it's too cold for later reference
            if ( !battery.charge_is_inhibited() && battery.too_cold_to_charge() ) {
                battery.enable_inhibit_charge();
            }
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            // Battery is empty. Disallow driving.
            if ( battery.has_empty_cell() ) {
                battery.enable_inhibit_drive();
                printf("Switching to state : batteryEmpty\n");
                statusLight.led_set_mode(FAULT);
                state = state_batteryEmpty;
                break;
            }
            /* The contactors are currently open. We don't want to allow the
             * contactors to close when the packs have different voltages. So we
             * inhibit the contactors on all packs here. When we switch into
             * another state we'll decide which contactors to allow to close
             * then. This will depend on which state we switch into.
             */
            if ( battery.packs_are_imbalanced() ) {
                battery.inhibit_contactor_close();
            }
            break;
        case E_IGNITION_ON:
            /* Packs are imbalanced. Decide which contactor(s) to allow to 
             * close. Since we're going into drive mode, we want to pick the
             * high pack(s).
             */
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_for_drive();
            }
            printf("Switching to state : drive\n");
            statusLight.led_set_mode(DRIVE);
            state = state_drive;
            break;
        case E_IGNITION_OFF:
            // Already in standby state, nothing to do.
            break;
        case E_CHARGING_INITIATED:
            /* If the batteries are not warm enough to be charged, turn on the
             * battery heater, and disallow charging until they're warm enough.
             */
            if ( battery.too_cold_to_charge() ) {
                battery.enable_heater();
                battery.enable_inhibit_charge();
                battery.enable_inhibit_drive();
                printf("Switching to state : state_batteryHeating, reason : charge requested, but too cold to charge\n");
                statusLight.led_set_mode(CHARGING);
                state = state_batteryHeating;
                break;
            }
            printf("Switching to state : charging\n");
            statusLight.led_set_mode(CHARGING);
            state = state_charging;
            break;
        case E_CHARGING_TERMINATED:
            break;
        case E_EMERGENCY_SHUTDOWN:
            break;
        default:
            printf("Received unknown event");
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
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                printf("Switching to state : overTempFault, reason : battery too hot\n");
                statusLight.led_set_mode(FAULT);
                state = state_overTempFault;
                break;
            }
            // We're not charging right now, but flag that it's too cold for later reference
            if ( !battery.charge_is_inhibited() && battery.too_cold_to_charge() ) {
                battery.enable_inhibit_charge();
            }
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            // Battery is full.
            // Can we disable regen?
            // if ( battery.has_full_cell() ) { }

            /* Battery is empty. Disallow driving (force car into neutral). We
             * cannot open the contactors as this could blow up the inverter.
             */
            if ( battery.has_empty_cell() ) {
                battery.enable_inhibit_drive();
                printf("Switching to state : batteryEmpty, reason : empty battery\n");
                statusLight.led_set_mode(FAULT);
                state = state_batteryEmpty;
                break;
            }
            /* If we're driving on a subset of pack(s), and we've driven down
             * the high pack(s) enough that its/their voltage matches the low
             * pack(s), allow the contactors to close on the low pack(s).
             */
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.disable_inhibit_for_drive();
            }
            break;
        case E_IGNITION_ON:
            // Already in drive mode, nothing to do.
            break;
        case E_IGNITION_OFF:
            printf("Switching to state : standby, reason : ignition turned off\n");
            statusLight.led_set_mode(STANDBY);
            state = state_standby;
            break;
        case E_CHARGING_INITIATED:
            /* Cannot go straight from drive mode to charge mode when packs are
            * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                printf("Switching to state : illegalStateTransitionFault, reason : cannot switch directly from drive to charge with imbalanced packs\n");
                statusLight.led_set_mode(FAULT);
                state = state_illegalStateTransitionFault;
                break;
            }
            /* Lets assume that we're not in motion (hopefully a safe 
             * assumption). All contactors are already closed so we can just 
             * switch straight into charge mode.
             */
            battery.enable_inhibit_drive();
            printf("Switching to state : charging, reason : charginging initiated\n");
            statusLight.led_set_mode(CHARGING);
            state = state_charging;
            break;
        case E_CHARGING_TERMINATED:
            break;
        case E_EMERGENCY_SHUTDOWN:
            // Tell inverter to shut down + short sleep.
            break;
        default:
            printf("Received unknown event\n");
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
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                printf("Switching to state : overTempFault, reason : battery too hot\n");
                statusLight.led_set_mode(FAULT);
                state = state_overTempFault;
                break;
            }
            // If no longer too cold to charge, allow charging
            if ( !battery.too_cold_to_charge() && !battery.has_full_cell() ) {
                battery.disable_heater();
                battery.disable_inhibit_charge();
                printf("Switching to state : charging, reason : battery warmed to minimum charging temperature\n");
                statusLight.led_set_mode(CHARGING);
                state = state_charging;
                break;
            }
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            /* There's no point in heating the battery to get it ready for charging if it's already full.
             * Charger should pick up on this and stop requesting charge. */
            if ( battery.has_full_cell() ) {
                battery.enable_inhibit_charge();
                battery.disable_heater();
                printf("Switching to state : charging, reason : battery was heating, but is already full. No need to continue heating.\n");
                statusLight.led_set_mode(CHARGING);
                state = state_charging;
                break;
            }
            break;
        case E_IGNITION_ON:
            break;
        case E_IGNITION_OFF:
            break;
        case E_CHARGING_INITIATED:
            break;
        case E_CHARGING_TERMINATED:
            // We're no longer seeking to charge. No need to continue heating.
            battery.disable_heater();
            /* Cannot go straight from drive mode to charge mode when packs are
             * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() && battery.ignition_is_on() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                // FIXME do we need to set some sort of error flag here?
                printf("Switching to state : illegalStateTransitionFault, reason : cannot switch directly from charge to drive with imbalanced packs\n");
                statusLight.led_set_mode(FAULT);
                state = state_illegalStateTransitionFault;
                break;
            }
            // Battery empty
            if ( battery.has_empty_cell() ) {
                battery.enable_inhibit_drive();
                printf("Switching to state : batteryEmpty, reason : charge terminated but battery still empty\n");
                statusLight.led_set_mode(FAULT);
                state = state_batteryEmpty;
                break;
            }
            // Drive mode
            if ( battery.ignition_is_on() ) {
                battery.disable_inhibit_charge();
                battery.disable_inhibit_drive();
                printf("Switching to state : drive, reason : charging request has been terminated and ignition is on\n");
                statusLight.led_set_mode(DRIVE);
                state = state_drive;
                break;
            }
            // Standby mode
            battery.disable_inhibit_drive();
            battery.disable_inhibit_charge();
            printf("Switching to state : standby, reason : charge request has been terminated\n");
            statusLight.led_set_mode(STANDBY);
            state = state_standby;
            break;
        case E_EMERGENCY_SHUTDOWN:
            break;
        default:
            printf("Received unknown event\n");

    }
}

/*
 * State          : charging
 * Ignition       : on or off
 * Contactors     : closed / inhibited
 * Charging       : yes
 * heater         : off
 * drive inhibit  : on
 * charge inhibit : off
 */
void state_charging(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                printf("Switching to state : overTempFault, reason : battery too hot\n");
                statusLight.led_set_mode(FAULT);
                state = state_overTempFault;
                break;
            }
            // If we're waiting on the battery to warm, and it has, allow charging
            if ( battery.heater_enabled() && !battery.too_cold_to_charge() ) {
                battery.disable_heater();
                battery.disable_inhibit_charge();
            }
            /* Deal with the unlikely scenario where the battery gets too cold
             * mid-charge. Enable heater, block charging.
             */
            if ( !battery.heater_enabled() && battery.too_cold_to_charge() ) {
                battery.enable_inhibit_charge();
                battery.enable_heater();
            }
            // Recalculate max charging current
            battery.update_max_charge_current();
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            /* Prevent cells from getting over-charged */
            if ( battery.has_full_cell() ) {
                battery.enable_inhibit_charge();
            }
            break;
        case E_IGNITION_ON:
            /* We're going to continue to charge so we should prevent the car
             * from driving away.
             */
            battery.enable_inhibit_drive();
            break;
        case E_IGNITION_OFF:
            battery.disable_inhibit_drive();
            break;
        case E_CHARGING_INITIATED:
            // We're already charging. Nothing to do.
            break;
        case E_CHARGING_TERMINATED:
            /* Cannot go straight from drive mode to charge mode when packs are
             * imbalanced. See note 1 above. */
            if ( battery.one_or_more_contactors_inhibited() && battery.ignition_is_on() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                // FIXME do we need to set some sort of error flag here?
                printf("Switching to state : illegalStateTransitionFault, reason : cannot switch directly from charge to drive with imbalanced packs\n");
                statusLight.led_set_mode(FAULT);
                state = state_illegalStateTransitionFault;
                break;
            }
            /* Did we start charging with an empty battery, but cancel the
             * charge before actually putting any energy into the battery?
             */
            if ( battery.has_empty_cell() ) {
                battery.enable_inhibit_drive();
                printf("Switching to state : batteryEmpty, reason : charge terminated but battery still empty\n");
                statusLight.led_set_mode(FAULT);
                state = state_batteryEmpty;
                break;
            }
            /* If battery is full (i.e., we've charged to 100%), reset kWh/Ah
             * counters on the ISA shunt.
             */
            if ( battery.has_full_cell() ) {
                send_ISA_reset_message();
            }
            // If ignition is already on, switch directly to drive mode
            if ( battery.ignition_is_on() ) {
                printf("Switching to state : drive, reason : charging terminated + ignition on\n");
                statusLight.led_set_mode(DRIVE);
                state = state_drive;
                break;
            }
            printf("Switching to state : standby, reason : charging terminated\n");
            statusLight.led_set_mode(STANDBY);
            state = state_standby;
            break;
        case E_EMERGENCY_SHUTDOWN:
            break;
        default:
            printf("Received unknown event\n");
    }
}


/*
 * State          : batteryEmpty
 * Ignition       : on / off
 * Contactors     : open / closed / inhibited
 * Charging       : no
 * heater         : off
 * drive inhibit  : on
 * charge inhibit : off
 */
void state_batteryEmpty(Event event) {
    switch (event) {
        case E_TEMPERATURE_UPDATE:
            // Battery is overheating. Stop everything.
            if ( battery.has_temperature_sensor_over_max() ) {
                battery.enable_inhibit_drive();
                battery.enable_inhibit_charge();
                printf("Switching to state : overTempFault, reason : battery too hot\n");
                statusLight.led_set_mode(FAULT);
                state = state_overTempFault;
                break;
            }
            if ( battery.charge_is_inhibited() && battery.too_cold_to_charge() ) {
                battery.enable_inhibit_charge();
            }
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            // After resting for a while, the voltage may rise again slightly.
            if ( !battery.has_empty_cell() ) {
                // allow driving again
                battery.disable_inhibit_drive();

                // If ignition is already enabled, switch directly to drive mode.
                if ( battery.ignition_is_on() ) {
                    if ( battery.one_or_more_contactors_inhibited() ) {
                        battery.disable_inhibit_for_drive();
                    }
                    printf("Switching to state : drive, reason : ignition turned on\n");
                    statusLight.led_set_mode(DRIVE);
                    state = state_drive;
                    break;
                }
                printf("Switching to state : standby, reason : battery level rose\n");
                statusLight.led_set_mode(STANDBY);
                state = state_standby;
            }
            break;
        case E_IGNITION_ON:
            // Disallow driving.
            battery.enable_inhibit_drive();
            break;
        case E_IGNITION_OFF:
            break;
        case E_CHARGING_INITIATED:
            // Is it too cold to charge?
            if ( battery.too_cold_to_charge() ) {
                battery.enable_inhibit_charge();
                battery.enable_heater();
            }
            printf("Switching to state : charging, reason : charging initiated\n");
            statusLight.led_set_mode(CHARGING);
            state = state_charging;
            break;
        case E_CHARGING_TERMINATED:
            break;
        case E_EMERGENCY_SHUTDOWN:
            break;
        default:
            printf("Received unknown event");
    }
}

/*
 * State          : overTempFault
 * Ignition       : on / off
 * Contactors     : open / closed
 * Charging       : yes (waiting for charing to terminate) / no
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
             * which state to switch to based on the input signals
             */
            if ( !battery.has_temperature_sensor_over_max() ) {
                // Charge mode overrides drive mode
                if ( battery.charge_enable_is_on() ) {
                    if ( battery.one_or_more_contactors_inhibited() ) {
                        battery.disable_inhibit_for_charge();
                    }
                    battery.disable_inhibit_charge();
                    printf("Switching to state : charging, reason : battery has cooled\n");
                    statusLight.led_set_mode(CHARGING);
                    state = state_charging;
                    break;
                }
                // Drive mode
                if ( battery.ignition_is_on() ) {
                    if ( battery.one_or_more_contactors_inhibited() ) {
                        battery.disable_inhibit_for_drive();
                    }
                    battery.disable_inhibit_charge();
                    battery.disable_inhibit_drive();
                    printf("Switching to state : drive, reason : battery has cooled\n");
                    statusLight.led_set_mode(DRIVE);
                    state = state_drive;
                    break;
                }
                // Standby mode
                battery.disable_inhibit_drive();
                battery.disable_inhibit_charge();
                printf("Switching to state : standby, reason : battery has cooled\n");
                statusLight.led_set_mode(STANDBY);
                state = state_standby;
                break;
            }
            break;
        case E_CELL_VOLTAGE_UPDATE:
            battery.process_voltage_update();
            break;
        case E_IGNITION_ON:
            break;
        case E_IGNITION_OFF:
            break;
        case E_CHARGING_INITIATED:
            break;
        case E_CHARGING_TERMINATED:
            break;
        case E_EMERGENCY_SHUTDOWN:
            break;
        default:
            printf("Received unknown event\n");
    }
}


/*
 * State          : illegalStateTransitionFault
 * Ignition       : --
 * Contactors     : open / closed
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
            break;
        case E_IGNITION_ON:
            break;
        case E_IGNITION_OFF:
            if ( ! battery.charge_enable_is_on() ) {
                printf("Switching to state : standby, reason : ignition and charging off\n");
                state = state_standby;
            }
            break;
        case E_CHARGING_INITIATED:
            break;
        case E_CHARGING_TERMINATED:
            if ( ! battery.ignition_is_on() ) {
                printf("Switching to state : standby, reason : ignition and charging off\n");
                state = state_standby;
            }
            break;
        case E_EMERGENCY_SHUTDOWN:
            break;
        default:
            printf("Received unknown event\n");
    }
}

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

const char* get_state_name() {
    for ( int i=0; i < 8; i++ ) {
        if ( state == stateNames[i].state ) {
            return stateNames[i].stateName;
        }
    }
    return "unknownState";
}
