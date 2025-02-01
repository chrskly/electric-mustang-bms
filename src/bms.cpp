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
#include "include/util.h"

#include "settings.h"

extern mutex_t canMutex;

/*
 * Recalculate the SoC of the battery periodically.
 */

struct repeating_timer updateSocTimer;

bool update_soc(struct repeating_timer *t) {
    extern Bms bms;
    bms.recalculate_soc();
    return true;
}

/*
 * Perform all health checks periodically.
 */

struct repeating_timer healthCheckTimer;

bool run_health_checks(struct repeating_timer *t) {
    extern Bms bms;
    extern Battery battery;
    extern Shunt shunt;

    // Temperature
    if ( battery.too_hot() ) {
        bms.send_event(E_TOO_HOT);
    } else if ( battery.too_cold_to_charge() ) {
        bms.send_event(E_TOO_COLD_TO_CHARGE);
    } else {
        bms.send_event(E_TEMPERATURE_OK);
    }

    // Voltage
    if ( battery.has_empty_cell() ) {
        bms.send_event(E_BATTERY_EMPTY);
    } else if ( battery.has_full_cell() ) {
        bms.send_event(E_BATTERY_FULL);
    } else {
        bms.send_event(E_BATTERY_NOT_EMPTY);
    }

    if ( bms.packs_are_imbalanced() ) {
        bms.send_event(E_PACKS_IMBALANCED);
    } else {
        bms.send_event(E_PACKS_NOT_IMBALANCED);
    }

    // Module liveness
    if ( ! battery.is_alive() ) {
        bms.send_event(E_MODULE_UNRESPONSIVE);
    } else {
        bms.send_event(E_MODULES_ALL_RESPONSIVE);
    }

    // Shunt liveness
    if ( shunt.is_dead() ) {
        bms.send_event(E_SHUNT_UNRESPONSIVE);
    } else {
        bms.send_event(E_SHUNT_RESPONSIVE);
    }
    return true;
}


//// ----
//
// Outbound message handlers
//
//// ----


/*
 * Send CAN messages to ISA shunt to tell it to reset. Resets the kw/ah
 * counters.
 */
void Bms::send_shunt_reset_message() {
    struct can_frame shuntResetFrame;
    zero_frame(&shuntResetFrame);
    shuntResetFrame.can_id = 0x411;
    shuntResetFrame.data[0] = 0x3F;
    shuntResetFrame.data[1] = 0x00;
    shuntResetFrame.data[2] = 0x00;
    shuntResetFrame.data[3] = 0x00;
    shuntResetFrame.data[4] = 0x00;
    shuntResetFrame.data[5] = 0x00;
    shuntResetFrame.data[6] = 0x00;
    shuntResetFrame.data[7] = 0x00;
    this->send_frame(&shuntResetFrame, false);
}


/*
 * Limits message 0x351
 *
 * Follows the SimpBMS format.
 *
 * byte 0 = Charge voltage LSB, scale 0.1, unit V
 * byte 1 = Charge voltage MSB, scale 0.1, unit V
 * byte 2 = Charge current LSB, scale 0.1, unit A
 * byte 3 = Charge current MSB, scale 0.1, unit A
 * byte 4 = Discharge current LSB, scale 0.1, unit A
 * byte 5 = Discharge current MSB, scale 0.1, unit A
 * byte 6 = Discharge voltage LSB, scale 0.1, unit V
 * byte 7 = Discharge voltage MSB, scale 0.1, unit V
 */

struct repeating_timer limitsMessageTimer;

