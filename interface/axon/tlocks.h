/*
 * Source File : tlocks.h
 * Begun : November 10, 1997
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
 
#ifndef _tlocks_h
#define _tlocks_h

int lock_term(char *name);
int check_term_lock(char *name);
int unlock_term(char *name);


void dump_termios_settings(struct termios *t);
void log_termios_settings(char *hdr, char *lname, struct termios *t);
void show_flags_by_mask(FlagNames *entries, int maxflags, tcflag_t flag);

#endif
