/*
 * Source File : mlines.c
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

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "mlines.h"

int get_mlines(int fd) {
  int i;
  ioctl(fd, TIOCMGET, &i);
  return(i);
}

int put_mlines(int fd, int lineset) {
  ioctl(fd, TIOCMSET, &lineset);
  ioctl(fd, TIOCMGET, &lineset);
  return(lineset);
}

int set_dtr(int fd) {
  int i;
  i = get_mlines(fd);
  i |= DTR;
  i = put_mlines(fd, i);
  return(i);
}

int clr_dtr(int fd) {
  int i;
  i = get_mlines(fd);
  i &= ~(DTR);
  i = put_mlines(fd, i);
  return(i);
}

int set_rts(int fd) {
  int i;
  i = get_mlines(fd);
  i |= RTS;
  i = put_mlines(fd, i);
  return(i);
}

int clr_rts(int fd) {
  int i;
  i = get_mlines(fd);
  i &= ~(RTS);
  i = put_mlines(fd, i);
  return(i);
}

int get_dcd(int fd) {
  if (get_mlines(fd) & DCD) return(1);
  return(0);
}

int get_dsr(int fd) {
  if (get_mlines(fd) & DSR) return(1);
  return(0);
}

int get_cts(int fd) {
  if (get_mlines(fd) & CTS) return(1);
  return(0);
}

int get_dtr(int fd) {
  if (get_mlines(fd) & DTR) return(1);
  return(0);
}

int get_rts(int fd) {
  if (get_mlines(fd) & RTS) return(1);
  return(0);
}
