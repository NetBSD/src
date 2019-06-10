#!/usr/bin/perl
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# Converts hex ascii into raw data.
# (This can be used, for example, to construct input for "wire_data -d".)

require 5.006.001;

use strict;
use IO::File;

sub usage {
    print ("Usage: packet.pl [file]\n");
    exit 1;
}

my $file = "STDIN";
if (@ARGV >= 1) {
    my $filename = shift @ARGV;
    open FH, "<$filename" or die "$filename: $!";
    $file = "FH";
}

my $input = "";
while (defined(my $line = <$file>) ) {
    chomp $line;
    $line =~ s/#.*$//;
    $input .= $line;
}

$input =~ s/\s+//g;
my $data = pack("H*", $input);
my $len = length $data;

binmode(STDOUT);
print($data);
exit(0);
