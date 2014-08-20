#!/usr/bin/perl -w
# Source File : parse_log_file.pl
# Begun : May 26, 2006
# Last Updated : June 27, 2014
#
# This script takes a data log file and reads each line decoding the
# appropiate entries.  If specified, the timestamp at the start of the
# line is used to determine whether the attempt to load the data into
# the rrd file is attempted.
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

my $verbose=1;

my $data_config_file;
# the next 3 items are filled by the config file
my $prefix;
my $first_data_parse;
my $data_parse_count;
my $datafile;
my $rrdfile;
my $file_offset;

if (!defined($ARGV[0]) || !defined($ARGV[1]) || !defined($ARGV[2])) {
  print "ERROR - must supply parameters\n";
  print "parse_log_file.pl usage: \n";
  print "  parse_log_file.pl data_config_file data_file rrd_file [seek_offset]\n";
  print "    (options must be left out for now)\n";
  exit(0);
}

$data_config_file = $ARGV[0];
if (-s $data_config_file) {
  # cool - got it
  if ($verbose > 0) {
    print "data config file \"$data_config_file\" selected.\n";
  }
  read_config_file();
} else { # no results file found
  print "ERROR - can't find config file \"$data_config_file\"\n";
  exit(1);
}

$datafile = $ARGV[1];
if (-s $datafile) {
  # cool - got it
  if ($verbose > 0) {
    print "results file \"$datafile\" selected.\n";
  }
} else { # no results file found
  print "ERROR - can't find results file \"$datafile\"\n";
  exit(1);
}

$rrdfile = $ARGV[2];
my $last_time;

if (-s $rrdfile) {
  # cool, found that too
  if ($verbose > 0) {
    print "rrd file \"$rrdfile\" selected.\n";
  }
  # find last time in rrd file
  $last_time = get_last_entry_time($rrdfile);
  if (!defined($last_time)) {
    print "ERROR - couldn't find final timestamp in $rrdfile\n";
    exit(1);
  }
  print "last valid rrd timestamp = $last_time\n";
} else { # no rrd file to stuff
  print "ERROR - can't find rrd file \"$rrdfile\"\n";
  exit(1);
}

# check for a seek value
if (defined($ARGV[3])) {
  # this should be a seek value, make sure its a number
  $file_offset = $ARGV[3];
  if (check_is_number($file_offset) == 0) { # good
    if ($verbose > 0) {
      print "offset will be set to " . $file_offset . "\n";
    }
  } else { # not good, should fail here
    print "ERROR - seek value is not a number!\n";
    exit(1);
  }
}

my $i;
my $j = 1;
my $tokens; # = "ch1:ch2:ch3:ch4:ch5:ch6";
for ($i=0;$i<$data_parse_count;$i++) {
  if ($i != 0) {
    $tokens .= ":ch" . $j;
  } else {
    $tokens = "ch" . $j;
  }
  $j++;
}

#print "tokens = \"" . $tokens . "\"\n";
my $starttime = 0;
my $endtime = 20000000000;