bool send_limits_message(struct repeating_timer *t) {
    extern Bms bms;
    extern Battery battery;
    struct can_frame limitsFrame;
    zero_frame(&limitsFrame);
    limitsFrame.can_id = BMS_LIMITS_MSG_ID;
    limitsFrame.data[0] = (uint8_t)( battery.get_max_voltage() * 10 ) && 0xFF;
    limitsFrame.data[1] = (uint8_t)( battery.get_max_voltage() * 10 ) >> 8;
    limitsFrame.data[2] = (uint8_t)( bms.get_max_charge_current() * 10 ) && 0xFF;
    limitsFrame.data[3] = (uint8_t)( bms.get_max_charge_current() * 10 ) >> 8;
    limitsFrame.data[4] = (uint8_t)( bms.get_max_discharge_current() * 10 ) && 0xFF;
    limitsFrame.data[5] = (uint8_t)( bms.get_max_discharge_current() * 10 ) >> 8;
    limitsFrame.data[6] = (uint8_t)( battery.get_min_voltage() * 10 ) && 0xFF;
    limitsFrame.data[7] = (uint8_t)( battery.get_min_voltage() * 10 ) >> 8;
    bms.send_frame(&limitsFrame, false);
    return true;
}


/*
 * BMS state message 0x352
 *
 * Custom message format (not in SimpBMS)
 *
 * byte 0 = bms state
 *   00 = standby
 *   01 = drive
 *   02 = batteryHeating
 *   03 = charging
 *   04 = batteryEmpty
 *   05 = overTempFault
 *   06 = illegalStateTransitionFault
 *   07 = criticalFault
 *   FF = Undefined error
 * byte 1 = error bits
 *   bit 0 = internalError          - something has gone wrong in the BMS
 *   bit 1 = packsImbalanced        - the voltage between two or more packs varies by an unsafe amount
 *   bit 2 = shuntIsDead            - the shunt has not sent a message in SHUNT_TTL seconds
 *   bit 3 = illegalStateTransition - We tried to transistion between states in an illegal way
 *   bit 4 = module(s) dead         - one or more modules have not sent a message in MODULE_TTL seconds
 *   bit 5 = 
 *   bit 6 = 
 *   bit 7 = 
 * byte 2 = status bits
 *   bit 0 = inhibitCharge
 *   bit 1 = inhibitDrive
 *   bit 2 = heaterEnabled
 *   bit 3 = ignitionOn
 *   bit 4 = chargeEnable
 *   bit 5 = disableRegen
 *   bit 6 =
 *   bit 7 =
 * byte 3 = charge inhibit reason
 *   00 = R_NONE
 *   01 = R_TOO_HOT
 *   02 = R_TOO_COLD
 *   03 = R_BATTERY_FULL
 *   04 = R_BATTERY_EMPTY
 *   05 = R_CHARGING
 *   06 = R_ILLEGAL_STATE_TRANSITION
 * byte 4 = drive inhibit reason
 *   Same mapping as charge inhibit reason
 * byte 5 = welding bits
 *   bit 0 = posContactorWelded   - the positive contactor is welded shut
 *   bit 1 = negContactorWelded   - the negative contactor is welded shut
 *   bit 2 = batt1ContactorWelded - the battery 1 contactor is welded shut
 *   bit 3 = batt2ContactorWelded - the battery 2 contactor is welded shut
 * byte 6 = unused
 * byte 7 = checksum
 */


struct repeating_timer bmsStateTimer;

bool send_bms_state_message(struct repeating_timer *t) {
    extern Bms bms;
    extern Battery battery;
    struct can_frame bmsStateFrame;
    zero_frame(&bmsStateFrame);

    bmsStateFrame.can_id = 0x352;

    if ( bms.get_state() == &state_standby ) {
        bmsStateFrame.data[0] = 0x00;
    } else if ( bms.get_state() == &state_drive ) {
        bmsStateFrame.data[0] = 0x01;
    } else if ( bms.get_state() == &state_batteryHeating ) {
        bmsStateFrame.data[0] = 0x02;
    } else if ( bms.get_state() == &state_charging ) {
        bmsStateFrame.data[0] = 0x03;
    } else if ( bms.get_state() == &state_batteryEmpty ) {
        bmsStateFrame.data[0] = 0x04;
    } else if ( bms.get_state() == &state_overTempFault ) {
        bmsStateFrame.data[0] = 0x05;
    } else if ( bms.get_state() == &state_illegalStateTransitionFault ) {
        bmsStateFrame.data[0] = 0x06;
    } else if ( bms.get_state() == &state_criticalFault ) {
        bmsStateFrame.data[0] = 0x07;
    } else {
        bmsStateFrame.data[0] = 0xFF;
    }

    bmsStateFrame.data[1] = bms.get_error_byte();
    bmsStateFrame.data[2] = bms.get_status_byte();
    bmsStateFrame.data[3] = bms.get_charge_inhibit_reason();
    bmsStateFrame.data[4] = bms.get_drive_inhibit_reason();
    bmsStateFrame.data[5] = bms.get_welding_byte();
    bmsStateFrame.data[6] = 0x00;
    bmsStateFrame.data[7] = 0x00; // checksum
    bms.send_frame(&bmsStateFrame, true);
    return true;
}


