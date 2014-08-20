/*
 * Source File : analog.c
 * Begun : 12 October 2013
 * Latest Revision : 13 July 2014
 * Version : 0.1
 *
 * Provides routines to access the analog data converter for all six 
 * channels and averages the readings.
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
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

#include "analog.h"

uint8_t cur_adc_chan;
uint16_t accum[6];
uint8_t accum_cnt;
uint16_t adc_data[6];

#define AVERAGE_CNT 64

void do_averaging_and_conversion(void) {
  uint8_t i;
  double d;
  for (i=0;i<6;i++) {
    d = accum[i];
    d /= AVERAGE_CNT;
    d *= 5000.0;
    d /= 1024.0;
    adc_data[i] = d;
    accum[i] = 0;
  }
  accum_cnt = 0;
}

uint8_t do_adc_loop(void) {
  if (conversion_done() == 0) return(0xff); /* not ready yet */
  accum[cur_adc_chan] += get_adc_data();
  cur_adc_chan++;
  if (cur_adc_chan >= 6) {
    accum_cnt++;
    cur_adc_chan = 0; /* reset */
    if (accum_cnt >= AVERAGE_CNT) { /* prep values */
      do_averaging_and_conversion();
      start_conversion(cur_adc_chan);
      return(0xff); /* signal new averaged values */
    }
  }
  start_conversion(cur_adc_chan);
  return(cur_adc_chan);
}

void initialize_adc(void) {
  uint8_t i;
  DIDR0 = 0x3f; /* disable all digital inputs on ADC inputs */
  ADCSRA = 0x87; /* enable ADC, clock = /128 */
  for (i=0;i<6;i++) {
    accum[i] = 0;
  }
  _delay_ms(1);
  cur_adc_chan = 0;
  accum_cnt = 0;
  start_conversion(cur_adc_chan);
}

void start_conversion(uint8_t chan) {
  ADCSRA |= 0x10; /* should clear ADIF flag */
  ADMUX = 0x40 | (chan & 0xf); /* select channel, ref = AVCC, right adjust result */
  ADCSRA |= 0x40; /* start conversion */
}

uint8_t conversion_done(void) {
  uint8_t i;
  i = ADCSRA;
  if ((i & 0x10) == 0x10) {
    return(1);
  }
  return(0);
}

uint16_t get_adc_data(void) {
  uint16_t i;
  i = ADCL;
  i |= (ADCH << 8);
  return(i);
}

