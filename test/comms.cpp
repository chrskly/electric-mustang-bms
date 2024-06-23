// /*
//  * This file is part of the ev mustang bms project.
//  *
//  * Copyright (C) 2024 Christian Kelly <chrskly@chrskly.com>
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  * GNU General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
//  */

// #include <stdio.h>
// #include <stdexcept>
// #include "pico/stdlib.h"
// #include "pico/multicore.h"
// #include "include/comms.h"
// #include "pico/stdlib.h"
// #include "settings.h"
// #include "include/battery.h"
// #include "include/pack.h"
// #include "include/bms.h"
// //#include "include/isashunt.h"


// extern MCP2515 mainCAN;
// extern Battery battery;
// extern Bms bms;

// extern mutex_t canMutex;

// // Handle messages coming in on the main CAN bus

// //struct can_frame m;
// //struct repeating_timer handleMainCANMessageTimer;


// // bool handle_main_CAN_messages(struct repeating_timer *t) {
// //     can_frame m;
// //     // Try to get the mutex. If we can't, we'll try again next time.
// //     if ( !mutex_enter_timeout_ms(&canMutex, CAN_MUTEX_TIMEOUT_MS) ) {
// //         printf("Failed to get main CAN mutex\n");
// //         return false;
// //     }
// //     // Read a message
// //     MCP2515::ERROR result = mainCAN.readMessage(&m);
// //     mutex_exit(&canMutex);
// //     // If there are no messages, just quit
// //     if ( result != MCP2515::ERROR_NOMSG ) {
// //         return false;
// //     }
// //     printf("Message received on mainCAN : %d\n", m.can_id);
// //     // If we got a message, handle it
// //     switch ( m.can_id ) {
// //         // BMS state message
// //         case 0x352:
// //             bms.set_state(m.data[0]);
// //         case 0x521:
// //             break;
// //         default:
// //             break;
// //     }
// //     return true;
// // }

// // void enable_handle_main_CAN_messages() {
// //     add_repeating_timer_ms(10, handle_main_CAN_messages, NULL, &handleMainCANMessageTimer);
// // }

// // Handle the CAN messages that come in from the BMS to the mock batteries

// struct repeating_timer handleBatteryCANMessagesTimer;

// bool handle_battery_CAN_messages(struct repeating_timer *t) {
//     battery.read_message();
//     return true;
// }

// void enable_handle_battery_CAN_messages() {
//     add_repeating_timer_ms(5, handle_battery_CAN_messages, NULL, &handleBatteryCANMessagesTimer);
// }



