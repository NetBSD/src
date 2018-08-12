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

# server-json.pl:
# Parses the JSON version of the server stats into a normalized format.

use JSON;

open(INPUT, "<json.stats");
my $text = do{local$/;<INPUT>};
close(INPUT);

my $ref = decode_json($text);
foreach $key (keys %{$ref->{opcodes}}) {
    print "opcode " . $key . ": " . $ref->{opcodes}->{$key} . "\n";
}
foreach $key (keys %{$ref->{rcodes}}) {
    print "rcode " . $key . ": " . $ref->{rcodes}->{$key} . "\n";
}
foreach $key (keys %{$ref->{qtypes}}) {
    print "qtype " . $key . ": " . $ref->{qtypes}->{$key} . "\n";
}
foreach $key (keys %{$ref->{nsstats}}) {
    print "nsstat " . $key . ": " . $ref->{nsstats}->{$key} . "\n";
}
