/*
 * Source File : control_io.h
 * Begun : November 24, 1997
 * Latest Revision : July 22, 2014
 * Version : 1.00
 *
 * Copyright (C) 1997-2014 John B. Rosheck, Jr.
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
 
#ifndef _control_io_h
#define _control_io_h

#define read_ch(a, b) read(a, b, 1)

void setup_logging(char *lname);

int start_console_io(void);
int start_serial_io(int fd, char *given_baud);
void restore_console_io(void);
void restore_serial_io(int fd);

#endif
