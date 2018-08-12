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

$target = shift;
while (<>) {
	$notbefore = $1 if m{^.* must not be signed before \d+ [(](\d+)[)]$};
	$inception = $1 if m{^.* inception time \d+ [(](\d+)[)]$};
}
die "missing notbefore time" unless $notbefore;
die "missing inception time" unless $inception;
my $delta = $inception - $notbefore;
die "bad inception time $delta"
	unless abs($delta - $target) < 3;
