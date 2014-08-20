#!/usr/bin/perl -w
# Source File : update_daily_records.pl
# Begun : 2014
# Latest Revision : August 19, 2014
#
# Copyright (C) 2014 John B. Rosheck, Jr.
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
use strict;

my $base = "/home/web";
my $data = $base . "/data/datalog";

if (!defined($ARGV[0]) || !defined($ARGV[1])) { # this is the data string and must be supplied
  udr_log("ERROR - not enough data given on the command line\n");
  exit(1);
}

my $prefix = $ARGV[0];
my $logfile = $data . "/" . $prefix . "-udr.log";
my $suffix = "txt";

my $datetimestring;
my $datestring;
my $timestring;
my $dayofweek;

# FORMAT: time:ch1:ch2:ch3:ch4:ch5:ch6:ch7:ch8
my $incoming = $ARGV[1];
my $given_time;
my @b;

@b = split(/:/, $incoming);
if (!defined($b[1])) { # this isn't enough data for sure
  udr_log("ERROR - not enough data supplied (\"" . $incoming . "\")\n");
  exit(1);
}
# count the data points
my $data_count = 0;
my $i;
for ($i=1;defined($b[$i]);$i++) {
  $data_count++;
}

$given_time = convert_datetime($b[0]);
my $record_filename = $data . "/" . $prefix . "-records-" . $datestring . "." . $suffix;
my $a;
if (-f $record_filename) { # already there, read and update
  if (open(RFILE, $record_filename)) {
    if (defined($a = <RFILE>)) {
      chomp($a);
      close(RFILE);
      update_record_file($a);
    } else { # its empty, fill it
      close(RFILE);
      create_new_record_file($record_filename);
    }
  }
} else { # doesn't exist yet, create it with the current data
  create_new_record_file($record_filename);
}

exit(0);

sub update_record_file {
  my ($given) = @_;
  # parse the given line from the current record file and compare it with
  # the new data and update hi/lo data as appropiate
  # data organized as data1_hi:timestamp data1_lo:timestamp data2_hi:t...
  my @e = split(/ /, $given); # split into hi/lo data points
  my @d; # use this to seperate data from timestamp
  my $even = 1;
  my $doff = 1;
  my $i;
  my $a;
  for ($i=0;defined($e[$i]);$i++) {
    @d = split(/:/, $e[$i]); # split entry into data and timestamp
    if ($even == 1) { # doing high
      if ($b[$doff] > $d[0]) { # new high record
        $e[$i] = $b[$doff] . ":" . $timestring;
      }
      $even = 0; # now do low
    } else { # doing low
      if ($b[$doff] < $d[0]) { # new low record
        $e[$i] = $b[$doff] . ":" . $timestring;
      }
      $doff++; # advance pointer to data
      $even = 1; # now do high
    }
  }
  # now dump the update to the records file
  if (open(NRFILE, ">$record_filename")) {
    for ($i=0;defined($e[$i]);$i++) {
      if ($i == 0) {
        $a = $e[$i];
      } else {
        $a .= " " . $e[$i];
      }
    }
    $a .= "\n";
    print NRFILE $a;
    close(NRFILE);
  }
}

sub create_new_record_file {
  my ($given) = @_;
  my $a;
  my $i;
  for ($i=1;$i<=$data_count;$i++) {
    # record the current values as both hi and low with the current time
    if ($i == 1) {
      $a = $b[$i] . ":" . $timestring . " " . $b[$i] . ":" . $timestring;
    } else {
      $a .= " " . $b[$i] . ":" . $timestring . " " . $b[$i] . ":" . $timestring;
    }
  }
  $a .= "\n";
  if (open(NFILE, ">$given")) {
    print NFILE $a;
    udr_log("started new record file \"" . $given . "\"\n");
    close(NFILE);
  } else {
    udr_log("ERROR - couldn't start new record file \"" . $given . "\"\n");
  }
}

sub convert_datetime {
  my ($realtime) = @_;
  my @t = localtime($realtime);
  $t[5] += 1900;
  $t[4] += 1;
  $datetimestring = sprintf("%04d%02d%02d-%02d%02d%02d", $t[5], $t[4], $t[3], $t[2], $t[1], $t[0]);
  $datestring = sprintf("%04d%02d%02d", $t[5], $t[4], $t[3]);
  $timestring = sprintf("%02d%02d%02d", $t[2], $t[1], $t[0]);
  $dayofweek = $t[6];
  return($timestring);
}

sub udr_log {
  my ($given) = @_;
  chomp($given);
  $given .= "\n";
  if (open(LFILE, ">>$logfile")) {
    print LFILE $given;
  }
  close(LFILE);
}
