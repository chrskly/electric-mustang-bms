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
#include <string>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pll.h"
#include "hardware/watchdog.h"

#include "mcp2515/mcp2515.h"
#include "include/bmstest.h"
#include "include/battery.h"
#include "include/io.h"
#include "include/bms.h"
#include "include/shunt.h"

#include "include/testcaseutils.h"
#include "include/testcases0xx.h"
#include "include/testcases1xx.h"
#include "include/testcases2xx.h"

#include "settings.h"

mutex_t canMutex;
Battery battery;
Shunt shunt;
Bms bms;

struct repeating_timer statusPrintTimer;

bool status_print(struct repeating_timer *t) {
    //extern Bms bms;
    // std::string DRV_INH = bms.get_inhibitDrive() ? "on" : "off";
    // std::string CHG_INH = bms.get_inhibitCharge() ? "on" : "off";
    // printf("    * DRV_INH:%s, CHG_INH:%s \n", DRV_INH.c_str(), CHG_INH.c_str());
    // std::string BATT1_INH = battery.get_pack(0)->get_inhibit() ? "on" : "off";
    // std::string BATT2_INH = battery.get_pack(1)->get_inhibit() ? "on" : "off";
    // printf("    * Batt1 Inh:%s, Batt2 Inh:%s \n", BATT1_INH.c_str(), BATT2_INH.c_str());
    //battery.print();
    return true;
}

int main() {
    stdio_init_all();

    set_sys_clock_khz(80000, true);

    // set up the serial port
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    printf("BMS Tester starting up ...\n");

    // 8MHz clock for CAN oscillator
    clock_gpio_init(CAN_CLK_PIN, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 10);

    mutex_init(&canMutex);

    printf("Battery has %d pcks\n", NUM_PACKS);

    shunt = Shunt();
    bms = Bms(&shunt);
    battery = Battery(NUM_PACKS);
    bms.set_battery(&battery);

    printf("Enable listening for inputs\n");
    enable_listen_for_input_signals();

    gpio_init(IGNITION_ENABLE_PIN);
    gpio_set_dir(IGNITION_ENABLE_PIN, GPIO_OUT);

    gpio_init(CHARGE_ENABLE_PIN);
    gpio_set_dir(CHARGE_ENABLE_PIN, GPIO_OUT);

    add_repeating_timer_ms(1000, status_print, NULL, &statusPrintTimer);

    while (true) {

        // while(true) {

        //     uint16_t voltage;

        //     voltage = battery.get_voltage_from_soc(90);
        //     printf("90%% SOC : %d\n", voltage);
        //     battery.set_all_cell_voltages(voltage);
        //     sleep_ms(20000);
            
        //     voltage = battery.get_voltage_from_soc(70);
        //     printf("70%% SOC : %d\n", voltage);
        //     battery.set_all_cell_voltages(voltage);
        //     sleep_ms(20000);

        //     voltage = battery.get_voltage_from_soc(50);
        //     printf("50%% SOC : %d\n", voltage);
        //     battery.set_all_cell_voltages(voltage);
        //     sleep_ms(20000);

        //     voltage = battery.get_voltage_from_soc(30);
        //     printf("30%% SOC : %d\n", voltage);
        //     battery.set_all_cell_voltages(voltage);
        //     sleep_ms(20000);

        //     voltage = battery.get_voltage_from_soc(10);
        //     printf("10%% SOC : %d\n", voltage);
        //     battery.set_all_cell_voltages(voltage);
        //     sleep_ms(20000);

        // }

        printf("========================================\n");
        printf("WARMING UP\n");
        printf("========================================\n");

        battery.set_all_cell_voltages(battery.get_voltage_from_soc(50));
        set_ignition_state(false);
        set_charge_enable_state(false);
        printf("Waiting for BMS to enter standby state\n");
        wait_for_bms_state(&bms, STATE_STANDBY, 30000);
        sleep_ms(20000);

        printf("========================================\n");
        printf("STARTING TESTS\n");
        printf("========================================\n");

        test_case_001(&battery, &bms);
        test_case_002(&battery, &bms);
        test_case_003(&battery, &bms);
        test_case_004(&battery, &bms);
        test_case_005(&battery, &bms);
        test_case_006(&battery, &bms);
        
        test_case_101(&battery, &bms);
        test_case_102(&battery, &bms);
        test_case_103(&battery, &bms);
        test_case_104(&battery, &bms);
        test_case_105(&battery, &bms);
        test_case_106(&battery, &bms);
        test_case_107(&battery, &bms);
        test_case_108(&battery, &bms);
        test_case_109(&battery, &bms);
        test_case_110(&battery, &bms);
        test_case_111(&battery, &bms);

        test_case_201(&battery, &bms);
        test_case_202(&battery, &bms);
        test_case_203(&battery, &bms);
        test_case_204(&battery, &bms);
        test_case_205(&battery, &bms);

    }

    return 0;
}