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
#include <string>

#include "hardware/gpio.h"
#include "include/io.h"
//#include "include/battery.h"
#include "include/bms.h"
#include "settings.h"

// Input handlers
// These are resistor divider inputs. High is on, low is off.

void gpio_callback(uint gpio, uint32_t events) {
    extern Bms bms;
    //extern Battery battery;

    int newState;
    std::string newStateStr;
    //printf("GPIO %d event %d\n", gpio, events);
    if ( gpio == DRIVE_INHIBIT_PIN ) {
        newState = gpio_get(DRIVE_INHIBIT_PIN);
        newStateStr = newState == 1 ? "on" : "off";
        printf("    * Drive inhibit signal changed to : %s\n", newStateStr.c_str());
        bms.set_inhibitDrive(newState == 1);
    }
    if ( gpio == CHARGE_INHIBIT_PIN ) {
        newState = gpio_get(CHARGE_INHIBIT_PIN);
        newStateStr = newState == 1 ? "on" : "off";
        printf("    * Charge inhibit signal changed to : %s\n", newStateStr.c_str());
        bms.set_inhibitCharge(newState == 1);
    }
    if ( gpio == INHIBIT_CONTACTOR_PINS[0] ) { // batt1 inhibit
        newState = gpio_get(INHIBIT_CONTACTOR_PINS[0]);
        newStateStr = newState == 1 ? "on" : "off";
        printf("    * Battery 1 inhibit signal changed to : %s\n", newStateStr.c_str());
        bms.get_battery().get_pack(0)->set_inhibit(newState == 1);
    }
    if ( gpio == INHIBIT_CONTACTOR_PINS[1] ) { // batt2 inhibit
        newState = gpio_get(INHIBIT_CONTACTOR_PINS[1]);
        newStateStr = newState == 1 ? "on" : "off";
        printf("    * Battery 2 inhibit signal changed to : %s\n", newStateStr.c_str());
        bms.get_battery().get_pack(1)->set_inhibit(newState == 1);
    }
}

void enable_listen_for_input_signals() {
    printf("Enabling input signal listeners\n");
    
    gpio_init(DRIVE_INHIBIT_PIN);
    gpio_set_dir(DRIVE_INHIBIT_PIN, GPIO_IN);

    gpio_init(CHARGE_INHIBIT_PIN);
    gpio_set_dir(CHARGE_INHIBIT_PIN, GPIO_IN);

    gpio_init(INHIBIT_CONTACTOR_PINS[0]);
    gpio_set_dir(INHIBIT_CONTACTOR_PINS[0], GPIO_IN);

    gpio_init(INHIBIT_CONTACTOR_PINS[1]);
    gpio_set_dir(INHIBIT_CONTACTOR_PINS[1], GPIO_IN);

    gpio_set_irq_enabled_with_callback(DRIVE_INHIBIT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(CHARGE_INHIBIT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(INHIBIT_CONTACTOR_PINS[0], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(INHIBIT_CONTACTOR_PINS[1], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}


// Outputs
// We're driving low side switches here. A high signal activates the switch which grounds whatever we're driving. In testing
// we're driving relays because we ulitmately want to generate a 12V signal for the digitial inputs on the BMS side.
// high => on, low => off

void set_ignition_state(bool state) {
    std::string stateStr = state ? "on" : "off";
    printf("    * Setting ignition state to %s\n", stateStr.c_str());
    gpio_put(IGNITION_ENABLE_PIN, state);
}

void set_charge_enable_state(bool state) {
    std::string stateStr = state ? "on" : "off";
    printf("    * Setting charge enable state to %s\n", stateStr.c_str());
    gpio_put(CHARGE_ENABLE_PIN, state);
}