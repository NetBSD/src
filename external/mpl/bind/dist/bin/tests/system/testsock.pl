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

# Test whether the interfaces on 10.53.0.* are up.

require 5.001;

use Socket;
use Getopt::Long;

my $port = 0;
my $id = 0;
GetOptions("p=i" => \$port,
           "i=i" => \$id);

my @ids;
if ($id != 0) {
	@ids = ($id);
} else {
	@ids = (1..8);
}

foreach $id (@ids) {
        my $addr = pack("C4", 10, 53, 0, $id);
	my $sa = pack_sockaddr_in($port, $addr);
	socket(SOCK, PF_INET, SOCK_STREAM, getprotobyname("tcp"))
      		or die "$0: socket: $!\n";
	setsockopt(SOCK, SOL_SOCKET, SO_REUSEADDR, pack("l", 1));

	bind(SOCK, $sa)
	    	or die sprintf("$0: bind(%s, %d): $!\n",
			       inet_ntoa($addr), $port);
	close(SOCK);
}
