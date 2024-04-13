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

#include "include/battery.h"

Battery::Battery(int _numPacks) {
    voltage = 0;
    lowestCellVoltage = 0;
    highestCellVoltage = 0;
    lowestCellTemperature = 0;
    highestCellTemperature = 0;
    numPacks = _numPacks;
}

void Battery::initialise() {
    printf("Initialising battery with %d packs\n", numPacks);

    for ( int p = 0; p < numPacks; p++ ) {
        printf("Initialising battery pack %d (cs:%d, cp:%d, mpp:%d, cpm:%d, tpm:%d)\n", p, CS_PINS[p], INHIBIT_CONTACTOR_PINS[p], MODULES_PER_PACK, CELLS_PER_MODULE, TEMPS_PER_MODULE);
        packs[p] = BatteryPack(p, CS_PINS[p], INHIBIT_CONTACTOR_PINS[p], MODULES_PER_PACK, CELLS_PER_MODULE, TEMPS_PER_MODULE);
        printf("Initialisation of battery pack %d complete\n", p);
        packs[p].set_battery(this);
    }
}