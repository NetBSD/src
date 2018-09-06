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

my $target = shift;
my $file = shift;
my $mtime = time - (stat $file)[9];
die "bad mtime $mtime"
	unless ($mtime - $target >= 0 && $mtime - $target < 60);