my $infile;
$infile = $datafile;
# Try to open file :
if ($verbose > 0) {
  print "trying to open file \"$infile\"\n";
}
if (!open(AFILE, $infile)) {
  print "ERROR - couldn't open file - $!\n";
  exit(1);
}
if (defined($file_offset)) {
  if (seek(AFILE, $file_offset, 0)) {
    if ($verbose > 0) {
      print "opened file to position $file_offset\n";
    }
  } else { # error on seek, must error out
    close(AFILE);
    print "ERROR - failed file seek to $file_offset on file $infile\n";
    exit(1);
  }
} else {
  if ($verbose > 0) {
    print "opened file, scanning...\n";
  }
}
my @b;
my $lcnt = 0;
my $ocnt = 0; # offset into file
if (defined($file_offset)) {
  $ocnt = $file_offset;
}
my $resp;
my $cntr=0;
my $last_data_parse = $first_data_parse + $data_parse_count - 1;
while (defined($a = <AFILE>)) {
  print "raw line = $a";
  $ocnt += length($a); # keep track of the current line seek in the data file
  $lcnt++;
  @b = split(/ +/, $a);
  # test that this is usable
  if (defined($b[$last_data_parse]) && (substr($a, length($a) - 1, 1) eq "\n")) { # got something
    if (($b[0] >= $starttime) && ($b[0] < $endtime)) {
      chomp($a);
      if ($verbose > 1) {
        print "got line = \"$a\"\n";
      } else {
        if ($verbose > 0) {
          $cntr++;
          if ($cntr >= 80) {
            print ".";
            $cntr = 0;
          }
        }
      }
      # do the line entry parse
      if (defined($b[$last_data_parse])) { # good line
        $a = $b[0];
        for ($i=$first_data_parse;$i<=$last_data_parse;$i++) {
          $a .= ":" . $b[$i];
        }
        if ($b[0] > $last_time) {
          do_update($a);
        } else {
          print "WARNING: time $a has already been written to the rrd\n";
        }
      } else {
        if ($verbose > 0) {
          print "ERROR - line ended early\n";
        }
      }
    }
  } else {
    if ($verbose > 0) {
      print "end of complete data lines\n";
      print "done: lines = $lcnt\n";
      print "offset:$ocnt\n";
      close(AFILE);
      exit(0);
    }
  }
}
if ($verbose >= 1) {
  print "\ndone: lines = $lcnt\n";
  print "offset:$ocnt\n";
}
close(AFILE);

exit(0);

sub do_update {
  my ($given_data) = @_;
  my $cmd;
  my $resp;

  #print "given data = \"" . $given_data . "\"\n";
  
  $cmd = "/home/web/datalog/update_daily_records.pl";
  $cmd .= " " . $prefix . " " . $given_data . "\n";
  print "running $cmd";
  $resp = `$cmd`;
  print "results = $resp";
  
  $cmd = "rrdtool update";
  $cmd .= " $rrdfile";
  $cmd .= " -t $tokens";
  $cmd .= " $given_data";
  $cmd .= " 2>&1";
  $cmd .= "\n";
  #print $cmd;
  $resp = `$cmd`;
  if ($resp ne "") {
    if ($verbose > 0) {
      #print "no error\n";
    }
  } else {
    print $resp;
  }
}

sub process_options {
  return(0);
}

sub check_is_number {
  my ($v) = @_;
  return(0);
}

sub read_config_file {
  my $a;
  my @b;
  if (!open(CFILE, $data_config_file)) {
    print "ERROR - couldn't open config file \"" . $data_config_file . "\" - $!\n";
    exit(1);
  }
  while (defined($a = <CFILE>)) {
    chomp($a);
    @b = split(/ +/, $a);
    if (defined($b[0]) && (length($b[0]) > 0)) {
      if (substr($b[0], 0, 1) ne '#') { # try to parse config information
        if (defined($b[0]) && defined($b[1]) && defined($b[2])) {
          $prefix = $b[0];
          $first_data_parse = $b[1];
          $data_parse_count = $b[2];
          print "found config information - \"" . $prefix . "\", \"" . $first_data_parse . "\", \"" . $data_parse_count . "\"\n";
          return(0);
        }
      }
    }
  }
  print "ERROR - couldn't find config parameters.\n";
  exit(1);
}

sub get_last_entry_time {
  my ($rfile) = @_;
  my $cmd;
  my @resp;
  my @b;
  my $i;

  $cmd = "rrdtool info $rfile\n";
  @resp = `$cmd`;
  $i = 0;
  while (defined($resp[$i])) {
    print $resp[$i];
    @b = split(/ = /, $resp[$i]);
    if ($b[0] eq "last_update") {
      chomp($b[1]);
      return($b[1]);
    }
    $i++;
  }
  return;
}
