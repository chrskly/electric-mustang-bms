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

/*------------------------------------------------------------------------------

EV Mustang BMS

------------------------------------------------------------------------------*/


#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
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
#include "include/io.h"
#include "include/shunt.h"


mutex_t canMutex;
Io io;
Shunt shunt;
Battery battery;
Bms bms;
//MCP2515 mainCAN;

// Watchdog

struct repeating_timer watchdogKeepaliveTimer;

bool watchdog_keepalive(struct repeating_timer *t) {
    watchdog_update();
    return true;
}

void enable_watchdog_keepalive() {
    add_repeating_timer_ms(1000, watchdog_keepalive, NULL, &watchdogKeepaliveTimer);
}

// Status print

struct repeating_timer statusPrintTimer;

bool status_print(struct repeating_timer *t) {
    extern Bms bms;
    bms.print();
    //battery.print();
    return true;
}

//
void enable_status_print() {
    printf(" * Enabling status print\n");
    add_repeating_timer_ms(1000, status_print, NULL, &statusPrintTimer);
}


int main() {
    stdio_init_all();

    // 80Mhz CPU speed
    set_sys_clock_khz(80000, true);

    // set up the serial port
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    printf("BMS starting up ...\n");

    // Check for unexpected reboot
    if (watchdog_caused_reboot()) {
        printf(" * !!!! Rebooted by Watchdog !!!!\n");
        bms.set_watchdog_reboot(true);
    } else {
        printf(" * Clean boot\n");
        bms.set_watchdog_reboot(false);
    }
    watchdog_enable(5000, 1);
    enable_watchdog_keepalive();

    // 8MHz clock for CAN oscillator
    clock_gpio_init(CAN_CLK_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 10);

    mutex_init(&canMutex);

    // Initialise all of the objects
    io = Io();
    shunt = Shunt();
    //battery = Battery(&canMutex, &io);
    battery = Battery(&io);
    bms = Bms(&battery, &io, &shunt);
    battery.initialise(&bms);

    enable_status_print();

    printf("---- BMS READY ----\n");

    while (true) {
    }

    return 0;
}