/*
 * Module liveness message 0x353
 *
 * Custom message format (not in SimpBMS)
 *
 * byte 0 = modules 0-7 heartbeat status (0 alive, 1 dead)
 * byte 1 = modules 8-15 hearbeat status (0 alive, 1 dead)
 * byte 2 = modules 16-23 heartbeat status (0 alive, 1 dead)
 * byte 3 = modules 24-31 heartbeat status (0 alive, 1 dead)
 * byte 4 = modules 32-39 heartbeat status (0 alive, 1 dead)
 * byte 5 = invalidEventCounter LSB
 * byte 6 = invalidEventCounter MSB
 * byte 7 = checksum
 */

struct repeating_timer moduleLivenessTimer;

bool send_module_liveness_message(struct repeating_timer *t) {
    extern Bms bms;
    extern Battery battery;
    struct can_frame moduleLivenessFrame;
    zero_frame(&moduleLivenessFrame);
    moduleLivenessFrame.can_id = 0x353;
    moduleLivenessFrame.data[0] = battery.get_module_liveness_byte(0);
    moduleLivenessFrame.data[1] = battery.get_module_liveness_byte(8);
    moduleLivenessFrame.data[2] = battery.get_module_liveness_byte(16);
    moduleLivenessFrame.data[3] = battery.get_module_liveness_byte(24);
    moduleLivenessFrame.data[4] = battery.get_module_liveness_byte(32);
    moduleLivenessFrame.data[5] = (uint8_t)bms.get_invalid_event_count() && 0xFF;
    moduleLivenessFrame.data[6] = (uint8_t)bms.get_invalid_event_count() >> 8;
    moduleLivenessFrame.data[7] = 0x00;  // checksum
    bms.send_frame(&moduleLivenessFrame, true);
    return true;
}

/*
 * Can tx/rx error counters message 0x354
 *
 * Custom message format (not in SimpBMS)
 *
 * byte 0 - 3 = can tx error counters (32bit counter)
 * byte 4 - 7 = can rx error counters (32bit counter)
 */

struct repeating_timer canErrorCountersTimer;

bool send_can_error_counters_message(struct repeating_timer *t) {
    extern Bms bms;
    struct can_frame canErrorCountersFrame;
    zero_frame(&canErrorCountersFrame);
    canErrorCountersFrame.can_id = 0x354;
    canErrorCountersFrame.data[0] = bms.getCanTxErrorCount() && 0xFF;
    canErrorCountersFrame.data[1] = bms.getCanTxErrorCount() >> 8 && 0xFF;
    canErrorCountersFrame.data[2] = bms.getCanTxErrorCount() >> 16 && 0xFF;
    canErrorCountersFrame.data[3] = bms.getCanTxErrorCount() >> 24 && 0xFF;
    canErrorCountersFrame.data[4] = bms.getCanRxErrorCount() && 0xFF;
    canErrorCountersFrame.data[5] = bms.getCanRxErrorCount() >> 8 && 0xFF;
    canErrorCountersFrame.data[6] = bms.getCanRxErrorCount() >> 16 && 0xFF;
    canErrorCountersFrame.data[7] = bms.getCanRxErrorCount() >> 24 && 0xFF;
    bms.send_frame(&canErrorCountersFrame, false);
    return true;
}


