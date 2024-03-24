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

/*------------------------------------------------------------------------------

EV Mustang BMS

------------------------------------------------------------------------------*/


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/watchdog.h"

#include "mcp2515/mcp2515.h"

#include "include/battery.h"
#include "include/bms.h"
#include "include/statemachine.h"
#include "include/comms.h"
#include "include/led.h"
#include "include/inputs.h"
#include "include/isashunt.h"


struct can_frame rx;

// receive frame
struct can_frame frame;

// error frame
struct can_frame errorFrame;

State state;
StatusLight statusLight;

Battery battery(NUM_PACKS);
MCP2515 mainCAN(SPI_PORT, MAIN_CAN_CS, SPI_MISO, SPI_MOSI, SPI_CLK, 500000);

ISAShunt shunt;


// Watchdog

struct repeating_timer watchdogKeepaliveTimer;

bool watchdog_keepalive(struct repeating_timer *t) {
    watchdog_update();
    return true;
}

void enable_watchdog_keepalive() {
    add_repeating_timer_ms(5000, watchdog_keepalive, NULL, &watchdogKeepaliveTimer);
}


// Status print

struct repeating_timer statusPrintTimer;

bool status_print(struct repeating_timer *t) {
    battery.print();
    return true;
}

//
void enable_status_print() {
    add_repeating_timer_ms(5000, status_print, NULL, &statusPrintTimer);
}


/*
 * Update SoC.
 *
 * It's excessive to update this value every time we get a message from the ISA
 * shunt. Just update at a regular interval.
 */

struct repeating_timer socUpdateTimer;

bool update_soc(struct repeating_timer *t) {
    battery.recalculate_soc();
    return true;
}

//
void enable_update_soc() {
    add_repeating_timer_ms(5000, update_soc, NULL, &socUpdateTimer);
}



int main() {
    stdio_init_all();

    set_sys_clock_khz(80000, true);

    // set up the serial port
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    printf("BMS starting up ...\n");

    state = &state_standby;

    battery.initialise();

    // Set up blinky LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    statusLight.led_set_mode(STANDBY);
    enable_led_blink();

    // 8MHz clock for CAN oscillator
    clock_gpio_init(CAN_CLK_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 10);

    printf("Setting up main CAN port (BITRATE:%d:%d)\n", CAN_500KBPS, MCP_8MHZ);
    mainCAN.reset();
    mainCAN.setBitrate(CAN_500KBPS, MCP_8MHZ);
    mainCAN.setNormalMode();
    printf("Enabling handling of inbound CAN messages on main bus\n");
    enable_handle_main_CAN_messages();

    // Check for unexpected reboot
    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");
        // errorFame.can_id = ;
        // mainCAN.sendMessage();
        return 0;
    } else {
        printf("Clean boot\n");
    }
    watchdog_enable(100, 1);
    enable_watchdog_keepalive();

    battery.print();
    battery.send_test_message();

    printf("Enabling handling of inbound CAN messages from batteries\n");
    enable_handle_battery_CAN_messages();

    printf("Enabling module polling\n");
    enable_module_polling();

    printf("Enabling status print\n");
    enable_status_print();

    printf("Enable listen for IGNITION_ON signal\n");
    enable_listen_for_ignition_signal();

    printf("Enable listen for CHARGE_ENABLE signal\n");
    enable_listen_for_charge_enable_signal();

    enable_update_soc();

    while (true) {
    }

    return 0;
}


