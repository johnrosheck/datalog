/*
 * Source File : main.c
 * Begun : 22 April 2012
 * Latest Revision : 2 June 2012
 * Version : 0.1
 *
 * This program continually reads the analog input ports and writes
 * that data out the serial port.
 *
 * Copyright (C) 2012-2014 John B. Rosheck, Jr.
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

uint16_t counter;

#define BAUD 0x08

void uart_init(void) {
  UBRR0H = 0;
  UBRR0L = BAUD;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

void uart_send(unsigned char data) {
  while (!(UCSR0A & (1 << UDRE0))) {
    /* wait */ ;
  }
  UDR0 = data;
}

unsigned char uart_get(void) {
  while (!(UCSR0A & (1 << RXC0))); /* wait for char */
  return(UDR0);
}

void do_print(char *p) {
  while (*p != 0) {
    uart_send(*p++);
  }
}

void show_byte(uint8_t c) {
  uint8_t i;
  unsigned char t;
  i = (c >> 4) & 0xf;
  if (i < 10) {
    t = '0' + i;
  } else {
    t = (i - 10) + 'A';
  }
  uart_send(t);
  i = c & 0xf;
  if (i < 10) {
    t = '0' + i;
  } else {
    t = (i - 10) + 'A';
  }
  uart_send(t);
}

void show_word(uint16_t i) {
  uint8_t c;
  c = (i >> 8) & 0xff;
  show_byte(c);
  c = i & 0xff;
  show_byte(c);
}

void show_number(uint16_t i) {
  uint16_t j;
  if (i >= 10000) {
    j = i / 10000;
    i -= (j * 10000);
    uart_send('0' + j);
  } else {
    /* uart_send('0'); - don't lead with zero here */ 
  }
  if (i >= 1000) {
    j = i / 1000;
    i -= (j * 1000);
    uart_send('0' + j);
  } else {
    uart_send('0');
  }
  if (i >= 100) {
    j = i / 100;
    i -= (j * 100);
    uart_send('0' + j);
  } else {
    uart_send('0');
  }
  if (i >= 10) {
    j = i / 10;
    i -= (j * 10);
    uart_send('0' + j);
  } else {
    uart_send('0');
  }
  uart_send('0' + i);
}


void do_main_loop(void) {
  uint8_t v;
  uint8_t i;
  while ((v = do_adc_loop()) != 0xaa) {
    /* _delay_ms(1); */
  }
  show_word(counter);
  uart_send(' ');
  for (i=0;i<6;i++) {
    show_number(adc_data[i]);
    uart_send(' ');
  }
  uart_send('\n');
  counter++;
}

void main(void) {
  int data;
  counter = 0;
  /* PORTB -> output */
  DDRB = 0xff; /* all outputs */
  DDRC = 0x00; /* all inputs */
  PORTB = 0x00; /* set to all zeros */
  DDRC &= 0xe0; /* force PC0 to PC5 to inputs */
  PORTD |= 0x80;

  uint8_t i, j, t, csec;
  uint8_t mode = 0; /* initialization / check out */

  uart_init();

  /* _delay_ms(1000); */
  /* do_print("Hello, world.\n"); */

  initialize_adc();
  /* _delay_ms(1000); */
  i = 0;
  j = 0;
  csec = 5; /* about 10 loops per second */
  while (1) {
    do_main_loop();
    t++;
    if (mode == 0) { /* initialization, check out */
      if (t >= csec) { /* test for more */
        t = 0;
      } /* else wait till more time has passed */
    } else if (mode == 1) { /* run mode */
    } else { /* error mode, only clear with power cycle */
    }
  }
}