/*
 * SoC message 0x355
 *
 * Follows the SimpBMS format.
 *
 * byte 0 = SoC LSB, scale 1, unit %
 * byte 1 = SoC MSB, scale 1, unit %
 * byte 2 = SoH LSB, scale 1, unit %
 * byte 3 = SoH MSB, scale 1, unit %
 * byte 4 = SoC LSB, scale 0.01, unit %
 * byte 5 = SoC MSB, scale 0.01, unit %
 * byte 6 = unused
 * byte 7 = unused
 */

struct repeating_timer socMessageTimer;

bool send_soc_message(struct repeating_timer *t) {
    extern Bms bms;
    struct can_frame socFrame;
    zero_frame(&socFrame);
    socFrame.can_id = BMS_SOC_MSG_ID;
    socFrame.data[0] = (uint8_t)bms.get_soc() && 0xFF;            // SoC LSB
    socFrame.data[1] = (uint8_t)bms.get_soc() >> 8;               // SoC MSB
    socFrame.data[2] = 0x00;                                      // SoH, not implemented
    socFrame.data[3] = 0x00;                                      // SoH, not implemented
    socFrame.data[4] = (uint8_t)( bms.get_soc() * 100 ) && 0xFF;  // SoC LSB, scaled
    socFrame.data[5] = (uint8_t)( bms.get_soc() * 100 ) >> 8;     // SoC MSB, scaled
    socFrame.data[6] = 0x00;                                      // unused
    socFrame.data[7] = 0x00;                                      // unused
    bms.send_frame(&socFrame, false);
    return true;
}

/*
 * Status message 0x356
 *
 * More or less follows the SimpBMS format.
 *
 * byte 0 = Voltage LSB, scale 0.01, unit V
 * byte 1 = Voltage MSB, scale 0.01, unit V
 * byte 2 = Current LSB, scale 0.1, unit A
 * byte 3 = Current MSB, scale 0.1, unit A
 * byte 4 = Temperature LSB, scale 0.1, unit C
 * byte 5 = Temperature MSB, scale 0.1, unit C
 * byte 6 = Voltage LSB (measured by shunt), scale 0.01, unit V
 * byte 7 = Voltage MSB (measured by shunt), scale 0.01, unit V
 */

struct repeating_timer statusMessageTimer;

bool send_status_message(struct repeating_timer *t) {
    extern Bms bms;
    extern Battery battery;
    extern Shunt shunt;
    struct can_frame statusFrame;
    zero_frame(&statusFrame);
    statusFrame.can_id = BMS_STATUS_MSG_ID;
    statusFrame.data[0] = (uint8_t)( battery.get_voltage() * 100 ) && 0xFF;
    statusFrame.data[1] = (uint8_t)( battery.get_voltage() * 100 ) >> 8;
    statusFrame.data[2] = (uint8_t)( shunt.get_amps() * 10 ) && 0xFF;
    statusFrame.data[3] = (uint8_t)( shunt.get_amps() * 10 ) >> 8;
    statusFrame.data[4] = battery.get_highest_sensor_temperature() && 0xFF;
    statusFrame.data[5] = (uint8_t)battery.get_highest_sensor_temperature() >> 8;
    statusFrame.data[6] = (uint8_t)( shunt.get_voltage1() * 100 ) && 0xFF;
    statusFrame.data[7] = (uint8_t)( shunt.get_voltage1() * 100 ) >> 8;
    bms.send_frame(&statusFrame, false);
    return true;
}


