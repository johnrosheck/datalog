#!/usr/bin/perl -w
# Source File : make_empty_rrd
# Begun : June 26, 2006
# Last Updated : June 30, 2014
#
# Copyright (C) 2006-2014 John B. Rosheck, Jr.
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
use strict;
use Time::Local;

if (!defined($ARGV[0])) {
  print "ERROR - must supply a prefix\n";
  exit(1);
}
my $prefix;
$prefix = $ARGV[0];

my $rrd_filepath= "/home/web/data/rrd/" . $prefix . ".rrd";

my $sample_cnt=0;
my $stepsize = 1; # how many seconds between samples
my $full_storage_span = 90 + 1; # number of days in entire db

my $secs_in_hour = (60 * 60);
my $secs_in_day = $secs_in_hour * 24.0;
my $rrd_length_in_secs = $secs_in_day * $full_storage_span;

my @tvar;
$tvar[0] = 0;
$tvar[1] = 0;
$tvar[2] = 0;
$tvar[3] = 1;
$tvar[4] = 0;
$tvar[5] = 106;
$tvar[6] = 0;
$tvar[7] = 0;
$tvar[8] = 0;

my $rtime = timelocal(@tvar);

print "local time at 1/1/2006 0:0:0 = $rtime\n";
#exit(0);

my $starttime = time() - $rrd_length_in_secs;
my $endtime = $starttime + $rrd_length_in_secs;
my $failstep = $stepsize * 2;
my $span = $rrd_length_in_secs / $stepsize; # number of data points in archive

my $cmd;
my $resp;

$cmd = "rrdtool create";
$cmd .= " $rrd_filepath";
$cmd .= " --start $starttime";
#$cmd .= " --end $endtime";
$cmd .= " --step $stepsize";
$cmd .= " DS:ch1:GAUGE:$failstep:-40:199"; # temp #1
$cmd .= " DS:ch2:GAUGE:$failstep:-40:199"; # temp #2
$cmd .= " DS:ch3:GAUGE:$failstep:-40:199"; # temp #3
$cmd .= " DS:ch4:GAUGE:$failstep:-40:199"; # temp #4
$cmd .= " DS:ch5:GAUGE:$failstep:-40:199"; # temp #5
$cmd .= " DS:ch6:GAUGE:$failstep:-40:199"; # temp #6
#$cmd .= " DS:ch7:GAUGE:$failstep:-40:199"; # outside temp
#$cmd .= " DS:ch8:GAUGE:$failstep:0:100"; # relative insolation
$cmd .= " RRA:AVERAGE:0.5:1:$span";
$cmd .= "\n";
print $cmd;
$resp = `$cmd`;
print $resp;
exit(0);

