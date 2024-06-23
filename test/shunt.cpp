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
#include "include/shunt.h"

struct repeating_timer handleShuntSendTimer;

bool handle_shunt_send(struct repeating_timer *t) {
    extern Shunt shunt;
    shunt.send_ampSeconds();
    shunt.send_wattHours();
    return true;
}

Shunt::Shunt() {
    printf("[shunt] Shunt object created\n");
}

void Shunt::set_CAN_port(MCP2515* _CAN) {
    CAN = _CAN;
}

void Shunt::enable() {
    printf("[shunt] Enabling shunt\n");
    add_repeating_timer_ms(1000, handle_shunt_send, NULL, &handleShuntSendTimer);
}

void Shunt::set_ampSeconds(int32_t newAmpSeconds) {
    ampSeconds = newAmpSeconds;
}

void Shunt::send_ampSeconds() {
    frame.can_id = 0x527;
    frame.can_dlc = 8;
    frame.data[0] = (ampSeconds >> 24) & 0xFF;
    frame.data[1] = (ampSeconds >> 16) & 0xFF;
    frame.data[2] = (ampSeconds >> 8) & 0xFF;
    frame.data[3] = ampSeconds & 0xFF;
    CAN->sendMessage(&frame);
}

void Shunt::set_wattHours(int32_t newWattHours) {
    wattHours = newWattHours;
}

void Shunt::send_wattHours() {
    frame.can_id = 0x528;
    frame.can_dlc = 8;
    frame.data[0] = (wattHours >> 24) & 0xFF;
    frame.data[1] = (wattHours >> 16) & 0xFF;
    frame.data[2] = (wattHours >> 8) & 0xFF;
    frame.data[3] = wattHours & 0xFF;
    CAN->sendMessage(&frame);
}

