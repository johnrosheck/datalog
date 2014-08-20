/*
 * Source File : tnames.c
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

#include "globals.h"
#include "termnames.h"
#include "tlocks.h"

#define USE_ASCII_PID 1


int lock_term(char *name) {
  char lbuf[MAXLINESIZE+1];
  pid_t pid;
  
  /* create a local file for the pid number */
  pid = getpid(); /* the current pid is used in the lock file */
  
}

/* see if the lock file is present */
int check_term_lock(char *name) {
  
}

int unlock_term(char *name) {
  
}
