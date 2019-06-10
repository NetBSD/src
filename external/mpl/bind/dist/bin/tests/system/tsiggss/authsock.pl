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

# test the update-policy external protocol

require 5.6.0;

use IO::Socket::UNIX;
use Getopt::Long;

my $path;
my $typeallowed = "A";
my $pidfile = "authsock.pid";
my $timeout = 0;

GetOptions("path=s" => \$path,
	   "type=s" => \$typeallowed,
	   "pidfile=s" => \$pidfile,
	   "timeout=i" => \$timeout);

if (!defined($path)) {
	print("Usage: authsock.pl --path=<sockpath> --type=type --pidfile=pidfile\n");
	exit(1);
}

unlink($path);
my $server = IO::Socket::UNIX->new(Local => $path, Type => SOCK_STREAM, Listen => 8) or
    die "unable to create socket $path";
chmod 0777, $path;

# setup our pidfile
open(my $pid,">",$pidfile)
    or die "unable to open pidfile $pidfile";
print $pid "$$\n";
close($pid);

if ($timeout != 0) {
    # die after the given timeout
    alarm($timeout);
}

while (my $client = $server->accept()) {
	$client->recv(my $buf, 8, 0);
	my ($version, $req_len) = unpack('N N', $buf);

	if ($version != 1 || $req_len < 17) {
		printf("Badly formatted request\n");
		$client->send(pack('N', 2));
		next;
	}

	$client->recv(my $buf, $req_len - 8, 0);

	my ($signer,
	    $name,
	    $addr,
	    $type,
	    $key,
	    $key_data) = unpack('Z* Z* Z* Z* Z* N/a', $buf);

	if ($req_len != length($buf)+8) {
		printf("Length mismatch %u %u\n", $req_len, length($buf)+8);
		$client->send(pack('N', 2));
		next;
	}

	printf("version=%u signer=%s name=%s addr=%s type=%s key=%s key_data_len=%u\n",
	       $version, $signer, $name, $addr, $type, $key, length($key_data));

	my $result;
	if ($typeallowed eq $type) {
		$result = 1;
		printf("allowed type %s == %s\n", $type, $typeallowed);
	} else {
		printf("disallowed type %s != %s\n", $type, $typeallowed);
		$result = 0;
	}

	$reply = pack('N', $result);
	$client->send($reply);
}
