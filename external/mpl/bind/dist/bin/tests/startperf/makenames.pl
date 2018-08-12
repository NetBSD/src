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

use strict;

die "Usage: makenames.pl <num> [<len>]" if (@ARGV == 0 || @ARGV > 2);
my $len = 10;
$len = @ARGV[1] if (@ARGV == 2);

my @chars = split("", "abcdefghijklmnopqrstuvwxyz123456789");

srand; 
for (my $i = 0; $i < @ARGV[0]; $i++) {
        my $name = "";
        for (my $j = 0; $j < $len; $j++) {
                my $r = rand 35;
                $name .= $chars[$r];
        }
        print "$name" . ".example\n";
}