/*
 * Alarms message 0x35A
 *
 * Follows the SimpBMS format. Note the docs disagree with the code. So,
 * following the code.
 *
 * First 4 bytes are alarms, second 4 bytes are warnings.
 *
 * byte 0
 *   bit 0
 *   bit 1
 *   bit 2 = high cell alarm
 *   bit 3
 *   bit 4 = low cell alarm
 *   bit 5
 *   bit 6 = high temp alarm
 *   bit 7
 * byte 1
 *   bit 0 = low temp alarm
 * byte 2
 * byte 3
 *   bit 0 = cell delta alarm
 * byte 4
 *   bit 0
 *   bit 1
 *   bit 2 = high cell warn
 *   bit 3
 *   bit 4 = low cell warn
 *   bit 5
 *   bit 6 = high temp warn
 * byte 5
 *   bit 0 = low temp warn
 * byte 6
 * byte 7 = checksum
 */

struct repeating_timer alarmMessageTimer;

bool send_alarm_message(struct repeating_timer *t) {
    extern Bms bms;
    extern Battery battery;
    struct can_frame alarmFrame;
    zero_frame(&alarmFrame);
    alarmFrame.can_id = BMS_ALARM_MSG_ID;
    // Set undervolt bit (3)
    if ( battery.has_empty_cell() ) {
        alarmFrame.data[0] |= 0x04;
    }
    // Set overvolt bit (4)
    if ( battery.has_full_cell() ) {
        alarmFrame.data[0] |= 0x08;
    }
    alarmFrame.data[1] = 0x00;
    // Set over temp bit (7)
    if ( battery.too_hot() ) {
        alarmFrame.data[1] |= 0x40;
    }
    alarmFrame.data[2] = 0x00;
    alarmFrame.data[3] = 0x00;
    alarmFrame.data[4] = 0x00;
    alarmFrame.data[5] = 0x00;
    alarmFrame.data[6] = 0x00;
    alarmFrame.data[7] = 0x00;
    bms.send_frame(&alarmFrame, true);
    return true;
}


//// ----
//
// Inbound message handlers
//
//// ----


// Handle messages coming in on the main CAN bus

struct repeating_timer handleMainCANMessageTimer;

bool handle_main_CAN_messages(struct repeating_timer *t) {
    struct can_frame m;
    extern Shunt shunt;
    extern Bms bms;
    if ( bms.read_frame(&m) ) {
        switch ( m.can_id ) {
            // ISA shunt amps
            case 0x521:
                shunt.set_amps( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) );
                shunt.heartbeat();
                break;
            // ISA shunt voltage 1
            case 0x522:
                shunt.set_voltage1( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) / 1000.0f );
                shunt.heartbeat();
                break;
            // ISA shunt voltage 2
            case 0x523:
                shunt.set_voltage2( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) / 1000.0f );
                shunt.heartbeat();
                break;
            // ISA shunt voltage 3
            case 0x524:
                shunt.set_voltage3( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) / 1000.0f );
                shunt.heartbeat();
                break;
            // ISA shunt temperature
            case 0x525:
                shunt.set_temperature( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) / 10 );
                shunt.heartbeat();
                break;
            // ISA shunt kilowatts
            case 0x526:
                shunt.set_watts( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) / 1000.0f );
                shunt.heartbeat();
                break;
            // ISA shunt amp-hours
            case 0x527:
                shunt.set_ampSeconds( (int32_t)(m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) );
                shunt.heartbeat();
                break;
            // ISA shunt kilowatt-hours
            case 0x528:
                shunt.set_wattHours( (int32_t)( (m.data[5] << 24) | (m.data[4] << 16) | (m.data[3] << 8) | (m.data[2]) ) );
                shunt.heartbeat();
                break;
            default:
                break;
        }
    }
    return true;
}




