#
# Source File : Makefile
# Begun : 22 April 2012
# Latest Revision : July 22, 2014
# Version : 1.00
#
# Copyright (C) 2012-2014 John B. Rosheck, Jr.
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

CC	= gcc

LIBS	=

all:	progs

progs:	cmd_resp

cmd_resp: cmd_resp.c
	$(CC) $@.c -o $@

clean	:	cleanbin
	rm -f *.o *~
	
cleanbin:
	rm -f cmd_resp

