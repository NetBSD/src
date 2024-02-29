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

use IO::File;
use IO::Socket;
use Data::Dumper;
use Net::DNS;
use Net::DNS::Packet;
use strict;

# Ignore SIGPIPE so we won't fail if peer closes a TCP socket early
local $SIG{PIPE} = 'IGNORE';

# Flush logged output after every line
local $| = 1;

my $server_addr = "10.53.0.8";

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $udpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $localport, Proto => "udp", Reuse => 1) or die "$!";
my $tcpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $localport, Proto => "tcp", Listen => 5, Reuse => 1) or die "$!";

print "listening on $server_addr:$localport.\n";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";;
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

sub handleUDP {
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

	my $response = new Net::DNS::Packet($qname, $qtype, $qclass);
	$response->header->qr(1);
	$response->header->aa(1);
	$response->header->tc(0);
	$response->header->id($id);

	# Responses to queries for no-questions/NS and ns.no-questions/A are
	# _not_ malformed or truncated.
	if ($qname eq "no-questions" && $qtype eq "NS") {
		$response->push("answer", new Net::DNS::RR($qname . " 300 NS ns.no-questions"));
		$response->push("additional", new Net::DNS::RR("ns.no-questions. 300 A 10.53.0.8"));
		return $response->data;
	} elsif ($qname eq "ns.no-questions") {
		$response->push("answer", new Net::DNS::RR($qname . " 300 A 10.53.0.8"))
			if ($qtype eq "A");
		return $response->data;
	} elsif ($qname =~ /\.formerr-to-all$/) {
		$response->header->rcode("FORMERR");
		return $response->data;
	}

	# don't use Net::DNS to construct the header only reply as early
	# versions just get it completely wrong.

	if ($qname eq "truncated.no-questions") {
		# QR, AA, TC: forces TCP retry
		return (pack("nnnnnn", $id, 0x8600, 0, 0, 0, 0));
	} elsif ($qname eq "tcpalso.no-questions") {
		# QR, REFUSED: forces TCP retry
		return (pack("nnnnnn", $id, 0x8205, 0, 0, 0, 0));
	}
	# QR, AA
	return (pack("nnnnnn", $id, 0x8400, 0, 0, 0, 0));
}

sub handleTCP {
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

	my @results = ();
	my $response = new Net::DNS::Packet($qname, $qtype, $qclass);

	$response->header->qr(1);
	$response->header->aa(1);
	$response->header->id($id);
	$response->push("answer", new Net::DNS::RR("$qname 300 A 1.2.3.4"));

	if ($qname eq "tcpalso.no-questions") {
		# for this qname we also return a bad reply over TCP
		# QR, REFUSED, no question section
		push (@results, pack("nnnnnn", $id, 0x8005, 0, 0, 0, 0));
	} else {
		push(@results, $response->data);
	}

	return \@results;
}

# Main
my $rin;
my $rout;
for (;;) {
	$rin = '';
	vec($rin, fileno($tcpsock), 1) = 1;
	vec($rin, fileno($udpsock), 1) = 1;

	select($rout = $rin, undef, undef, undef);

	if (vec($rout, fileno($udpsock), 1)) {
		printf "UDP request\n";
		my $buf;
		$udpsock->recv($buf, 512);
		my $result = handleUDP($buf);
		my $num_chars = $udpsock->send($result);
		print "  Sent $num_chars bytes via UDP\n";
	} elsif (vec($rout, fileno($tcpsock), 1)) {
		my $conn = $tcpsock->accept;
		my $buf;
		for (;;) {
			my $lenbuf;
			my $n = $conn->sysread($lenbuf, 2);
			last unless $n == 2;
			my $len = unpack("n", $lenbuf);
			$n = $conn->sysread($buf, $len);
			last unless $n == $len;
			print "TCP request\n";
			my $result = handleTCP($buf);
			foreach my $response (@$result) {
				$len = length($response);
				$n = $conn->syswrite(pack("n", $len), 2);
				$n = $conn->syswrite($response, $len);
				print "    Sent: $n chars via TCP\n";
			}
		}
		$conn->close;
	}
}
