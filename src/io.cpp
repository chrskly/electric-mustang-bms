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

#include "hardware/gpio.h"

#include "include/statemachine.h"
#include "settings.h"
#include "include/io.h"
#include "include/bms.h"

extern Bms bms;

// Input signal handler
// These are resistor divider inputs. High is on, low is off.

void gpio_callback(uint gpio, uint32_t events) {
    extern State state;
    int newState;
    std::string newStateStr;
    if ( gpio == IGNITION_ENABLE_PIN ) {
        newState = gpio_get(IGNITION_ENABLE_PIN);
        newStateStr = newState == 1 ? "on" : "off";
        printf("    * Ignition signal changed to : %s\n", newStateStr.c_str());
        if ( newState ) {
            bms.send_event(E_IGNITION_ON);
        } else {
            bms.send_event(E_IGNITION_OFF);
        }
    }
    if ( gpio == CHARGE_ENABLE_PIN ) {
        newState = gpio_get(CHARGE_ENABLE_PIN);
        newStateStr = newState == 1 ? "on" : "off";
        printf("Charge signal changed to : %s\n", newStateStr.c_str());
        if ( newState ) {
            bms.send_event(E_CHARGING_INITIATED);
        } else {
            bms.send_event(E_CHARGING_TERMINATED);
        }
    }
}

Io::Io() {
    ignitionOn = false;
    chargeEnable = false;

    // IGNITION input
    gpio_init(IGNITION_ENABLE_PIN);
    gpio_set_dir(IGNITION_ENABLE_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(IGNITION_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // CHARGE_ENABLE input
    gpio_init(CHARGE_ENABLE_PIN);
    gpio_set_dir(CHARGE_ENABLE_PIN, GPIO_IN);
    gpio_set_irq_enabled(CHARGE_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // POS_CONTACTOR_FEEDBACK input
    gpio_init(POS_CONTACTOR_FEEDBACK_PIN);
    gpio_set_dir(POS_CONTACTOR_FEEDBACK_PIN, GPIO_IN);
    gpio_set_irq_enabled(POS_CONTACTOR_FEEDBACK_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // NEG_CONTACTOR_FEEDBACK input
    gpio_init(NEG_CONTACTOR_FEEDBACK_PIN);
    gpio_set_dir(NEG_CONTACTOR_FEEDBACK_PIN, GPIO_IN);
    gpio_set_irq_enabled(NEG_CONTACTOR_FEEDBACK_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // DRIVE_INHIBIT output
    gpio_init(DRIVE_INHIBIT_PIN);
    gpio_set_dir(DRIVE_INHIBIT_PIN, GPIO_OUT);
    disable_drive_inhibit("initialization\n");

    // CHARGE_INHIBIT output
    gpio_init(CHARGE_INHIBIT_PIN);
    gpio_set_dir(CHARGE_INHIBIT_PIN, GPIO_OUT);
    disable_charge_inhibit("initialization\n");

    // Heater output
    gpio_init(HEATER_ENABLE_PIN);
    gpio_set_dir(HEATER_ENABLE_PIN, GPIO_OUT);
    disable_heater();
}

// REMINDER : THESE OUTPUTS ARE A LOW SIDE SWITCHES.
//     gpio high == on  == output low
//     gpio low  == off == output high/floating?

// DRIVE_INHIBIT output

void Io::enable_drive_inhibit(std::string context) {
    printf("    * Enabling drive inhibit : %s\n", context.c_str());
    gpio_put(DRIVE_INHIBIT_PIN, 1);
}

void Io::disable_drive_inhibit(std::string context) {
    printf("    * Disabling drive inhibit : %s\n", context.c_str());
    gpio_put(DRIVE_INHIBIT_PIN, 0);
}

bool Io::drive_is_inhibited() {
    return gpio_get(DRIVE_INHIBIT_PIN);
}

// CHARGE_INHIBIT output

void Io::enable_charge_inhibit(std::string context) {
    printf("    * Enabling charge inhibit : %s\n", context.c_str());
    gpio_put(CHARGE_INHIBIT_PIN, 1);
}

void Io::disable_charge_inhibit(std::string context) {
    printf("    * Disabling charge inhibit : %s\n", context.c_str());
    gpio_put(CHARGE_INHIBIT_PIN, 0);
}

bool Io::charge_is_inhibited() {
    return gpio_get(CHARGE_INHIBIT_PIN);
}

// HEATER output

void Io::enable_heater() {
    if ( !gpio_get(HEATER_ENABLE_PIN) ) {
        printf("Enabling heater\n");
        gpio_put(HEATER_ENABLE_PIN, 1);
    }
}

void Io::disable_heater() {
    if ( gpio_get(HEATER_ENABLE_PIN) ) {
        printf("Disabling heater\n");
        gpio_put(HEATER_ENABLE_PIN, 0);
    }
}

bool Io::heater_is_enabled() {
    return gpio_get(HEATER_ENABLE_PIN);
}

// Inputs

bool Io::ignition_is_on() {
    return gpio_get(IGNITION_ENABLE_PIN);
}

bool Io::charge_enable_is_on() {
    return gpio_get(CHARGE_ENABLE_PIN);
}

bool Io::pos_contactor_is_welded() {
    return gpio_get(POS_CONTACTOR_FEEDBACK_PIN);
}

bool Io::neg_contactor_is_welded() {
    return gpio_get(NEG_CONTACTOR_FEEDBACK_PIN);
}
