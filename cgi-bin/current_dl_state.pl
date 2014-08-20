#!/usr/bin/perl -w
# Source File : current_dl_state.pl
# Begun : 2006
# Latest Revision : August 19, 2014
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

my $data_name;
if (defined($ARGV[0])) {
  $data_name = $ARGV[0];
} else {
  $data_name = "DataLog";
}

my $prefix;

$prefix = find_prefix(); # from the running program's file name

my $base = "/home/web";
my $log_path = $base . "/data/datalog";
my $exec_path = $base . "/dataview";
my $web_image_path = "/datalog/images";
my $lfile_prefix = $prefix . "-";
my $lfile_suffix = ".log";

my @tvar;
@tvar = localtime(time());
$tvar[5] += 1900;
$tvar[4] += 1;

my $weekdatestring;
my $datestring;
$datestring = sprintf("%04d%02d%02d", $tvar[5], $tvar[4], $tvar[3]);
my $ecode;

my $logfile = $log_path . "/" . $lfile_prefix . $datestring . $lfile_suffix;
my $realstring;
my $cmd;
my $resp;

if (-f $logfile) {
  $cmd = "tail -1 ";
  $cmd .= $logfile;
  $cmd .= "\n";
  $resp = `$cmd`;
  $realstring = $resp;
} else {
  $ecode = "ERROR - missing log file - logging program way be offline";
}

print "Content-type: text/html\n\n";

print "<HR>\n";
print "<H1 ALIGN=\"CENTER\">" . $data_name . " Data</H1>\n";

# values filled by parse_realstring()
my $sample_time;
my $temp1;
my $temp2;
my $temp3;
my $temp4;
my $temp5;
my $temp6;

if (!defined($ecode)) {
  #print "realstring = " . $realstring . "<BR>\n";
  $ecode = parse_realstring($realstring);
  if (!defined($ecode)) {
    print "<H3 ALIGN=\"CENTER\">Sample Time : " . $sample_time . "</H3>\n";
    print "<TABLE BORDER=\"1\" WIDTH=\"95%\" ALIGN=\"CENTER\">\n";
    print "<TR><TH WIDTH=\"14.3%\">Temp #1</TH>\n";
    print "<TH WIDTH=\"14.3%\">Temp #2</TH>\n";
    print "<TH WIDTH=\"14.3%\">Temp #3</TH>\n";
    print "<TH WIDTH=\"14.3%\">Temp #4</TH>\n";
    print "<TH WIDTH=\"14.3%\">Temp #5</TH>\n";
    print "<TH WIDTH=\"14.3%\">Temp #6</TH></TR>\n";
    print "<TR><TD ALIGN=\"CENTER\"><B>" . $temp1 . "&deg;F</B></TD>\n";
    print "<TD ALIGN=\"CENTER\"><B>" . $temp2 . "&deg;F</B></TD>\n";
    print "<TD ALIGN=\"CENTER\"><B>" . $temp3 . "&deg;F</B></TD>\n";
    print "<TD ALIGN=\"CENTER\"><B>" . $temp4 . "&deg;F</B></TD>\n";
    print "<TD ALIGN=\"CENTER\"><B>" . $temp5 . "&deg;F</B></TD>\n";
    print "<TD ALIGN=\"CENTER\"><B>" . $temp6 . "&deg;F</B></TD></TR>\n";
    print "</TABLE><BR>\n";
    $cmd = $exec_path . "/mkgraph-320-day " . $datestring;
    system("$cmd >& /dev/null\n");
    $cmd = $exec_path . "/mkgraph-1024-day " . $datestring;
    system("$cmd >& /dev/null\n");
    print "<DIV ALIGN=\"CENTER\">\n";
    print "<A HREF=\"" . $web_image_path . "/" . $prefix . "-day-" . $datestring . ".png\">\n";
    print "<IMG SRC=\"" . $web_image_path . "/" . $prefix . "-sm-day-" . $datestring . ".png\"></A><BR>\n";
    print "</DIV>\n";
    print "<DIV ALIGN=\"CENTER\">\n";
    print "<A HREF=\"" . $web_image_path . "/" . $prefix . "-wk-" . $weekdatestring . ".png\">\n";
    print "<IMG SRC=\"" . $web_image_path . "/" . $prefix . "-sm-wk-" . $weekdatestring . ".png\"></A><BR>\n";
    print "</DIV>\n";
    print "<DIV ALIGN=\"CENTER\">\n";
    print "<H4><A HREF=\"/cgi-bin/show_" . $prefix . "_archive.pl\">Show Archive</A></H4>\n";
    print "</DIV>\n";
  } else {
    print "$ecode<BR>\n";
  }
} else {
  print "$ecode<BR>\n";
  print "datestring = " . $datestring . "<BR>\n";
  print "logfile = " . $logfile . "<BR>\n";
}

#if (defined($ecode)) {
#  print $ecode;
#  print "<HR>\n";
#  exit(0);
#}

print "<HR>\n";

exit(0);

sub parse_realstring {
  my ($logline) = @_;
  my @b;
  my $a;
  my $c;
  chomp($logline);
  @b = split(/ +/, $logline);
  if (defined($b[11])) { # valid
    $c = $b[0];
    $sample_time = get_real_date($c);
    $temp1 = $b[6];
    $temp2 = $b[7];
    $temp3 = $b[8];
    $temp4 = $b[9];
    $temp5 = $b[10];
    $temp6 = $b[11];
  } else {
    return("ERROR - not enough items read");
  }
  return(undef);
}

sub get_real_date {
  my ($given) = @_;
  my @t;
  @t = localtime($given);
  my $a;
  $t[5] += 1900;
  $t[4] += 1;
  $a = sprintf("%04d%02d%02d-%02d%02d%02d", $t[5], $t[4], $t[3], $t[2], $t[1], $t[0]);
  # find the last Sunday
  my $i;
  my $j;
  $i = (24 * 60 * 60) * $t[6]; # day in week, Sunday is 0, go back to it
  $j = $given - $i;
  @t = localtime($j);
  $t[5] += 1900;
  $t[4] += 1;
  $weekdatestring = sprintf("%04d%02d%02d", $t[5], $t[4], $t[3]);
  return($a);
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
  if ($i < 18) { # not enough space for a proper name
    print "ERROR - name too small\n";
    exit(-1);
  }
  my $data_prefix;
  $data_prefix = substr($a, 8, $i - 17);
  return($data_prefix);
  #print "found prefix = \"" . $data_prefix . "\"\n";
  #exit(0);
}
