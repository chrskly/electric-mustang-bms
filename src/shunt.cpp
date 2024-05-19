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

#include <time.h>
#include "include/shunt.h"
#include "include/util.h"
#include "settings.h"

Shunt::Shunt() {
    amps = 0;
    voltage1 = 0;
    voltage2 = 0;
    voltage3 = 0;
    temperature = 0;
    watts = 0;
    ampSeconds = 0;
    wattHours = 0;
}

void Shunt::heartbeat() {
    lastHeartbeat = get_clock();
}

bool Shunt::is_dead() {
    return !( ((double)(get_clock() - lastHeartbeat) / CLOCKS_PER_SEC) < SHUNT_TTL );
}

int32_t Shunt::get_amps() {
    return amps;
}

void Shunt::set_amps(int32_t _amps) {
    amps = _amps;
}

int32_t Shunt::get_voltage1() {
    return voltage1;
}

void Shunt::set_voltage1(int32_t _voltage1) {
    voltage1 = _voltage1;
}

int32_t Shunt::get_voltage2() {
    return voltage2;
}

void Shunt::set_voltage2(int32_t _voltage2) {
    voltage2 = _voltage2;
}

int32_t Shunt::get_voltage3() {
    return voltage3;
}

void Shunt::set_voltage3(int32_t _voltage3) {
    voltage3 = _voltage3;
}

int32_t Shunt::get_temperature() {
    return temperature;
}

void Shunt::set_temperature(int32_t _temperature) {
    temperature = _temperature;
}

int32_t Shunt::get_watts() {
    return watts;
}

void Shunt::set_watts(int32_t _watts) {
    watts = _watts;
}

int32_t Shunt::get_ampSeconds() {
    return ampSeconds;
}

void Shunt::set_ampSeconds(int32_t _ampSeconds) {
    ampSeconds = _ampSeconds;
}

int32_t Shunt::get_wattHours() {
    return wattHours;
}

void Shunt::set_wattHours(int32_t _wattHours) {
    wattHours = _wattHours;
}



