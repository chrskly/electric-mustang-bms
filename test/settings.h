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

#ifndef BMS_SRC_SETTINGS_H_
#define BMS_SRC_SETTINGS_H_

#define VERSION 1.0

// Serial port
#define UART_ID      uart0
#define BAUD_RATE   115200
#define UART_TX_PIN      0  // pin 1
#define UART_RX_PIN      1  // pin 2

// CAN bus
#define SPI_PORT      spi0
#define SPI_MISO        16          // pin 21
#define SPI_CLK         18          // pin 24
#define SPI_MOSI        19          // pin 25
#define CAN_CLK_PIN     21          // pin 27
#define MAIN_CAN_CS     17          // pin 22
const int CS_PINS[2] = { 20, 15 };  // Chip select pins for the CAN controllers for each battery pack.

// Inputs
#define DRIVE_INHIBIT_PIN  13                      //
const bool DRIVE_INHIBIT_ACTIVE_LOW = true;       // Is the drive inhibit signal active low?
#define CHARGE_INHIBIT_PIN 11                      //
const bool CHARGE_INHIBIT_ACTIVE_LOW = true;      // Is the charge inhibit signal active low?
#define HEATER_ENABLE_PIN   9                      //
const bool HEATER_ENABLE_ACTIVE_LOW = true;       // Is the heater signal active low?
const int INHIBIT_CONTACTOR_PINS[2] = { 14, 12 };  //
const bool INHIBIT_CONTACTOR_ACTIVE_LOW = true;   // Is the inhibit contactor signal active low?

// Outputs
#define IGNITION_ENABLE_PIN  2  //
const bool IGNITION_ENABLE_ACTIVE_LOW = true;  // Is the ignition signal active low?
#define CHARGE_ENABLE_PIN    3  //
const bool CHARGE_ENABLE_ACTIVE_LOW = true;    // Is the charge signal active low?
#define IN_1_PIN            11  // unused
#define IN_2_PIN            12  // unused
#define IN_3_PIN            13  // unused
#define IN_4_PIN            14  // unused


#define NUM_PACKS         2                        // The total number of paralleled packs in this battery
#define CELLS_PER_MODULE 16                        // The number of cells in each module
#define TEMPS_PER_MODULE  4                        // The number of temperature sensors in each module
#define MODULES_PER_PACK  6                        // The number of modules in each pack

#define PACK_ALIVE_TIMEOUT 5                       // If we have not seen an update from the BMS in
                                                   // PACK_ALIVE_TIMEOUT seconds, then mark the pack
                                                   // as dead.

#define MODULE_TTL 5                               // If we have not seen an update from a module in
                                                   // MODULE_TTL seconds, them mark the module as
                                                   // dead.

#define SHUNT_TTL 3                                // If we have not seen an update from the ISA
                                                   // shunt in SHUNT_TTL seconds, then mark it as
                                                   // dead.

#define SAFE_VOLTAGE_DELTA_BETWEEN_PACKS 0.01      // When closing contactors, the voltage difference between the packs shall not
                                                   // be greater than this voltage, in volts.


// The capacity of the battery pack
#define BATTERY_CAPACITY_WH 14800         // 7.4kWh usable per pack, x2 packs
#define BATTERY_CAPACITY_AS 187200        // 26Ah per pack (93,600 As), x2 packs
#define CALCULATE_SOC_FROM_AMP_SECONDS 1  // Should we calculate SoC from amp seconds (value = 1) or
                                          // kWh (value = 0)? 

// Official min pack voltage = 269V. 269 / 6 / 16 = 2.8020833333V
#define CELL_EMPTY_VOLTAGE 2850

// Official max pack voltage = 398V. 398 / 6 / 16 = 4.1458333333V
#define CELL_FULL_VOLTAGE 4100

#define MINIMUM_TEMPERATURE -20          // 
#define MINIMUM_CHARGING_TEMPERATURE 0   // Disallow charging below this temperature
#define WARNING_TEMPERATURE 30           // 
#define MAXIMUM_TEMPERATURE 50           // Stop everything if the battery is above this temperature

#define CHARGE_THROTTLE_TEMP_LOW  20                // Start throttling charge current when battery sensors above this temperature
#define CHARGE_THROTTLE_TEMP_HIGH 30                // Top of the throttling scale. Limit current to CHARGE_CURRENT_MIN at and above this temperature
#define CHARGE_CURRENT_MAX 125                      // ~50kw
#define CHARGE_CURRENT_MIN 8                        // ~3.3kw

#define BALANCE_INTERVAL 1200                       // number of seconds between balancing sessions

//// ---- CAN message IDs

#define BMS_LIMITS_MSG_ID 0x351                     // Charge/discharge limits message
#define BMS_SOC_MSG_ID 0x355                        // SoC status message
#define BMS_STATUS_MSG_ID 0x356                     // Status message emitted by the BMS
#define BMS_ALARM_MSG_ID 0x35A                      // Warning message emitted by the BMS

#define CAN_ID_ISA_SHUNT_AH 0x527                   // Message ISA shunt sends which contains Ah data.
#define CAN_ID_ISA_SHUNT_WH 0x528                   // Message ISA shunt sends which contains Wh data.

#endif  // BMS_SRC_SETTINGS_H_
