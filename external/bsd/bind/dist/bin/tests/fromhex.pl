#!/usr/bin/perl
#
# Copyright (C) 2015  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

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
