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

# server-xml.pl:
# Parses the XML version of the server stats into a normalized format.

use XML::Simple;
use Data::Dumper;

my $ref = XMLin("xml.mem");
print Dumper($ref);