Bms::Bms(Battery* _battery, Io* _io, Shunt* _shunt) {
    battery = _battery;
    state = &state_standby;
    io = _io;
    shunt = _shunt;
    internalError = false;
    statusLight = StatusLight(this);
    chargeInhibitReason = R_NONE;
    driveInhibitReason = R_NONE;

    printf("[bms][init] setting up main CAN port\n");
    CAN = new MCP2515(SPI_PORT, MAIN_CAN_CS, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
    MCP2515::ERROR result = CAN->reset();
    if ( result != MCP2515::ERROR_OK ) {
        printf("[bms][init] WARNING problem resetting main CAN port : %d\n", result);
    }
    result = CAN->setBitrate(CAN_500KBPS, MCP_8MHZ);
    if ( result != MCP2515::ERROR_OK ) {
        printf("[bms][init] WARNING problem setting bitrate on main CAN port : %d\n", result);
    }
    result = CAN->setNormalMode();
    if ( result != MCP2515::ERROR_OK ) {
        printf("[bms][init] WARNING problem setting normal mode on main CAN port : %d\n", result);
    }
    printf("[bms][init] main CAN port memory address : %p\n", CAN);

    /*
    printf("[bms][init] sending 5 test messages\n");
    for ( int i = 0; i < 5; i++ ) {
        can_frame m;
        m.can_id = 0x100 + i;
        m.can_dlc = 8;
        for ( int j = 0; j < 8; j++ ) {
            m.data[j] = j;
        }
        this->send_frame(&m, true);
    }
    */

    printf("[bms][init] enabling CAN message handlers\n");
    // limits (out)
    add_repeating_timer_ms(1000, send_limits_message, NULL, &limitsMessageTimer);
    // bms state (out)
    add_repeating_timer_ms(1000, send_bms_state_message, NULL, &bmsStateTimer);
    // module liveness (out)
    add_repeating_timer_ms(5000, send_module_liveness_message, NULL, &moduleLivenessTimer);
    // can error counters (out)
    add_repeating_timer_ms(1000, send_can_error_counters_message, NULL, &canErrorCountersTimer);
    // soc (out)
    add_repeating_timer_ms(1000, send_soc_message, NULL, &socMessageTimer);
    // status (out)
    add_repeating_timer_ms(1000, send_status_message, NULL, &statusMessageTimer);
    // Alarms (out)
    add_repeating_timer_ms(1000, send_alarm_message, NULL, &alarmMessageTimer);
    // main CAN (in)
    add_repeating_timer_ms(5, handle_main_CAN_messages, NULL, &handleMainCANMessageTimer);

    // It's excessive to update the SoC every time we get a message from the ISA
    // shunt. Just update at a regular interval.
    printf("[bms][init] enabling SoC update timer\n");
    add_repeating_timer_ms(500, update_soc, NULL, &updateSocTimer);

    // health checks
    add_repeating_timer_ms(100, run_health_checks, NULL, &healthCheckTimer);
}

void Bms::set_state(State newState, std::string reason) {
    std::string oldStateName = get_state_name(state);
    std::string newStateName = get_state_name(newState);
    printf("[bms][set_state] switching from state %s to state %s, reason : %s\n", oldStateName.c_str(), newStateName.c_str(), reason.c_str());
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
    } else if ( state == state_criticalFault ) {
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

void Bms::print() {
    std::string chg_inh = io->charge_is_inhibited() ? "true" : "false";
    std::string drv_inh = io->drive_is_inhibited() ? "true" : "false";
    std::string ign = io->ignition_is_on() ? "true" : "false";
    std::string chg_en = io->charge_enable_is_on() ? "true" : "false";
    int8_t Tmax = battery->get_highest_sensor_temperature();
    int8_t Tmin = battery->get_lowest_sensor_temperature();
    int16_t Vmax = battery->get_highest_cell_voltage();
    int16_t Vmin = battery->get_lowest_cell_voltage();
    printf("State:%s, SoC:%d, DRV_INH:%s, CHG_INH:%s, IGN:%s, CHG_EN:%s\n",
        get_state_name(get_state()), soc, drv_inh.c_str(), chg_inh.c_str(), ign.c_str(), chg_en.c_str());
    printf(" V:%d, VMax:%d, VMin:%d\n", battery->get_voltage()/1000, Vmax, Vmin );
    printf(" TMax:%d, TMin:%d\n", Tmax, Tmin );
    battery->print();
}

// Watchdog

void Bms::set_watchdog_reboot(bool value) {
    watchdogReboot = value;
}

// DRIVE_INHIBIT

void Bms::enable_drive_inhibit(std::string context, InhibitReason reason) {
    if ( !drive_is_inhibited() ) {
        set_drive_inhibit_reason(reason);
        io->enable_drive_inhibit(context);
    }
}

void Bms::disable_drive_inhibit(std::string context) {
    clear_drive_inhibit_reason();
    if ( drive_is_inhibited() ) {
        io->disable_drive_inhibit(context);
    }
}

bool Bms::drive_is_inhibited() {
    return io->drive_is_inhibited();
}

void Bms::set_drive_inhibit_reason(InhibitReason reason) {
    driveInhibitReason = reason;
}

void Bms::clear_drive_inhibit_reason() {
    driveInhibitReason = R_NONE;
}

int8_t Bms::get_drive_inhibit_reason() {
    return driveInhibitReason;
}

// CHARGE_INHIBIT

void Bms::enable_charge_inhibit(std::string context, InhibitReason reason) {
    if ( !charge_is_inhibited() ) {
        set_charge_inhibit_reason(reason);
        io->enable_charge_inhibit(context);
    }
}

void Bms::disable_charge_inhibit(std::string context) {
    clear_charge_inhibit_reason();
    if ( charge_is_inhibited() ) {
        io->disable_charge_inhibit(context);
    }
}

bool Bms::charge_is_inhibited() {
    return io->charge_is_inhibited();
}

void Bms::set_charge_inhibit_reason(InhibitReason reason) {
    chargeInhibitReason = reason;
}

void Bms::clear_charge_inhibit_reason() {
    chargeInhibitReason = R_NONE;
}

int8_t Bms::get_charge_inhibit_reason() {
    return chargeInhibitReason;
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
        soc = 100 * (BATTERY_CAPACITY_AS + shunt->get_ampSeconds()) / BATTERY_CAPACITY_AS;
    } else {
        soc = 100 * (BATTERY_CAPACITY_WH + shunt->get_wattHours()) / BATTERY_CAPACITY_WH;
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
        shunt->is_dead() << 2 | \
        illegalStateTransition << 3 | \
        ! battery->is_alive() << 4
    );
}

/* Combine status bits into status byte to send out in status CAN message
 * bit 0 = charge inhibited
 * bit 1 = drive inhibited
 * bit 2 = heater enabled
 * bit 3 = ignition on
 * bit 4 = charge enabled
 * bit 5 = regen not allowed
 * bit 6 =
 * bit 7 =
 */
uint8_t Bms::get_status_byte() {
    return (
        0x00 | \
        charge_is_inhibited() | \
        drive_is_inhibited() << 1 | \
        heater_is_enabled() << 2 | \
        ignition_is_on() << 3 | \
        charge_is_enabled() << 4 | \
        regen_not_allowed() << 5
    );
}

void Bms::increment_invalid_event_count() {
    invalidEventCounter++;
}

uint8_t Bms::get_welding_byte() {
    return (
        0x00 | \
        posContactorWelded | \
        negContactorWelded << 1 | \
        packContactorsWelded[0] << 2 | \
        packContactorsWelded[1] << 3
    );

}

void Bms::do_welding_checks() {
    posContactorWelded = io->pos_contactor_is_welded();
    negContactorWelded = io->neg_contactor_is_welded();
    packContactorsWelded[0] = battery->contactor_is_welded(0);
    packContactorsWelded[1] = battery->contactor_is_welded(1);
}

// Charging

// FIXME account for inhibited packs
int8_t Bms::get_max_charge_current_by_soc() {
    return 0;
}

void Bms::update_max_charge_current() {
    // Safeties
    if ( battery->too_hot() || charge_is_inhibited() ) {
        maxChargeCurrent = 0;
        return;
    }
    maxChargeCurrent = std::min(battery->get_max_charge_current_by_temperature(), get_max_charge_current_by_soc());
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

// Track when the pack voltages match each other
void Bms::pack_voltages_match_heartbeat() {
    lastTimePackVoltagesMatched = get_clock();
}

bool Bms::packs_are_imbalanced() {
    return ( get_clock() - lastTimePackVoltagesMatched ) > PACKS_IMBALANCED_TTL;
}


// Comms

bool Bms::send_frame(can_frame* frame, bool doChecksum) {
    for ( int t = 0; t < SEND_FRAME_RETRIES; t++ ) {
        // printf("[bms][send_frame] 0x%03X  [ ", frame->can_id);
        // for ( int i = 0; i < frame->can_dlc; i++ ) {
        //     printf("%02X ", frame->data[i]);
        // }
        // printf("]\n");

        if ( doChecksum ) {
            // Calculate XOR checksum
            frame->data[7] = 0;
            for ( int i = 0; i < 7; i++ ) {
                frame->data[7] ^= frame->data[i];
            }
        }

        if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
            incrementCanTxErrorCount();
            continue;
        }

        MCP2515::ERROR result = this->CAN->sendMessage(frame);
        mutex_exit(&canMutex);

        // Sending failed, try again
        if ( result != MCP2515::ERROR_OK ) {
            if ( result == MCP2515::ERROR_FAIL ) {
                printf(" [send_frame %d] ERROR_FAIL, try again\n", t);
                incrementCanTxErrorCount();
            } else if ( result == MCP2515::ERROR_ALLTXBUSY ) {
                printf(" [send_frame %d] ERROR_ALLTXBUSY, try again\n", t);
                incrementCanTxErrorCount();
            } else if ( result == MCP2515::ERROR_FAILINIT ) {
                printf(" [send_frame %d] ERROR_FAILINIT, try again\n", t);
                incrementCanTxErrorCount();
            } else if ( result == MCP2515::ERROR_FAILTX ) {
                printf(" [send_frame %d] ERROR_FAILTX, try again\n", t);
                incrementCanTxErrorCount();
            } else if ( result == MCP2515::ERROR_NOMSG ) {
                printf(" [send_frame %d] ERROR_NOMSG, try again\n", t);
                incrementCanTxErrorCount();
            }
            continue;
        }
        // Frame was sent
        return true;
    }
    // Failed to send after all retries
    return false;
}

bool Bms::read_frame(can_frame* frame) {
    for ( int t = 0; t < READ_FRAME_RETRIES; t++ ) {    
        if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
            incrementCanRxErrorCount();
            return false;
        }
        MCP2515::ERROR result = this->CAN->readMessage(frame);
        mutex_exit(&canMutex);
        if ( result != MCP2515::ERROR_OK ) {
            if ( result == MCP2515::ERROR_FAIL ) {
                printf("[bms][read_frame] %d/%d ERROR_FAIL, try again\n", t, READ_FRAME_RETRIES);
                incrementCanRxErrorCount();
            } else if ( result == MCP2515::ERROR_ALLTXBUSY ) {
                printf("[bms][read_frame] %d/%d ERROR_ALLTXBUSY, try again\n", t, READ_FRAME_RETRIES);
                incrementCanRxErrorCount();
            } else if ( result == MCP2515::ERROR_FAILINIT ) {
                printf("[bms][read_frame] %d/%d ERROR_FAILINIT, try again\n", t, READ_FRAME_RETRIES);
                incrementCanRxErrorCount();
            } else if ( result == MCP2515::ERROR_FAILTX ) {
                printf("[bms][read_frame] %d/%d ERROR_FAILTX, try again\n", t, READ_FRAME_RETRIES);
                incrementCanRxErrorCount();
            } else if ( result == MCP2515::ERROR_NOMSG ) {
                return true;
            }
            continue;
        }
        // Frame was read, print it out
        // printf("[bms][read_frame] 0x%03X  [ ", frame->can_id);
        // for ( int i = 0; i < frame->can_dlc; i++ ) {
        //     printf("%02X ", frame->data[i]);
        // }
        // printf("]\n");
        return true;
    }
    // Failed to read after all retries
    return false;
}