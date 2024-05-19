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
#include "include/io.h"
#include "include/battery.h"
#include "settings.h"

// Input handlers

void gpio_callback(uint gpio, uint32_t events) {
    extern Battery battery;
    if ( gpio == DRIVE_INHIBIT_PIN ) {
        if ( gpio_get(DRIVE_INHIBIT_PIN) == 1 ) {
            battery.get_bms()->set_inhibitDrive(true);
        } else {
            battery.get_bms()->set_inhibitDrive(false);
        }
    }
    if ( gpio == CHARGE_INHIBIT_PIN ) {
        if ( gpio_get(CHARGE_INHIBIT_PIN) == 1 ) {
            battery.get_bms()->set_inhibitCharge(true);
        } else {
            battery.get_bms()->set_inhibitCharge(false);
        }
    }
    if ( gpio == INHIBIT_CONTACTOR_PINS[0] ) { // batt1 inhibit
        if ( gpio_get(INHIBIT_CONTACTOR_PINS[0]) == 1 ) {
            battery.get_pack(0).set_inhibit(true);
        } else {
            battery.get_pack(0).set_inhibit(false);
        }
    }
    if ( gpio == INHIBIT_CONTACTOR_PINS[1] ) { // batt2 inhibit
        if ( gpio_get(INHIBIT_CONTACTOR_PINS[1]) == 1 ) {
            battery.get_pack(1).set_inhibit(true);
        } else {
            battery.get_pack(1).set_inhibit(false);
        }
    }
}

void enable_listen_for_input_signals() {
    gpio_set_pulls(DRIVE_INHIBIT_PIN, !DRIVE_INHIBIT_ACTIVE_LOW, DRIVE_INHIBIT_ACTIVE_LOW);
    gpio_set_pulls(CHARGE_INHIBIT_PIN, !CHARGE_INHIBIT_ACTIVE_LOW, CHARGE_INHIBIT_ACTIVE_LOW);
    gpio_set_pulls(INHIBIT_CONTACTOR_PINS[0], !INHIBIT_CONTACTOR_ACTIVE_LOW, INHIBIT_CONTACTOR_ACTIVE_LOW);
    gpio_set_pulls(INHIBIT_CONTACTOR_PINS[1], !INHIBIT_CONTACTOR_ACTIVE_LOW, INHIBIT_CONTACTOR_ACTIVE_LOW);
    gpio_set_irq_enabled_with_callback(DRIVE_INHIBIT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(CHARGE_INHIBIT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(INHIBIT_CONTACTOR_PINS[0], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(INHIBIT_CONTACTOR_PINS[1], GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}


// Outputs

void set_ignition_state(bool state) {
    printf("Setting ignition state to %d\n", state);
    if ( IGNITION_ENABLE_ACTIVE_LOW == state ) {
        gpio_put(IGNITION_ENABLE_PIN, 0);
    } else {
        gpio_put(IGNITION_ENABLE_PIN, 1);
    }
}

void set_charge_enable_state(bool state) {
    printf("Setting charge enable state to %d\n", state);
    if ( CHARGE_ENABLE_ACTIVE_LOW == state ) {
        gpio_put(CHARGE_ENABLE_PIN, 0);
    } else {
        gpio_put(CHARGE_ENABLE_PIN, 1);
    }
}