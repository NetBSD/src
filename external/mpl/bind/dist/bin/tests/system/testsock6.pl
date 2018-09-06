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

require 5.001;

use IO::Socket::INET6;

foreach $addr (@ARGV) {
	my $sock;
	$sock = IO::Socket::INET6->new(LocalAddr => $addr,
                                       LocalPort => 0,
                                       Proto     => tcp)
                             or die "Can't bind : $@\n";
	close($sock);
}
