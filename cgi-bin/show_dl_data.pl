#!/usr/bin/perl -w
# Source File : show_dl_data.pl
# Begun : 2011
# Latest Revision : August 19, 2014
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

my $base = "/home/web";
my $execbase = $base . "/datalog";

my $doctype_string = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\">\n";

my $std_wid = 1761;
my $std_hgt = 879;
my %inputkeys;
my $query_string;
my $specmessage;
get_query_string();

# get the file = parameter
my $a = $inputkeys{"date"};
my @b;
my $cmd;
my $resp;
if (!defined($a)) {
  $specmessage = "ERROR - no valid parameter";
} else {
  # extract this date's data and make a image file
  #$cmd = "/home/datalog/dataview/mkgraph-1024-day $a";
  $cmd = $execbase . "/mkgraph " . $a . " " . $execbase . "/day-1024.form";
  #$specmessage = "cmd = " . $cmd . "<BR>\n";
  $resp = `$cmd\n`;
  $specmessage .= "<IMG SRC=\"/datalog/images/" . $prefix . "-day-" . $a . ".png\"><BR>\n";
}

# dump something to the screen
print "Content-type: text/html\n\n";
print $doctype_string;
if (defined($a)) {
  print "<HTML><HEAD><TITLE>Show Date - $a</TITLE></HEAD>\n";
} else {
  print "<HTML><HEAD><TITLE>Show Date</TITLE></HEAD>\n";
}
print "<BODY>\n";

print $specmessage;

print "<BR></BODY></HTML>\n";
exit(0);

#<HEAD>
#<TITLE>wrapper</TITLE>
#</HEAD>
#<BODY>
#<HR>
#<HR>
#
#<H1 ALIGN="CENTER">wrapper</H1>
#
#<A HREF="/cgi-pub/get_coords">
#<IMG SRC="/pub/hm2/images/temp-wk-20090412.png" WIDTH="1761" HEIGHT="879" ISMAP>
#</A><BR>
#</BODY>
#</HTML>


# handle the input stream from form (standard parsing template)
sub get_query_string {
  $query_string = $ENV{'QUERY_STRING'};
  my @keypairs = split(/&/, $query_string);
  my $i;
  my $name;
  my $value;
  my $lastvalue;
  my $cnt = 0;
  #$specmessage = "got to query\n";
  while (defined($i = $keypairs[$cnt])) {
    ($name, $value) = split(/=/, $i);
    $value =~ tr/+/ /; # translates URL spaces
    $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
    # create hash full of posted keys
    #$specmessage .= "got \"$name\" = \"$value\"\n";
    $cnt++;
    if (!defined($keypairs[$cnt])) { # the next will fail
      if (index($value, "#") >= 0) {
        $inputkeys{$name} = $value;
        $lastvalue = $value;
        #$specmessage .= "found pound in query, last value = $value\n";
      } else {
        $inputkeys{$name} = $value;
        $lastvalue = $value;
        #$specmessage .= "NO pound in query, last value = $value\n";
      }
    } else {
      $inputkeys{$name} = $value;
    }
  }
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
