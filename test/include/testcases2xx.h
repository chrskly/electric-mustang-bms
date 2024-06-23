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

#ifndef BMS_TEST_INCLUDE_TESTCASES2XX_H_
#define BMS_TEST_INCLUDE_TESTCASES2XX_H_

#include "include/battery.h"

bool test_case_201_battery_too_cold_to_charge(Battery* battery, Bms* bms);
bool test_case_202_battery_warm_enough_to_charge_again(Battery* battery, Bms* bms);
bool test_case_203_too_cold_to_charge_but_charge_requested(Battery* battery, Bms* bms);
bool test_case_204_battery_too_hot_to_charge(Battery* battery, Bms* bms);

#endif // BMS_TEST_INCLUDE_TESTCASES2XX_H_