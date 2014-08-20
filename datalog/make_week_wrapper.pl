#!/usr/bin/perl -w
# Source File : make_week_wrapper
# Begun : June 26, 2006
# Last Updated : July 1, 2014
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

my $doctype_string = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\">\n";

my $prefix;
my $std_wid = 1761;
my $std_hgt = 879;

if (!defined($ARGV[0]) || !defined($ARGV[1])) {
  print "ERROR - must specify a prefix and date in the form YYYYMMDD\n";
  exit(1);
}

$prefix = $ARGV[0];

# need to convert the given date to a time plus construct dates for the full
# week.
my @dt; # this is filled with YYYYMMDD date stamps for the week, index 0 to 6
my $a;
if (!defined($a = get_all_days_in_week($ARGV[1]))) {
  print "ERROR - malformed date string, use the form YYYYMMDD\n";
  exit(1);
}

# dump something to the screen
#print "Content-type: text/html\n\n"; - not needed for static page
print $doctype_string;
print "<HTML><HEAD><TITLE>Week of $a</TITLE></HEAD>\n";
print "<BODY>\n";
print "<H1 ALIGN=\"CENTER\">Week of $a</H1>\n";

print "<MAP NAME=\"wkmap\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[0] . "\" SHAPE=RECT COORDS=\"52,33,291,833\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[1] . "\" SHAPE=RECT COORDS=\"292,33,531,833\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[2] . "\" SHAPE=RECT COORDS=\"532,33,771,833\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[3] . "\" SHAPE=RECT COORDS=\"772,33,1011,833\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[4] . "\" SHAPE=RECT COORDS=\"1012,33,1251,833\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[5] . "\" SHAPE=RECT COORDS=\"1252,33,1491,833\">\n";
print "<AREA HREF=\"/cgi-bin/" . $prefix . "_show_date.pl" . '?' . "date=" . $dt[6] . "\" SHAPE=RECT COORDS=\"1492,33,1731,833\">\n";
print "</MAP>\n";

print "<A HREF=\"/cgi-bin/get_coords" . '?' . "wk=" . $dt[0] . "\">\n";
print "<IMG SRC=\"/datalog/images/" . $prefix . "-wk-" . $dt[0] . ".png\" WIDTH=\"1761\" HEIGHT=\"879\" USEMAP=\"#wkmap\">\n";
print "</A>\n";

print "<BR></BODY></HTML>\n";
exit(0);

# needs to return the given date in the form MM/DD/YYYY and fill the array
# dt[0-6] with the entire weeks of dates of the form YYYYMMDD.
sub get_all_days_in_week {
  my ($dstr) = @_;
  my $tsec;
  my $long_date;
  if (check_is_number($dstr) == 0) { # yes it is
    #print "passed check is number\n";
    # convert the given time into seconds
    if (defined($tsec = rev_date_string($dstr))) {
      #print "passed rev date string\n";
      # this should be the time in seconds from epoch
      $long_date = get_date_string($tsec);
      my $i;
      for ($i=0;$i<7;$i++) {
        $dt[$i] = get_YYYYMMDD_string($tsec);
        $tsec += 86400;
      }
      return($long_date);
    } else {
      return(undef);
    }
  }
  return(undef); # will exit out on an error
}

sub check_is_number {
  my ($v) = @_;
  my $l = length($v);
  if ($l == 0) {
    return(1);
  }
  my $i;
  my $a;
  for ($i=0;$i<$l;$i++) {
    $a = substr($v, $i, 1);
    if (($a lt "0") || ($a gt "9")) {
      return(1);
    } # else don't do anything
  }
  return(0);
}

sub get_dtime_string {
  my ($a) = @_;
  my @t = localtime($a);
  $t[5] += 1900; # fix up year
  $t[4] += 1; # fix up month
  my $r = sprintf("%02d/%02d/%04d %02d:%02d:%02d", $t[4], $t[3], $t[5], $t[2], $t[1], $t[0]);
  return($r);
}

sub get_date_string {
  my ($a) = @_;
  my @t = localtime($a);
  $t[5] += 1900; # fix up year
  $t[4] += 1; # fix up month
  my $r = sprintf("%02d/%02d/%04d", $t[4], $t[3], $t[5]);
  return($r);
}

sub get_YYYYMMDD_string {
  my ($a) = @_;
  my @t = localtime($a);
  $t[5] += 1900; # fix up year
  $t[4] += 1; # fix up month
  my $r = sprintf("%04d%02d%02d", $t[5], $t[4], $t[3]);
  return($r);
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
    return(undef);
  }
  if (!defined($mon = check_number(substr($t, 4, 2)))) {
    return(undef);
  }
  if (!defined($day = check_number(substr($t, 6, 2)))) {
    return(undef);
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
    return(undef);
  }
  if (($tr[1] < 0) || ($tr[1] > 59)) {
    return(undef);
  }
  if (($tr[2] < 0) || ($tr[2] > 24)) {
    return(undef);
  }
  if (($tr[3] < 0) || ($tr[3] > 31)) {
    return(undef);
  }
  if (($tr[4] < 0) || ($tr[4] > 11)) {
    return(undef);
  }
  if ($tr[5] < 70) {
    return(undef);
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
