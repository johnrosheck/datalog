#!/usr/bin/perl -w
# Source File : dl_daemon.pl
# Begun : May 25, 2009
# Last Updated : June 28, 2014
#
# Runs periodicly and updates the rrd file from source data then checks
# and if needed runs weekly graph updates on that same interval.  Other
# checks are run to update weekly wrapper html files.  A week is defined
# as to start on Sunday.
#
# Copyright (C) 2009-2014 John B. Rosheck, Jr.
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

my $norun = 0;
my $verbose = 1;

my $base = "/home/web";
my $webbase = $base . "/html/datalog";
my $execbase = $base . "/datalog";

my $prefix;

$prefix = find_prefix();

my $delay = 60;
my $history_checked = 0; # forces backwards scan to update all graphs, etc.

# global time variables (current time)
my $realtime; # int since epoch
my $datestring; # date in form YYYYMMDD
my $timestring; # time in form hhmmss
my $datetimestring; # date and time in form YYYYMMDD-hhmmss
my $dayofweek; # 0 is Sunday

# used to mark week wrapover, format YYYYMMDD and will be the previous
# or current Sunday
my $current_week;

my $notdone = 1;

sub stop_running {
  $notdone = 0;
}

$SIG{"INT"} = \&stop_running;

my $t;
my $cmd;
my $resp;
my $wkday0;

while ($notdone == 1) {
  # this will update the rrd file to the current time
  $cmd = $execbase . "/add_to_rrd.pl " . $prefix;
  print "##CMD=" . $cmd . "\n";
  if ($norun == 0) {
    $resp = `$cmd\n`;
    if ($verbose > 0) {
      print $resp;
    }
  }
  # update this process's time
  load_current_datetime();
  if ($verbose > 0) {
    print "TD2: $datetimestring\n";
  }
  # update this week's graphs
  $wkday0 = find_this_week();
  if (defined($current_week)) {
    if ($wkday0 != $current_week) { # new week
      if ($verbose > 0) {
        print "TD2: new week starting on Sunday $wkday0\n";
      }
      # check out new week
      check_week($wkday0);
      $current_week = $wkday0;
    } # else nothing to do here
  } else { # not defined, load it and check
    if ($verbose > 0) {
      print "TD2: startup forcing new week starting on Sunday $wkday0\n";
    }
    $current_week = $wkday0;
    check_week($wkday0);
  }
  
  $cmd = "$execbase/mkgraph $datestring $execbase/forms/" . $prefix . "-day-320.form\n";
  print "##CMD=$cmd";
  if ($norun == 0) {
    $resp = `$cmd`;
    if ($verbose > 1) {
      print $resp;
    }
  }

  $cmd = "$execbase/mkgraph $datestring $execbase/forms/" . $prefix . "-day-1024.form\n";
  print "##CMD=$cmd";
  if ($norun == 0) {
    $resp = `$cmd`;
    if ($verbose > 1) {
      print $resp;
    }
  }

#  $cmd = "$execbase/mkgraph-320-week $wkday0\n";
  $cmd = "$execbase/mkgraph $wkday0 $execbase/forms/" . $prefix . "-week-320.form\n";
  print "##CMD=$cmd";
  if ($norun == 0) {
    $resp = `$cmd`;
    if ($verbose > 1) {
      print $resp;
    }
  }

#  $cmd = "$execbase/mkgraph-1024-week $wkday0\n";
  $cmd = "$execbase/mkgraph $wkday0 $execbase/forms/" . $prefix . "-week-1024.form\n";
  print "##CMD=$cmd";
  if ($norun == 0) {
    $resp = `$cmd`;
    if ($verbose > 1) {
      print $resp;
    }
  }

  # add_temps with fill rrd file but check for old graphs if not already done
  if ($history_checked == 0) {
    if ($verbose > 0) {
      print "TD2: checking history...\n";
    }
    check_history();
    $history_checked = 1;
  }

  # do the delay but check for abort
  $t = 0;
  while (($notdone == 1) && ($t < $delay)) {
    sleep(1);
    $t++;
  }
}

