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
#include "include/inputs.h"
#include "include/battery.h"
#include "settings.h"

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
}

void enable_listen_for_drive_inhibit_signal() {
    gpio_set_irq_enabled_with_callback(DRIVE_INHIBIT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

void enable_listen_for_charge_inhibit_signal() {
    gpio_set_irq_enabled(CHARGE_INHIBIT_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}