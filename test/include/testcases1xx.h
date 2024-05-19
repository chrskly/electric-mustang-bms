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

#ifndef BMS_TEST_INCLUDE_TESTCASES1XX_H_
#define BMS_TEST_INCLUDE_TESTCASES1XX_H_

#include "include/battery.h"

bool test_case_101_inhibit_battery_contactor_close_when_pack_voltages_differ(Battery* battery);
bool test_case_102_do_not_inhibit_battery_contactor_close_when_pack_voltage_differ_and_ignition_is_on(Battery* battery);
bool test_case_103_ignition_turned_on_when_battery_contactors_are_inhibited(Battery* battery);
bool test_case_104_ignition_turned_off_when_battery_contactors_are_inhibited(Battery* battery);
bool test_case_105_start_charging_when_battery_contactors_are_inhibited(Battery* battery);
bool test_case_106_stop_charging_when_battery_contactors_are_inhibited(Battery* battery);
bool test_case_107_charging_on_one_pack_and_voltage_equalises(Battery* battery);
bool test_case_108_driving_on_one_pack_and_voltage_equalises(Battery* battery);
bool test_case_109_driving_on_one_pack_then_begin_charging_while_ignition_still_on(Battery* battery);

#endif // BMS_TEST_INCLUDE_TESTCASES1XX_H_