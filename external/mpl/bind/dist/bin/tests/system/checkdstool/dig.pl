#!/usr/bin/perl

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

my $arg;
my $ext;
my $file;

foreach $arg (@ARGV) {
    if ($arg =~ /^\+/) {
        next;
    }
    if ($arg =~ /^-t/) {
        next;
    }
    if ($arg =~ /^ds$/i) {
        $ext = "ds";
        next;
    }
    if ($arg =~ /^dnskey$/i) {
        $ext = "dnskey";
        next;
    }
    $file = $arg;
    next;
}

open F, $file . "." . $ext . ".db" || die $!;
while (<F>) {
    print;
}
close F;
