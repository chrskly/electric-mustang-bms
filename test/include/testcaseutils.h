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

#ifndef BMS_TEST_INCLUDE_TESTCASEUTILS_H_
#define BMS_TEST_INCLUDE_TESTCASEUTILS_H_

#include "include/bms.h"
#include "include/battery.h"

bool wait_for_soc(Bms* bms, int soc, int timeout);
bool wait_for_drive_inhibit_state(Bms* bms, bool state, int timeout);
bool wait_for_charge_inhibit_state(Bms* bms, bool state, int timeout);
bool wait_for_bms_state(Bms* bms, BmsState state, int timeout);
bool wait_for_batt_inhibit_state(Battery* battery, int packId, bool state, int timeout);
bool wait_for_packs_imbalanced_state(Bms* bms, bool state, int timeout);
bool wait_for_heater_enable_state(Bms* bms, bool state, int timeout);

#endif // BMS_TEST_INCLUDE_TESTCASEUTILS_H_ 