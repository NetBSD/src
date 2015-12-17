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

# fetch.pl:
# Simple script to fetch HTTP content from the statistics channel
# of a BIND server. Fetches the full XML stats from 10.53.0.2 port
# 8853 by default; these can be overridden by command line arguments.

use File::Fetch;
use Getopt::Std;

sub usage {
    print ("Usage: fetch.pl [-s address] [-p port] [path]\n");
    exit 1;
}

my %options={};
getopts("s:p:", \%options);

my $addr = "10.53.0.2";
$addr = $options{a} if defined $options{a};

my $path = 'xml/v3';
if (@ARGV >= 1) {
    $path = shift @ARGV;
}

my $port = 8853;
$port = $options{p} if defined $options{p};

my $ff = File::Fetch->new(uri => "http://$addr:$port/$path");
my $file = $ff->fetch() or die $ff->error;
print ("$file\n");
