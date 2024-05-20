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

#include "include/bms.h"
#include "include/shunt.h"

extern Shunt shunt;

struct repeating_timer updateSocTimer;

bool update_soc(struct repeating_timer *t) {
    extern Bms bms;
    bms.recalculate_soc();
    return true;
}

Bms::Bms(Battery* _battery, Io* _io) {
    battery = _battery;
    state = &state_standby;
    io = _io;
    internalError = false;

    // It's excessive to update the SoC every time we get a message from the ISA
    // shunt. Just update at a regular interval.
    add_repeating_timer_ms(5000, update_soc, NULL, &updateSocTimer);
}

void Bms::set_state(State newState, std::string reason) {
    std::string oldStateName = get_state_name(state);
    std::string newStateName = get_state_name(newState);
    printf("Switching from state %s to state %s, reason : %s\n", oldStateName.c_str(), newStateName.c_str(), reason.c_str());
    state = newState;
    // Change light blinking pattern based on state
    if ( state == state_standby ) {
        statusLight.set_mode(STANDBY);
    } else if ( state == state_drive ) {
        statusLight.set_mode(DRIVE);
    } else if ( state == state_batteryHeating ) {
        statusLight.set_mode(CHARGING);
    } else if ( state == state_charging ) {
        statusLight.set_mode(CHARGING);
    } else if ( state == state_batteryEmpty ) {
        statusLight.set_mode(FAULT);
    } else if ( state == state_overTempFault ) {
        statusLight.set_mode(FAULT);
    } else if ( state == state_illegalStateTransitionFault ) {
        statusLight.set_mode(FAULT);
    } else {
        statusLight.set_mode(FAULT);
    }
}

State Bms::get_state() {
    return state;
}

void Bms::send_event(Event event) {
    state(event);
}

// Watchdog

void Bms::set_watchdog_reboot(bool value) {
    watchdogReboot = value;
}

// DRIVE_INHIBIT

void Bms::enable_drive_inhibit() {
    io->enable_drive_inhibit();
}

void Bms::disable_drive_inhibit() {
    io->disable_drive_inhibit();
}

bool Bms::drive_is_inhibited() {
    return io->drive_is_inhibited();
}

// CHARGE_INHIBIT

void Bms::enable_charge_inhibit() {
    io->enable_charge_inhibit();
}

void Bms::disable_charge_inhibit() {
    io->disable_charge_inhibit();
}

bool Bms::charge_is_inhibited() {
    return io->charge_is_inhibited();
}

// HEATER

void Bms::enable_heater() {
    io->enable_heater();
}

void Bms::disable_heater() {
    io->disable_heater();
}

bool Bms::heater_is_enabled() {
    return io->heater_is_enabled();
}

// IGNITION

bool Bms::ignition_is_on() {
    return io->ignition_is_on();
}

// CHARGE_ENABLE

bool Bms::charge_is_enabled() {
    return io->charge_enable_is_on();
}

// SoC

uint8_t Bms::get_soc() {
    return soc;
}

/*
 * Recalculate the SoC based on the latest data from the ISA shunt.
 *
 * 0 khw/ah == 100% charged. Value goes negative as we draw energy from the pack.
 */
void Bms::recalculate_soc() {
    if ( CALCULATE_SOC_FROM_AMP_SECONDS == 1 ) {
        soc = 100 * (BATTERY_CAPACITY_AS + shunt.get_ampSeconds()) / BATTERY_CAPACITY_AS;
    } else {
        soc = 100 * (BATTERY_CAPACITY_WH + shunt.get_wattHours()) / BATTERY_CAPACITY_WH;
    }
}

// Error

void Bms::set_internal_error() {
    internalError = true;
}

void Bms::clear_internal_error() {
    internalError = false;
}

// Combine error bits into error byte to send out in status CAN message
uint8_t Bms::get_error_byte() {
    return (
        0x00 | \
        internalError | \
        battery->packs_are_imbalanced() << 1 | \
        shunt.is_dead() << 2
    );
}

// Combine status bits into status byte to send out in status CAN message
uint8_t Bms::get_status_byte() {
    return (
        0x00 | \
        charge_is_inhibited() | \
        drive_is_inhibited() << 1 | \
        heater_is_enabled() << 2 | \
        ignition_is_on() << 3 | \
        charge_is_enabled() << 4
    );
}

// Charging

void Bms::update_max_charge_current() {
    float highestTemperature = battery->get_highest_sensor_temperature();
    if ( highestTemperature > CHARGE_THROTTLE_TEMP_LOW ) {
        float degreesOver = highestTemperature - CHARGE_THROTTLE_TEMP_LOW;
        float scaleFactor = 1 - (degreesOver / (CHARGE_THROTTLE_TEMP_HIGH - CHARGE_THROTTLE_TEMP_LOW));
        float chargeCurrent = (scaleFactor * (CHARGE_CURRENT_MAX - CHARGE_CURRENT_MIN)) + CHARGE_CURRENT_MIN;
        maxChargeCurrent = static_cast<int>(chargeCurrent);
    } else {
        maxChargeCurrent = static_cast<int>(CHARGE_CURRENT_MAX);
    }
}

int8_t Bms::get_max_charge_current() {
    return maxChargeCurrent;
}

void Bms::update_max_discharge_current() {
    // FIXME actual implementation
    maxDischargeCurrent = 100;
}

int8_t Bms::Bms::get_max_discharge_current() {
    return maxDischargeCurrent;
}

// statusLight

void Bms::led_blink() {
    statusLight.led_blink();
}