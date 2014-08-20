#!/usr/bin/perl -w
# Source File : show_dl_archive.pl
# Begun : <= 2011
# Latest Revision : June 26, 2014
#
# Copyright (C) 2011-2014 John B. Rosheck, Jr.
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

my $prefix;

$prefix = find_prefix(); # from the running program's file name

my $base = "/home/web/html";
my $img_ref = "/datalog/images";
my $wrap_ref = "/datalog/wrappers";
my $img_dir = $base . $img_ref;
my $wrap_dir = $base . $wrap_ref;
my $report_dir = "/home/web/data/datalog";

print "Content-type: text/html\n\n";

print "<HR>\n";
print "<H1 ALIGN=\"CENTER\">Datalog - History by Week</H1>\n";
print "<H3 ALIGN=\"CENTER\">(Click on image for expanded view)</H3>\n";

my @files;
my $fcnt;
my $sm_prefix = $prefix . "-sm-wk";
my $big_prefix = $prefix . "-wk-";
my $big_suffix = ".png";
my $wrap_prefix = $prefix . "-wk-";
my $wrap_suffix = ".html";

@files = find_date_files($img_dir, $sm_prefix);

@files = sort(@files);
@files = reverse(@files);
my $i = 0;
my $t;
my $f;
my $w;
while (defined($files[$i])) {
  #print "file #$i = \"$files[$i]\"\n";
  $t = get_filename_time($files[$i]);
  $f = $big_prefix . $t . $big_suffix;
  $w = $wrap_prefix . $t . $wrap_suffix;
  # construct a dual section - left is graph, right is table
  print "<TABLE WIDTH=\"100%\">\n";
  print "<TR><TD WIDTH=\"50%\">\n";
  if (-s "$wrap_dir/$w") {
    print "<A HREF=\"" . $wrap_ref . "/" . $w . "\">\n";
    print "<IMG SRC=\"" . $img_ref . "/" . $files[$i] . "\"></A><BR>\n";
  } else { # no wrapper file
    print "<A HREF=\"" . $img_ref . "/" . $f . "\">\n";
    print "<IMG SRC=\"" . $img_ref . "/" . $files[$i] . "\"></A><BR>\n";
  }
  print "</TD><TD WIDTH=\"50%\">\n";
  #print "&nbsp;";
  show_results_table($t);
  print "</TD></TR></TABLE>\n";
  print "<BR>\n";
  $i++;
}
#print "there were $i files found\n";
print "<HR>\n";
print "<HR>\n";

exit(0);

sub get_report_file {
  my ($d) = @_;
  my $f;
  my $a;
  my @b;
  $f = $report_dir . "/" . $prefix . "-records-" . $d . ".txt";
  if (-s $f) {
    if (open(RFILE, $f)) {
      if (defined($a = <RFILE>)) {
        chomp($a);
        @b = split(/ /, $a);
      }
      close(RFILE);
      return(@b);
    }
  }
  return(undef);
}

sub convert_datareading {
  my ($given) = @_;
  my @b;
  my $a;
  @b = split(/:/, $given);
  $a = sprintf("%3.1f", $b[0]);
  return($a);
}

sub show_results_table {
  my ($t) = @_;
  # get the dates starting from the given date
  my $i;
  my $d;
  my $a;
  my $j;
  my @dt;
  my $hi;
  my $lo;
  $d = $t;
  print "<TABLE BORDER=\"1\">\n";
  print "<TR>\n";
  print "<TH WIDTH=\"12.5%\">Date</TH>\n";
  print "<TH WIDTH=\"12.5%\">Rtemp Hi/Lo</TH>\n";
  print "<TH WIDTH=\"12.5%\">IHtmp Hi/Lo</TH>\n";
  print "<TH WIDTH=\"12.5%\">ILtmp Hi/Lo</TH>\n";
  print "<TH WIDTH=\"12.5%\">WHtmp Hi/Lo</TH>\n";
  print "<TH WIDTH=\"12.5%\">WLtmp Hi/Lo</TH>\n";
  print "<TH WIDTH=\"12.5%\">Out  Hi/Lo</TH>\n";
  print "<TH WIDTH=\"12.5%\">Sol  Hi/Lo</TH>\n";
  print "</TR>\n";
  #print $d . "<BR>\n";
  for ($i=0;$i<7;$i++) {
    @dt = get_report_file($d);
    print "<TR>\n";
    $a = substr($d, 4, 2) . "/";
    $a .= substr($d, 6, 2);
    # $a .= substr($d, 0, 4); - ignore year
    print "<TD ALIGN=\"CENTER\">";
    print "<A HREF=\"/cgi-bin/datalog/dl_show_raw" . '?' . "date=" . $d . "\">$a</A></TD>\n";
    for ($j=0;$j<8;$j++) {
      if ($j != 1) {
        if (defined($dt[$j*2]) && defined($dt[($j*2)+1])) {
          $hi = convert_datareading($dt[$j*2]);
          $lo = convert_datareading($dt[($j*2)+1]);
          print "<TD ALIGN=\"CENTER\">" . $hi . "/" . $lo . "</TD>\n";
        } else {
          print "<TD ALIGN=\"CENTER\">OUCH</TD>\n";
        }
      }
    }
    #print "<TD ALIGN=\"RIGHT\">2</TD>\n";
    #print "<TD ALIGN=\"RIGHT\">3</TD>\n";
    #print "<TD ALIGN=\"RIGHT\">4</TD>\n";
    #print "<TD ALIGN=\"RIGHT\">5</TD>\n";
    #print "<TD ALIGN=\"RIGHT\">6</TD>\n";
    #print "</TR>\n";
    $d = next_day($d);
  }
  print "</TABLE>\n";
}

