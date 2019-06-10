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

# This is a TCP-only DNS server whose aim is to facilitate testing how dig
# copes with prematurely closed TCP connections.
#
# This server can be configured (through a separate control socket) with a
# series of responses to send for subsequent incoming TCP DNS queries.  Only
# one query is handled before closing each connection.  In order to keep things
# simple, the server is not equipped with any mechanism for handling malformed
# queries.
#
# Available response types are defined in the %response_types hash in the
# getAnswerSection() function below.  Each RR returned is generated dynamically
# based on the QNAME found in the incoming query.

use IO::File;
use Net::DNS;
use Net::DNS::Packet;

use strict;

# Ignore SIGPIPE so we won't fail if peer closes a TCP socket early
local $SIG{PIPE} = 'IGNORE';

# Flush logged output after every line
local $| = 1;

my $server_addr = "10.53.0.5";
if (@ARGV > 0) {
	$server_addr = @ARGV[0];
}

my $mainport = int($ENV{'PORT'});
if (!$mainport) { $mainport = 5300; }
my $ctrlport = int($ENV{'EXTRAPORT1'});
if (!$ctrlport) { $ctrlport = 5301; }

my $ctlsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $ctrlport, Proto => "tcp", Listen => 5, Reuse => 1) or die "$!";

my $tcpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $mainport, Proto => "tcp", Listen => 5, Reuse => 1) or die "$!";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";;
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

my @response_sequence = ("complete_axfr");
my $connection_counter = 0;

# Return the next answer type to send, incrementing the connection counter and
# making sure the latter does not exceed the size of the array holding the
# configured response sequence.
sub getNextResponseType {
	my $response_type = $response_sequence[$connection_counter];

	$connection_counter++;
	$connection_counter %= scalar(@response_sequence);

	return $response_type;
}

# Return an array of resource records comprising the answer section of a given
# response type.
sub getAnswerSection {
	my ($response_type, $qname) = @_;

	my %response_types = (
		no_response => [],

		partial_axfr => [
			Net::DNS::RR->new("$qname 300 IN SOA . . 0 0 0 0 300"),
			Net::DNS::RR->new("$qname NS ."),
		],

		complete_axfr => [
			Net::DNS::RR->new("$qname 300 IN SOA . . 0 0 0 0 300"),
			Net::DNS::RR->new("$qname NS ."),
			Net::DNS::RR->new("$qname 300 IN SOA . . 0 0 0 0 300"),
		],
	);

	return $response_types{$response_type};
}


# Generate a Net::DNS::Packet containing the response to send on the current
# TCP connection.  If the answer section of the response is determined to be
# empty, no data will be sent on the connection at all (immediate EOF).
sub generateResponse {
	my ($buf) = @_;
	my $request;

	if ($Net::DNS::VERSION > 0.68) {
		$request = new Net::DNS::Packet(\$buf, 0);
		$@ and die $@;
	} else {
		my $err;
		($request, $err) = new Net::DNS::Packet(\$buf, 0);
		$err and die $err;
	}

	my @questions = $request->question;
	my $qname = $questions[0]->qname;
	my $qtype = $questions[0]->qtype;
	my $qclass = $questions[0]->qclass;
	my $id = $request->header->id;

	my $packet = new Net::DNS::Packet($qname, $qtype, $qclass);
	$packet->header->qr(1);
	$packet->header->aa(1);
	$packet->header->id($id);

	my $response_type = getNextResponseType();
	my $answers = getAnswerSection($response_type, $qname);
	for my $rr (@$answers) {
		$packet->push("answer", $rr);
	}

	print "    Sending \"$response_type\" response\n";

	return $packet->data if @$answers;
}

my $rin;
my $rout;
for (;;) {
	$rin = '';
	vec($rin, fileno($ctlsock), 1) = 1;
	vec($rin, fileno($tcpsock), 1) = 1;

	select($rout = $rin, undef, undef, undef);

	if (vec($rout, fileno($ctlsock), 1)) {
		my $conn = $ctlsock->accept;
		@response_sequence = split(' ', $conn->getline);
		$connection_counter = 0;
		print "Response sequence set to: @response_sequence\n";
		$conn->close;
	} elsif (vec($rout, fileno($tcpsock), 1)) {
		my $buf;
		my $lenbuf;
		my $conn = $tcpsock->accept;
		my $n = $conn->sysread($lenbuf, 2);
		die unless $n == 2;
		my $len = unpack("n", $lenbuf);
		$n = $conn->sysread($buf, $len);
		die unless $n == $len;
		print "TCP request\n";
		my $response = generateResponse($buf);
		if ($response) {
			$len = length($response);
			$n = $conn->syswrite(pack("n", $len), 2);
			$n = $conn->syswrite($response, $len);
			print "    Sent: $n chars via TCP\n";
		} else {
			print "    No response sent\n";
		}
		$conn->close;
	}
}
