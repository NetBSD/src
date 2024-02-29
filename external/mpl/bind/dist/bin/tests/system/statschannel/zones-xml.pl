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

# zones-xml.pl:
# Parses the XML version of the dnssec sign stats for the
# "dnssec" zone in the default view into a normalized format.

use XML::Simple;

my $file = $ARGV[0];
my $zone = $ARGV[1];

my $ref = XMLin($file);

my $counters = $ref->{views}->{view}->{_default}->{zones}->{zone}->{$zone}->{counters};

foreach $group (@$counters) {

    my $type = $group->{type};

    if ($type eq "dnssec-sign" || $type eq "dnssec-refresh") {
        if (exists $group->{counter}->{name}) {
            print $type . " operations " . $group->{counter}->{name} . ": " . $group->{counter}->{content} . "\n";
	} else {
            foreach $key (keys %{$group->{counter}}) {
                print $type . " operations " . $key . ": ". $group->{counter}->{$key}->{content} ."\n";
            }
        }
    }
}
