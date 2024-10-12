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

#ifndef BMS_TEST_INCLUDE_TESTCASES0XX_H_
#define BMS_TEST_INCLUDE_TESTCASES0XX_H_

#include "include/battery.h"

bool test_case_001(Battery* battery, Bms* bms);
bool test_case_002(Battery* battery, Bms* bms);
bool test_case_003(Battery* battery, Bms* bms);
bool test_case_004(Battery* battery, Bms* bms);
bool test_case_005(Battery* battery, Bms* bms);
bool test_case_006(Battery* battery, Bms* bms);

#endif // BMS_TEST_INCLUDE_TESTCASES0XX_H_