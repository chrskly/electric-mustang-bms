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

#include "include/testcases0xx.h"

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

    while (true) {
        if ( ! test_case_001_ensure_car_cannot_be_driven_when_battery_is_empty(&battery) ) {
            printf("Test case 001 failed\n");
        }
    }

    return 0;
}

