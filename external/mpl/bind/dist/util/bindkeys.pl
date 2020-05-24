#!/usr/bin/env perl
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
use warnings;

my $lines;
while (<>) {
    chomp;
    if (/\/\* .Id:.* \*\//) {
	next;
    }
    s/\"/\\\"/g;
    s/$/\\n\\/;
    $lines .= $_ . "\n";
}

my $mkey = "#define TRUST_ANCHORS \\\n\t\"\\\n" . $lines . "\"\n";

print "#ifndef BIND_KEYS_H\n";
print "#define BIND_KEYS_H 1\n";
print $mkey;
print "#endif /* BIND_KEYS_H */\n";
