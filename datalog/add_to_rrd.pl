#!/usr/bin/perl -w
# Source File : add_to_rrd.pl
# Begun : May 27, 2006
# Last Updated : June 28, 2014
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

my $base = "/home/web";
my $logs = $base . "/data/datalog";

my $prefix;
if (!defined($ARGV[0])) {
  print "ERROR - no prefix passed to add_to_rrd.pl\n";
  exit(1);
}

$prefix = $ARGV[0];

my $rrdfile = $base . "/data/rrd/" . $prefix . ".rrd";
my $ebase = $base . "/datalog";
my $store_file = $ebase . "/.add_to_" . $prefix . "_rrd_data";
my $config_file = $ebase . "/" . $prefix . "_data.config";
my $run = 1;

my $last_time;
$last_time = get_last_entry_time($rrdfile);
if (!defined($last_time)) {
  print "ERROR - couldn't find final timestamp in $rrdfile\n";
  exit(1);
}
print "last timestamp = $last_time\n";
my $a = get_file_timestring($last_time);
print "converted = \"$a\"\n";

my @b = split(/-/, $a);
my @files;
print "looking for dates on or after $b[0]\n";
@files = find_rest_of_date_files($b[0]);

my $dt;
my $i;
my $k;
my $cmd;
my $resp;
my $j;

my $cur_logfile;

my $cache_file;
my $cache_offset;

# see if there's a cache file
if (-s $store_file) {
  if (open(CFILE, $store_file)) {
    while (defined($a = <CFILE>)) {
      if (defined($k = find_variable($a, "offset:"))) {
        print "found cached offset = $k\n";
        $cache_offset = $k;
      }
      if (defined($k = find_variable($a, "file:"))) {
        print "found cached file for offset = $k\n";
        $cache_file = $k;
      }
    }
  } else {
    print "ERROR - couldn't open cache file - $!\n";
  }
}

for ($i=0;defined($files[$i]);$i++) {
#  $dt = sprintf("%s%02d", $mon, $i);
  $dt = $files[$i];
  $cur_logfile = $logs . "/" . $prefix . "-" . $dt . ".log";
  if (defined($cache_file)) {
    if ($cache_file eq $cur_logfile) {
      print "using cached offset for file $cur_logfile, offset = $cache_offset\n";
      $cmd = "$ebase/parse_log_file.pl $config_file $cur_logfile $rrdfile $cache_offset\n";
    } else {
      $cmd = "$ebase/parse_log_file.pl $config_file $cur_logfile $rrdfile\n";
    }
  } else {
    $cmd = "$ebase/parse_log_file.pl $config_file $cur_logfile $rrdfile\n";
  }
  print "cmd = $cmd";
  if ($run >= 1) {
    $resp = `$cmd`;
    print $resp;
    @b = split(/\n/, $resp);
    for ($j=0;defined($b[$j]);$j++) {
      if (length($b[$j]) >= 7) {
        if (substr($b[$j], 0, 7) eq "offset:") {
          print "got offset line : $b[$j]\n";
          $a = substr($b[$j], 7, length($b[$j]) - 7);
          print "got offset value of $a\n";
          # if this is the last file in the list, then save this to the cache
          $k = $i + 1;
          if (!defined($files[$k])) { # no more files
            if (open(CFILE, ">$store_file")) {
              print CFILE "file:" . $logs . "/" . $prefix . "-" . $dt . ".log\n";
              print CFILE "offset:" . $a . "\n";
              close(CFILE);
            } else {
              print "ERROR - couldn't open data cache file - $!\n";
            }
          }
        }
      }
    }
  }
}

exit(0);

sub find_variable {
  my ($l, $v) = @_;
  my $a;
  if (length($l) >= length($v)) {
    if (substr($l, 0, length($v)) eq $v) {
      $a = substr($l, length($v), length($l) - length($v));
      chomp($a);
      if (length($a) > 0) {
        return($a);
      }
    }
  }
  return(undef);
}

sub find_rest_of_date_files {
  my ($lt) = @_;
  my $fcnt=0;
  my $a;
  my $c;
  my @found;
  if (!opendir(SDIR, $logs)) {
    print "ERROR - couldn't open dir \"$logs\" - $!\n";
    return;
  }
  while (defined($a = readdir(SDIR))) {
    if (($a ne ".") && ($a ne "..")) {
      if (defined($c = get_time_field($a))) {
        if ($c ge $lt) {
          $found[$fcnt] = $c;
          print "file \"$a\" - time \"$found[$fcnt]\"\n";
          $fcnt++;
        } else {
          # print "skipping \"$a\"\n";
        }
      }
    }
  }
  closedir(SDIR);
  @found = sort(@found);
  return(@found);
}

sub get_time_field {
  my ($given) = @_;
  my $a;
  my $c;
  my $i;
  $i = rindex($given, ".");
  if ($i < 8) {
    return;
  }
  if (length($given) < (length($prefix) + 1)) {
    return; # not big enough
  }
  if ($i != (length($prefix) + 1 + 8)) {
    return;
  }
  $a = substr($given, 0, length($prefix) + 1);
  if ($a ne ($prefix . "-")) {
    return; # not the right prefix
  }
  $a = substr($given, $i - 8, 8);
  for ($i=0;$i<8;$i++) {
    $c = substr($a, $i, 1);
    if (($c lt "0") || ($c gt "9")) {
      return;
    }
  }
  return($a);
}

sub last_day_of_month {
  my ($given) = @_;
  my $yr;
  my $mo;
  $yr = substr($given, 0, 4);
  $mo = substr($given, 4, 2);
  if (($mo == 1) || ($mo == 3) || ($mo == 5) || ($mo == 7) || ($mo == 8) || ($mo == 10) || ($mo == 12)) {
    return(31);
  } else {
    if ($yr % 4) {
      if ($mo == 2) {
        return(29);
      }
    } else {
      if ($mo == 2) {
        return(28);
      }
    }
  }
  if ($mo <= 12) {
    return(30);
  }
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

sub get_file_timestring {
  my ($given) = @_;
  if (!defined($given)) {
    $given = time();
  }
  my @lt = localtime($given);
  $lt[4] += 1;
  $lt[5] += 1900;
  my $a = sprintf("%04d%02d%02d-%02d%02d%02d", $lt[5], $lt[4], $lt[3], $lt[2], $lt[1], $lt[0]);
  return($a);
}
