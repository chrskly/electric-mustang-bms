/*
 * This file is part of the ev mustang charge controller project.
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


#include <time.h>
#include <stdio.h>
#include <pico/stdlib.h>

#include "mcp2515/mcp2515.h"


clock_t get_clock() {
    return (clock_t) time_us_64() / 10000;
}

void zero_frame(can_frame* frame) {
    frame->can_id = 0;
    frame->can_dlc = 8;
    for ( int i = 0; i < 8; i++ ) {
        frame->data[i] = 0;
    }
}

void print_frame(can_frame* frame) {
    printf(" [print_frame] ID: 0x%03X, DLC: %d, Data: ", frame->can_id, frame->can_dlc);
    for ( int i = 0; i < 8; i++ ) {
        printf("%d ", frame->data[i]);
    }
    printf("\n");
}