sub find_date_files {
  my ($dir, $prefix) = @_;
  my $fcnt=0;
  my $a;
  my $c;
  my @found;
  if (!opendir(SDIR, $dir)) {
    print "ERROR - couldn't open dir \"$dir\" - $!\n";
    return;
  }
  while (defined($a = readdir(SDIR))) {
    if (($a ne ".") && ($a ne "..")) {
      #print "got file $a\n";
      if (defined($c = get_timed_file_prefix($a, $prefix))) {
        $found[$fcnt] = $c;
        #print "file \"$a\" - time \"$found[$fcnt]\"\n";
        $fcnt++;
      } else {
        # print "skipping \"$a\"\n";
      }
    }
  }
  closedir(SDIR);
  return(@found);
}

sub get_timed_file_prefix {
  my ($given, $prefix) = @_;
  my $a;
  my $c;
  my $i;
  my $ri;
  $ri = rindex($given, ".");
  if ($ri < 8) {
    return;
  }
  $a = substr($given, $ri - 8, 8);
  for ($i=0;$i<8;$i++) {
    $c = substr($a, $i, 1);
    if (($c lt "0") || ($c gt "9")) {
      return;
    }
  }
  if (defined($prefix) && (length($prefix) > 0)) {
    $a = substr($given, 0, $ri - 9);
    #print "testing \"$prefix\" against \"$a\" from \"$given\"\n";
    if ($a eq $prefix) {
      return($given);
    }
  } elsif (defined($prefix)) {
    return($given);
  }
  return;
}

sub get_filename_time {
  my ($given) = @_;
  my $a;
  my $c;
  my $i;
  my $ri;
  $ri = rindex($given, ".");
  if ($ri < 8) {
    return;
  }
  $a = substr($given, $ri - 8, 8);
  for ($i=0;$i<8;$i++) {
    $c = substr($a, $i, 1);
    if (($c lt "0") || ($c gt "9")) {
      return;
    }
  }
  return($a);
}

# find the next year/mon/day given a year/mon/day
sub next_day {
  my ($date) = @_;
  my $yr;
  my $mon;
  my $day;
  $yr = substr($date, 0, 4);
  $mon = substr($date, 4, 2);
  $day = substr($date, 6, 2);
  my $new_yr;
  my $new_mon;
  my $new_day;
  my @days_per_month;
  $days_per_month[0] = 31;
  if (($yr % 4) == 0) { # leap year
    $days_per_month[1] = 29;
  } else {
    $days_per_month[1] = 28;
  }
  $days_per_month[2] = 31;
  $days_per_month[3] = 30;
  $days_per_month[4] = 31;
  $days_per_month[5] = 30;
  $days_per_month[6] = 31;
  $days_per_month[7] = 31;
  $days_per_month[8] = 30;
  $days_per_month[9] = 31;
  $days_per_month[10] = 30;
  $days_per_month[11] = 31;

  #print "got $yr year, $mon month, $day day\n";
  if (($mon < 1) || ($mon > 12)) {
    print "month $mon out of range\n";
    return(undef);
  }
  if ($day < 1) {
    print "day $day out of range\n";
    return(undef);
  }
  if ($day > $days_per_month[$mon - 1]) {
    print "day $day out of range\n";
    return(undef);
  }
  if ($day != $days_per_month[$mon - 1]) {
    $new_day = $day + 1;
    $new_mon = $mon;
    $new_yr = $yr;
  } else { # last day of the month
    if ($mon == 12) {
      $new_day = 1;
      $new_mon = 1;
      $new_yr = $yr + 1;
    } else {
      $new_day = 1;
      $new_mon = $mon + 1;
      $new_yr = $yr;
    }
  }
  my $a;
  $a = sprintf("%04d%02d%02d", $new_yr, $new_mon, $new_day);
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
  if ($i < 17) { # not enough space for a proper name
    print "ERROR - name too small\n";
    exit(-1);
  }
  my $data_prefix;
  $data_prefix = substr($a, 5, $i - 16);
  #print "found prefix = \"" . $data_prefix . "\"\n";
  return($data_prefix);
  #exit(0);
}
