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

#ifndef BMS_TEST_INCLUDE_BMS_H_
#define BMS_TEST_INCLUDE_BMS_H_

#include "hardware/timer.h"
#include "include/shunt.h"
#include "mcp2515/mcp2515.h"
#include "include/battery.h"

enum BmsState : uint8_t {
    STATE_STANDBY,
    STATE_DRIVE,
    STATE_BATTERY_HEATING,
    STATE_CHARGING,
    STATE_BATTERY_EMPTY,
    STATE_OVER_TEMP_FAULT,
    STATE_ILLEGAL_STATE_TRANSITION_FAULT,
};

class Bms {
    private:
        int8_t soc;
        BmsState state;                // Internal state of the BMS [ idle, drive, charge, etc.]
        bool internalError;           // Something has gone wrong with the BMS
        bool packsAreImbalanced;      // The voltage between the two packs is too damn high
        bool inhibitCharge;           // Indicates that the BMS INHIBIT_CHARGE signal is enabled
        bool inhibitDrive;            // Indicates that the BMS INHIBIT_DRIVE signal is enabled
        bool heaterEnabled;           // Indicates that the battery heater is currently enabled
        bool chargeEnable;            // Charger is asking to charge
        bool ignitionOn;              // BMS thinks the igntion is on
        uint16_t voltage;             // Total voltage of the battery
        uint16_t amps;                // Current being drawn from the battery
        uint16_t temperature;         // Temperature of the battery
        uint16_t maxVoltage;          // Maximum voltage of the battery
        uint16_t minVoltage;          // Minimum voltage of the battery
        int16_t maxChargeCurrent;     // Maximum current that can be drawn from the battery
        int16_t maxDischargeCurrent;  // Maximum current that can be drawn from the battery
        int64_t moduleLiveness;       // Bitfield of module liveness
        MCP2515* CAN;
        Shunt* shunt;
        Battery* battery;

    public:
        Bms();
        Bms(Shunt* _shunt);
        void set_state(uint8_t new_state);
        BmsState get_state();
        void set_internalError(bool new_internalError);
        void set_packsImbalanced(bool new_packsImbalanced);
        bool get_packsImbalanced();
        void set_inhibitCharge(bool new_inhibitCharge);
        bool get_inhibitCharge();
        void set_inhibitDrive(bool new_inhibitDrive);
        bool get_inhibitDrive();
        void set_heaterEnabled(bool new_heaterEnabled);
        bool get_heaterEnabled();
        void set_ignitionOn(bool new_ignitionOn);
        void set_chargeEnable(bool new_chargeEnable);
        void set_soc(int16_t new_soc);
        int8_t get_soc();
        void set_voltage(uint16_t new_voltage);
        void set_amps(uint16_t new_amps);
        void set_temperature(uint16_t new_temperature);
        void set_maxVoltage(uint16_t new_maxVoltage);
        void set_minVoltage(uint16_t new_minVoltage);
        void set_maxChargeCurrent(int16_t new_maxChargeCurrent);
        void set_maxDischargeCurrent(int16_t new_maxDischargeCurrent);
        void set_moduleLiveness(int64_t new_moduleLiveness);
        Battery* get_battery();
        void set_battery(Battery* _battery);
        bool send_frame(can_frame* frame);
        bool read_frame(can_frame* frame);
};

#endif  // BMS_TEST_INCLUDE_BMS_H_
