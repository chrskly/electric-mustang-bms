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
#include <cstdint>
#include "include/bms.h"

Bms::Bms() {}

void Bms::set_state(BmsState new_state) {
    state = new_state;
}

BmsState Bms::get_state() {
    return state;
}

void Bms::set_internalError(bool new_internalError) {
    internalError = new_internalError;
}

void Bms::set_packsImbalanced(bool new_packsImbalanced) {
    packsAreImbalanced = new_packsImbalanced;
}

bool Bms::get_packsImbalanced() {
    return packsAreImbalanced;
}

void Bms::set_inhibitCharge(bool new_inhibitCharge) {
    inhibitCharge = new_inhibitCharge;
}

bool Bms::get_inhibitCharge() {
    return inhibitCharge;
}

void Bms::set_inhibitDrive(bool new_inhibitDrive) {
    inhibitDrive = new_inhibitDrive;
}

bool Bms::get_inhibitDrive() {
    return inhibitDrive;
}

void Bms::set_heaterEnabled(bool new_heaterEnabled) {
    heaterEnabled = new_heaterEnabled;
}

bool Bms::get_heaterEnabled() {
    return heaterEnabled;
}

void Bms::set_ignitionOn(bool new_ignitionOn) {
    ignitionOn = new_ignitionOn;
}

void Bms::set_chargeEnable(bool new_chargeEnable) {
    chargeEnable = new_chargeEnable;
}

void Bms::set_soc(int16_t new_soc) {
    soc = new_soc;
}

int8_t Bms::get_soc() {
    return soc;
}

void Bms::set_voltage(uint16_t new_voltage) {
    voltage = new_voltage;
}

void Bms::set_amps(uint16_t new_amps) {
    amps = new_amps;
}

void Bms::set_temperature(uint16_t new_temperature) {
    temperature = new_temperature;
}

void Bms::set_maxVoltage(uint16_t new_maxVoltage) {
    maxVoltage = new_maxVoltage;
}

void Bms::set_minVoltage(uint16_t new_minVoltage) {
    minVoltage = new_minVoltage;
}

void Bms::set_maxChargeCurrent(int16_t new_maxChargeCurrent) {
    maxChargeCurrent = new_maxChargeCurrent;
}

void Bms::set_maxDischargeCurrent(int16_t new_maxDischargeCurrent) {
    maxDischargeCurrent = new_maxDischargeCurrent;
}

void Bms::set_moduleLiveness(int64_t new_moduleLiveness) {
    moduleLiveness = new_moduleLiveness;
}