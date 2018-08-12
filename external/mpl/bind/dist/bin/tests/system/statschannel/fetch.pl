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
