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

#
# Ad hoc name server
#

use IO::File;
use IO::Socket;
use Net::DNS;
use Net::DNS::Packet;

# Ignore SIGPIPE so we won't fail if peer closes a TCP socket early
local $SIG{PIPE} = 'IGNORE';

# Flush logged output after every line
local $| = 1;

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $server_addr = "10.53.0.3";

my $udpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $localport, Proto => "udp", Reuse => 1) or die "$!";
my $tcpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $localport, Proto => "tcp", Listen => 5, Reuse => 1) or die "$!";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

sub handleQuery {
	my $buf = shift;
	my $packet;

	if ($Net::DNS::VERSION > 0.68) {
		$packet = new Net::DNS::Packet(\$buf, 0);
		$@ and die $@;
	} else {
		my $err;
		($packet, $err) = new Net::DNS::Packet(\$buf, 0);
		$err and die $err;
	}

	print "REQUEST:\n";
	$packet->print;

	$packet->header->qr(1);
	$packet->header->aa(1);

	my @questions = $packet->question;
	my $qname = $questions[0]->qname;
	my $qtype = $questions[0]->qtype;

	if ($qname eq "example.net" && $qtype eq "NS") {
		$packet->push("answer", new Net::DNS::RR($qname . " 300 NS ns.example.net"));
		$packet->push("additional", new Net::DNS::RR("ns.example.net 300 A 10.53.0.3"));
	} elsif ($qname eq "ns.example.net") {
		$packet->push("answer", new Net::DNS::RR($qname . " 300 A 10.53.0.3"));
	} elsif ($qname eq "nodata.example.net") {
		# Do not add a SOA RRset.
	} elsif ($qname eq "nxdomain.example.net") {
		# Do not add a SOA RRset.
		$packet->header->rcode(NXDOMAIN);
	} elsif ($qname eq "www.example.net") {
		# Data for address/alias filtering.
		if ($qtype eq "A") {
			$packet->push("answer", new Net::DNS::RR($qname . " 300 A 192.0.2.1"));
		} elsif ($qtype eq "AAAA") {
			$packet->push("answer", new Net::DNS::RR($qname . " 300 AAAA 2001:db8:beef::1"));
		}
	} elsif ($qname eq "badcname.example.net") {
		$packet->push("answer",
			      new Net::DNS::RR($qname .
				       " 300 CNAME badcname.example.org"));
	} elsif (($qname eq "baddname.example.net" || $qname eq "gooddname.example.net") && $qtype eq "NS") {
		$packet->push("authority", new Net::DNS::RR("example.net IN SOA (1 2 3 4 5)"))
	} elsif ($qname eq "foo.baddname.example.net") {
		$packet->push("answer",
			      new Net::DNS::RR("baddname.example.net" .
				       " 300 DNAME baddname.example.org"));
	} elsif ($qname eq "foo.gooddname.example.net") {
		$packet->push("answer",
			      new Net::DNS::RR("gooddname.example.net" .
				       " 300 DNAME gooddname.example.org"));
	} elsif ($qname eq "goodcname.example.net") {
		$packet->push("answer",
			      new Net::DNS::RR($qname .
				       " 300 CNAME goodcname.example.org"));
	} elsif ($qname =~ /^nodata\.example\.net$/i) {
		$packet->header->aa(1);
	} elsif ($qname =~ /^nxdomain\.example\.net$/i) {
		$packet->header->aa(1);
		$packet->header->rcode(NXDOMAIN);
	} elsif ($qname eq "large-referral.example.net") {
		for (my $i = 1; $i < 1000; $i++) {
			$packet->push("authority", new Net::DNS::RR("large-referral.example.net 300 NS ns" . $i . ".fake.redirect.com"));
		}
		# No glue records
	} elsif ($qname eq "foo.bar.sub.tld1") {
		$packet->push("answer", new Net::DNS::RR("$qname 300 TXT baz"));
	} elsif ($qname eq "cname.sub.example.org") {
		$packet->push("answer",
			      new Net::DNS::RR($qname .
				       " 300 CNAME ok.sub.example.org"));
	} elsif ($qname eq "ok.sub.example.org") {
		$packet->push("answer",
			      new Net::DNS::RR($qname . " 300 A 192.0.2.1"));
	} elsif ($qname eq "www.dname.sub.example.org") {
		$packet->push("answer",
			      new Net::DNS::RR("dname.sub.example.org" .
				       " 300 DNAME ok.sub.example.org"));
	} elsif ($qname eq "www.ok.sub.example.org") {
		$packet->push("answer",
			      new Net::DNS::RR($qname . " 300 A 192.0.2.1"));
	} elsif ($qname eq "foo.glue-in-answer.example.org") {
		$packet->push("answer", new Net::DNS::RR($qname . " 300 A 192.0.2.1"));
	} elsif ($qname eq "ns.example.net") {
		$packet->push("answer",
			      new Net::DNS::RR($qname .
				       " 300 A 10.53.0.3"));
	} else {
		$packet->push("answer", new Net::DNS::RR("www.example.com 300 A 1.2.3.4"));
	}

	print "RESPONSE:\n";
	$packet->print;

	return $packet->data;
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
		my $result = handleQuery($buf);
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
			my $result = handleQuery($buf);
			$len = length($result);
			$conn->syswrite(pack("n", $len), 2);
			$n = $conn->syswrite($result, $len);
			print "    Sent: $n chars via TCP\n";
		}
		$conn->close;
	}
}
