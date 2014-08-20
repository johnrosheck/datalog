#
# Source File : Makefile
# Begun : April 5, 1996
# Latest Revision : July 22, 2014
# Version : 3.00
#
# Copyright (C) 1996-2014 John B. Rosheck, Jr.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#

CC	 = gcc
OPTIMIZE = -fomit-frame-pointer -Wall -O2 -s
CFLAGS	 = $(DEFINES) $(OPTIMIZE)
LFLAGS   = -N
LDFLAGS  =

all:	progs

progs : axon ${PROGS_O}

PROGS_O = axon.o interactive.o control_io.o tnames.o mlines.o

objs:	$(PROGS_O)

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<

.o:
	$(CC) $(CFLAGS) $(LFLAGS) -o $* $*.o $(LIBS)
	chmod a+rs,go-w $*

#
axon: $(PROGS_O)
	$(CC) $(LDFLAGS) -o $@ $(PROGS_O) -lm
	chmod a+rs,go-w axon
#
#
clean		:	cleanbin
	rm -f *.o *~ axon
#
cleanbin	:
	rm -f $(PROGS_O)
#
install		:
	cp axon ../bin/axon

