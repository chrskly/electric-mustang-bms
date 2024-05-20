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

void gpio_callback(uint gpio, uint32_t events) {
    extern State state;
    if ( gpio == IGNITION_ENABLE_PIN ) {
        printf("Ignition signal changed to : %d", gpio_get(IGNITION_ENABLE_PIN));
        if ( IGNITION_ENABLE_ACTIVE_LOW == gpio_get(IGNITION_ENABLE_PIN) ) {
            bms.send_event(E_IGNITION_OFF);
        } else {
            bms.send_event(E_IGNITION_ON);
        }
    }
    if ( gpio == CHARGE_ENABLE_PIN ) {
        printf("Charge signal changed to : %d", gpio_get(CHARGE_ENABLE_PIN));
        if ( CHARGE_ENABLE_ACTIVE_LOW == gpio_get(CHARGE_ENABLE_PIN) ) {
            bms.send_event(E_CHARGING_TERMINATED);
        } else {
            bms.send_event(E_CHARGING_INITIATED);
        }
    }
}

Io::Io() {
    ignitionOn = false;
    chargeEnable = false;

    // IGNITION input
    gpio_set_dir(IGNITION_ENABLE_PIN, GPIO_IN);
    gpio_set_pulls(IGNITION_ENABLE_PIN, !IGNITION_ENABLE_ACTIVE_LOW, IGNITION_ENABLE_ACTIVE_LOW);
    gpio_set_irq_enabled_with_callback(IGNITION_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // CHARGE_ENABLE input
    gpio_set_dir(CHARGE_ENABLE_PIN, GPIO_IN);
    gpio_set_pulls(CHARGE_ENABLE_PIN, !CHARGE_ENABLE_ACTIVE_LOW, CHARGE_ENABLE_ACTIVE_LOW);
    gpio_set_irq_enabled(CHARGE_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // DRIVE_INHIBIT output
    gpio_init(DRIVE_INHIBIT_PIN);
    gpio_set_dir(DRIVE_INHIBIT_PIN, GPIO_OUT);
    gpio_set_pulls(DRIVE_INHIBIT_PIN, !DRIVE_INHIBIT_ACTIVE_LOW, DRIVE_INHIBIT_ACTIVE_LOW);

    // CHARGE_INHIBIT output
    gpio_init(CHARGE_INHIBIT_PIN);
    gpio_set_pulls(CHARGE_INHIBIT_PIN, !CHARGE_INHIBIT_ACTIVE_LOW, CHARGE_INHIBIT_ACTIVE_LOW);
    gpio_set_dir(CHARGE_INHIBIT_PIN, GPIO_OUT);

    // Heater output
    gpio_init(HEATER_ENABLE_PIN);
    gpio_set_dir(HEATER_ENABLE_PIN, GPIO_OUT);
    gpio_set_pulls(HEATER_ENABLE_PIN, !HEATER_ENABLE_ACTIVE_LOW, HEATER_ENABLE_ACTIVE_LOW);
}

// DRIVE_INHIBIT output

void Io::enable_drive_inhibit() {
    printf("Enabling drive inhibit\n");
    if ( DRIVE_INHIBIT_ACTIVE_LOW ) {
        gpio_put(DRIVE_INHIBIT_PIN, 0);
    } else {
        gpio_put(DRIVE_INHIBIT_PIN, 1);
    }
}

void Io::disable_drive_inhibit() {
    printf("Disabling drive inhibit\n");
    if ( DRIVE_INHIBIT_ACTIVE_LOW ) {
        gpio_put(DRIVE_INHIBIT_PIN, 1);
    } else {
        gpio_put(DRIVE_INHIBIT_PIN, 0);
    }
}

bool Io::drive_is_inhibited() {
    return gpio_get(DRIVE_INHIBIT_PIN);
}

// CHARGE_INHIBIT output

void Io::enable_charge_inhibit() {
    printf("Enabling charge inhibit\n");
    if ( CHARGE_INHIBIT_ACTIVE_LOW ) {
        gpio_put(CHARGE_INHIBIT_PIN, 0);
    } else {
        gpio_put(CHARGE_INHIBIT_PIN, 1);
    }
}

void Io::disable_charge_inhibit() {
    printf("Disabling charge inhibit\n");
    if ( CHARGE_INHIBIT_ACTIVE_LOW ) {
        gpio_put(CHARGE_INHIBIT_PIN, 0);
    } else {
        gpio_put(CHARGE_INHIBIT_PIN, 1);
    }
}

bool Io::charge_is_inhibited() {
    return gpio_get(CHARGE_INHIBIT_PIN);
}

// HEATER output

void Io::enable_heater() {
    printf("Enabling heater\n");
    if ( HEATER_ENABLE_ACTIVE_LOW ) {
        gpio_put(HEATER_ENABLE_PIN, 0);
    } else {
        gpio_put(HEATER_ENABLE_PIN, 1);
    }
}

void Io::disable_heater() {
    printf("Disabling heater\n");
    if ( HEATER_ENABLE_ACTIVE_LOW ) {
        gpio_put(HEATER_ENABLE_PIN, 1);
    } else {
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


