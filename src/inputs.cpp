/*
 * This file is part of the ev mustang bms project.
 *
 * Copyright (C) 2022 Christian Kelly <chrskly@chrskly.com>
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
#include "include/inputs.h"

void gpio_callback(uint gpio, uint32_t events) {
    extern State state;
    if ( gpio == IGNITION_ENABLE_PIN ) {
        if ( gpio_get(IGNITION_ENABLE_PIN) == 1 ) {
            state(E_IGNITION_ON);
        } else {
            state(E_IGNITION_OFF);
        }
    }
    if ( gpio == CHARGE_ENABLE_PIN ) {
        if ( gpio_get(CHARGE_ENABLE_PIN) == 1 ) {
            state(E_CHARGING_INITIATED);
        } else {
            state(E_CHARGING_TERMINATED);
        }
    }
}

void enable_listen_for_ignition_signal() {
    gpio_set_irq_enabled_with_callback(IGNITION_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

void enable_listen_for_charge_enable_signal() {
    // gpio_set_irq_enabled_with_callback(CHARGE_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &charge_enable_callback);
    gpio_set_irq_enabled(CHARGE_ENABLE_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}



