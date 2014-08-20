/*
 * Source File : termnames.h
 * Begun : April 5, 1996
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
 
#ifndef _termnames_h
#define _termnames_h

/* this won't work without termios.h and linux/termios.h */

typedef struct {
  char *name;
  tcflag_t mask;
} FlagNames;

#define MAXINFLAGS 14
extern FlagNames in_flags[];

#define MAXOUTFLAGS 14
extern FlagNames out_flags[];

/* these flags are for output delays enabled */
#define ODLYFLAGS (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY)

#define MAXCNTLFLAGS 8
extern FlagNames cntl_flags[];

#define CSIZEMASK CSIZE
#define MAXCSIZENAMES 4
extern FlagNames csize_names[];

#define BAUDMASK CBAUD
#define MAXBAUDNAMES 18
extern FlagNames baud_names[];

#define MAXLOCALFLAGS 15
extern FlagNames local_flags[];

#define MAXCCNAMES 17
extern FlagNames cntl_names[];

#define MAXLINENAMES 9
extern FlagNames line_names[];

#endif