exit(0);

sub check_week {
  my ($given) = @_;
  my $cmd;
  my $resp;
  my $f = $webbase . "/wrappers/" . $prefix . "-wk-" . $given . ".html";
  if (!(-s $f)) {
    $cmd = $execbase . "/make_week_wrapper.pl " . $prefix . " " . $given . " > " . $f . "\n";
    print "##CMD=" . $cmd;
    if ($norun == 0) {
      $resp = `$cmd`;
      if ($verbose > 1) {
        print $resp;
      }
    }
  }
}

# check for history - there could be a bunch of weeks not already graphed.
# Check for this on startup and generate anything old that is missing.
sub check_history {
}

# return the datestring of the previous (or current) Sunday
sub find_this_week {
  my $w = $realtime - ($dayofweek * (3600 * 24));
  my @t = localtime($w);
  my $a;
  $t[5] += 1900;
  $t[4] += 1;
  $a = sprintf("%04d%02d%02d", $t[5], $t[4], $t[3]);
  return($a);
}

sub load_current_datetime {
  $realtime = time();
  my @t = localtime($realtime);
  $t[5] += 1900;
  $t[4] += 1;
  $datetimestring = sprintf("%04d%02d%02d-%02d%02d%02d", $t[5], $t[4], $t[3], $t[2], $t[1], $t[0]);
  $datestring = sprintf("%04d%02d%02d", $t[5], $t[4], $t[3]);
  $timestring = sprintf("%02d%02d%02d", $t[2], $t[1], $t[0]);
  $dayofweek = $t[6];
}

sub rev_date_string {
  my ($t) = @_;
  my $yr;
  my $mon;
  my $day;
  my $hr;
  my $min;
  my $sec;
  my @tr;
  my $res;
  
  if (!defined($yr = check_number(substr($t, 0, 4)))) {
    return;
  }
  if (!defined($mon = check_number(substr($t, 4, 2)))) {
    return;
  }
  if (!defined($day = check_number(substr($t, 6, 2)))) {
    return;
  }
  $hr = 0;
  $min = 0;
  $sec = 0;
  $tr[0] = int($sec);
  $tr[1] = int($min);
  $tr[2] = int($hr);
  $tr[3] = int($day);
  $tr[4] = int($mon) - 1; # fix up
  $tr[5] = int($yr) - 1900; # fix up
  # check bounds
  if (($tr[0] < 0) || ($tr[0] > 60)) {
    return;
  }
  if (($tr[1] < 0) || ($tr[1] > 59)) {
    return;
  }
  if (($tr[2] < 0) || ($tr[2] > 24)) {
    return;
  }
  if (($tr[3] < 0) || ($tr[3] > 31)) {
    return;
  }
  if (($tr[4] < 0) || ($tr[4] > 11)) {
    return;
  }
  if ($tr[5] < 70) {
    return;
  }
  $res = timelocal($tr[0],$tr[1],$tr[2],$tr[3],$tr[4],$tr[5]);
  return($res);
}

sub check_number {
  my ($given) = @_;
  if (!defined($given)) {
    return;
  }
  my $l = length($given);
  my $i;
  my $a;
  for ($i=0;$i<$l;$i++) {
    $a = substr($given, $i, 1);
    if (($a < "0") || ($a > "9")) {
      return;
    }
  }
  return(int($given));
}

sub find_prefix {
  my $a;
  $a = $0;
  my $i;
  my @b;
  my @c;
  @b = split(/\//, $a);
  for ($i=0;defined($b[$i]);$i++)  { # find last item
  }
  $i--;
  if ($i < 0) {
    print "ERROR - improper prefix\n";
    exit(-1);
  }
  $a = $b[$i];
  $i = length($a);
  if ($i < 11) { # not enough space for a proper name
    print "ERROR - name too small\n";
    exit(-1);
  }
  my $data_prefix;
  $data_prefix = substr($a, 0, $i - 10);
  return($data_prefix);
  #print "found prefix = \"" . $data_prefix . "\"\n";
  #exit(0);
}
