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
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/watchdog.h"

#include "mcp2515/mcp2515.h"
#include "include/bmstest.h"
#include "include/battery.h"
#include "include/comms.h"
#include "include/io.h"

#include "include/testcases0xx.h"
#include "include/testcases1xx.h"
#include "include/testcases2xx.h"

MCP2515 mainCAN(SPI_PORT, MAIN_CAN_CS, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);
Battery battery(NUM_PACKS);

int main() {
    stdio_init_all();

    set_sys_clock_khz(80000, true);

    // set up the serial port
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    printf("BMS Tester starting up ...\n");

    battery.initialise();

    // 8MHz clock for CAN oscillator
    clock_gpio_init(CAN_CLK_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 10);

    printf("Setting up main CAN port (BITRATE:%d:%d)\n", CAN_500KBPS, MCP_8MHZ);
    mainCAN.reset();
    mainCAN.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mainCAN.setNormalMode();
    printf("Enabling handling of inbound CAN messages on main bus\n");
    enable_handle_main_CAN_messages();

    printf("Enabling handling of inbound CAN messages to the mock batteries\n");
    enable_handle_battery_CAN_messages();

    printf("Enable listening for inputs\n");
    enable_listen_for_input_signals();

    while (true) {
        test_case_001_ensure_car_cannot_be_driven_when_battery_is_empty(&battery);
        test_case_002_ensure_battery_cannot_be_charged_when_full(&battery);
        
        // test_case_101_inhibit_battery_contactor_close_when_pack_voltages_differ(&battery);
        // test_case_102_do_not_inhibit_battery_contactor_close_when_pack_voltage_differ_and_ignition_is_on(&battery);
        // test_case_103_ignition_turned_on_when_battery_contactors_are_inhibited(&battery);
        // test_case_104_ignition_turned_off_when_battery_contactors_are_inhibited(&battery);
        // test_case_105_start_charging_when_battery_contactors_are_inhibited(&battery);
        // test_case_106_stop_charging_when_battery_contactors_are_inhibited(&battery);
        // test_case_107_charging_on_one_pack_and_voltage_equalises(&battery);
        // test_case_108_driving_on_one_pack_and_voltage_equalises(&battery);
        // test_case_109_driving_on_one_pack_then_begin_charging_while_ignition_still_on(&battery);

        // test_case_201_battery_too_cold_to_charge(&battery);
        // test_case_202_battery_warm_enough_to_charge_again(&battery);
        // test_case_203_too_cold_to_charge_but_charge_requested(&battery);
        // test_case_204_battery_too_hot_to_charge(&battery);
    }

    return 0;
}

