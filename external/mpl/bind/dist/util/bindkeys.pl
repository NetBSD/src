#!/usr/bin/env perl

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

use strict;
use warnings;

print "#pragma once\n";
print "#define TRUST_ANCHORS \"\\\n";

my $fn = shift or die "Usage: $0 FILENAME\n";
open(my $fh, '<', $fn) or die "cannot open file $ARGV[1]\n";
while (<$fh>) {
    chomp;
    s/\"/\\\"/g;
    print $_ . "\\n\\\n";
}
close($fh);
print "\"\n";
