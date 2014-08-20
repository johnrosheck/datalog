/*
 * Source File : analog.h
 * Begun : 12 October 2013
 * Latest Revision : 13 July 2014
 * Version : 0.1
 *
 * Copyright (C) 2013-2014 John B. Rosheck, Jr.
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

#ifndef _analog_h
#define _analog_h

extern uint16_t adc_data[6];

void initialize_adc(void);
void start_conversion(uint8_t chan);
uint8_t conversion_done(void);
uint16_t get_adc_data(void);
uint8_t do_adc_loop(void);

#endif
