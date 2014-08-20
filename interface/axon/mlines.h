/*
 * Source File : mlines.h
 * Begun : April 22, 1996
 * Latest Revision : July 22, 2014
 * Version : 1.00
 *
 * Copyright (C) 1996-2014 John B. Rosheck, Jr.
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
 
#ifndef _mlines_h
#define _mlines_h

int get_mlines(int fd); /* get modem lines through ioctl */
int put_mlines(int fd, int lineset); /* set modem lines through ioctl */
int set_dtr(int fd);
int clr_dtr(int fd);
int set_rts(int fd);
int clr_rts(int fd);
int get_dcd(int fd);
int get_dsr(int fd);
int get_cts(int fd);
int get_dtr(int fd);
int get_rts(int fd);

#define DTR TIOCM_DTR
#define RTS TIOCM_RTS
#define CTS TIOCM_CTS
#define DSR TIOCM_DSR
#define DCD TIOCM_CD
#define RI  TIOCM_RI

#endif
