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

die "Usage: makenames.pl zonename num_records" if (@ARGV != 2);
my $zname = @ARGV[0];
my $nrecords = @ARGV[1];

my @chars = split("", "abcdefghijklmnopqrstuvwxyz");

print"\$TTL 300	; 5 minutes
\$ORIGIN $zname.
@			IN SOA	mname1. . (
				2011080201 ; serial
				20         ; refresh (20 seconds)
				20         ; retry (20 seconds)
				1814400    ; expire (3 weeks)
				600        ; minimum (1 hour)
				)
			NS	ns
ns			A	10.53.0.3\n";

srand; 
for (my $i = 0; $i < $nrecords; $i++) {
        my $name = "";
        for (my $j = 0; $j < 8; $j++) {
                my $r = rand 25;
                $name .= $chars[$r];
        }
        print "$name" . "\tIN\tA\t";
        my $x = int rand 254;
        my $y = int rand 254;
        my $z = int rand 254;
        print "10.$x.$y.$z\n";
}

