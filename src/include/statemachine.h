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

#ifndef BMS_SRC_INCLUDE_STATEMACHINE_H_
#define BMS_SRC_INCLUDE_STATEMACHINE_H_

enum Event {
    E_TEMPERATURE_UPDATE,     // a battery temperature update has been received
    E_CELL_VOLTAGE_UPDATE,    // a cell voltage update has been received
    E_IGNITION_ON,            // Ignition was turned on
    E_IGNITION_OFF,           // Ignition was turned off
    E_CHARGING_INITIATED,     // charging has been initiated
    E_CHARGING_TERMINATED,    // charging has stopped
    E_EMERGENCY_SHUTDOWN,     //
};

typedef void (*State)(Event);

void state_standby(Event event);
void state_drive(Event event);
void state_charging(Event event);
void state_batteryEmpty(Event event);
void state_overTempFault(Event event);
void state_illegalStateTransitionFault(Event event);

#endif  // BMS_SRC_INCLUDE_STATEMACHINE_H_
