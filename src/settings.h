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
#define UART_TX_PIN      0                          // pin 1
#define UART_RX_PIN      1                          // pin 2

// CAN bus
#define SPI_PORT      spi0
#define SPI_MISO        16                          // pin 21
#define SPI_CLK         18                          // pin 24
#define SPI_MOSI        19                          // pin 25
#define CAN_CLK_PIN     21                          // pin 27
#define MAIN_CAN_CS     17                          // pin 22
const int CS_PINS[2] = { 20, 15 };                  // Chip select pins for the CAN controllers for each battery pack.

// Inputs
#define IGNITION_ENABLE_PIN        10               // Ignition on input signal
#define CHARGE_ENABLE_PIN           9               // Charge enabled input signal
#define POS_CONTACTOR_FEEDBACK_PIN 11               // Feedback from the HVJB positive contactor for welding detection
#define NEG_CONTACTOR_FEEDBACK_PIN 12               // Feedback from the HVJB negative contactor for welding detection
const int CONTACTOR_FEEDBACK_PINS[2] = { 13, 14 };  // Feedback from the battery box contactors for welding detection

// Outputs
#define CHARGE_INHIBIT_PIN 4                        // Low-side switch to create CHARGE_INHIBIT signal. a.k.a OUT1
#define HEATER_ENABLE_PIN 5                         // Low-side switch to turn on battery heaters. a.k.a. OUT2
const int INHIBIT_CONTACTOR_PINS[2] = { 2, 3 };     // Low-side switch to disallow closing of battery box contactors
#define DRIVE_INHIBIT_PIN 6                         // Low-side switch to disallow driving. a.k.a OUT3
#define OUT_4_PIN 7                                 // unused

// Pack/module configuration
#define NUM_PACKS         2                         // The total number of paralleled packs in this battery
#define CELLS_PER_MODULE 16                         // The number of cells in each module
#define TEMPS_PER_MODULE  4                         // The number of temperature sensors in each module
#define MODULES_PER_PACK  6                         // The number of modules in each pack

// Timeouts
#define MODULE_TTL 5                                // If we have not seen an update from a module in MODULE_TTL
                                                    // seconds, them mark the module as dead.

#define SHUNT_TTL 3                                 // If we have not seen an update from the ISA shunt in SHUNT_TTL
                                                    // seconds, then mark it as dead.

#define PACKS_IMBALANCED_TTL 3000                   // If the packs are imbalanced for more than PACKS_IMBALANCED_TTL
                                                    // seconds, then actually inhibit the contactors.

#define SAFE_VOLTAGE_DELTA_BETWEEN_PACKS 10         // When closing contactors, the voltage difference between the packs
                                                    // shall not be greater than this voltage, in millivolts.

#define CELL_DELTA_WARN_THRESHOLD 20                // If the cell delta is greater than this value, then raise a warning.
#define CELL_DELTA_ALARM_THRESHOLD 200              // If the cell delta is greater than this value, then raise an alarm.

// Temperature
#define PACK_TEMP_SAMPLE_INTERVAL 60                // How often to sample the pack temperature in seconds
#define WARNING_TEMPERATURE 30                      // 
#define MAXIMUM_TEMPERATURE 50                      // Stop everything if the battery is above this temperature
#define CHARGE_TEMPERATURE_MINIMUM -10              // minimum temperature required to allow charging
#define CHARGE_TEMPERATURE_DERATING_MINIMUM 15      // where temperature based derating kicks in
#define CHARGE_TEMPERATURE_DERATING_THRESHOLD 1     // Allow temperature to increase this much per minute. Above that, derate.

// Battery capacity/voltages/etc.
#define BATTERY_CAPACITY_WH 14800                   // 7.4kWh usable per pack, x2 packs == 14.8kWh
#define BATTERY_CAPACITY_AS 187200                  // 26Ah per pack (93,600 As), x2 packs == 187,200 As
#define CALCULATE_SOC_FROM_AMP_SECONDS 1            // Should we calculate SoC from amp seconds (value = 1) or
                                                    // kWh (value = 0)? 
#define CELL_EMPTY_VOLTAGE 2900                     // Official min pack voltage = 269V. 269 / 6 / 16 = 2.8020833333V
#define CELL_FULL_VOLTAGE 4000                      // Official max pack voltage = 398V. 398 / 6 / 16 = 4.1458333333V

// Cell balancing
#define CELL_BALANCE_VOLTAGE 3900                   // Cell balancing should only happen above this voltage
#define CELL_BALANCE_INTERVAL 60000                 // Interval between cell balancing sessions in milliseconds

// Communication
#define CAN_MUTEX_TIMEOUT_MS 200                    // Timeout for the CAN mutex
#define SEND_FRAME_RETRIES 6                        // Number of times to retry sending a frame before giving up
#define READ_FRAME_RETRIES 3                        // Number of times to retry reading a frame before giving up

#endif  // BMS_SRC_SETTINGS_H_
