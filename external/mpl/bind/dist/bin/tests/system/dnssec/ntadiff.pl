#!/usr/bin/perl -w
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
use Time::Piece;
use Time::Seconds;

exit 1 if (scalar(@ARGV) != 2);

my $actual = Time::Piece->strptime($ARGV[0], '%d-%b-%Y %H:%M:%S.000 %z');
my $expected = Time::Piece->strptime($ARGV[1], '%s') + ONE_WEEK;
my $diff = abs($actual - $expected);

print($diff . "\n");
