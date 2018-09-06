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

#
# Ad hoc name server
#

use IO::File;
use IO::Socket;
use Net::DNS;
use Net::DNS::Packet;

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $sock = IO::Socket::INET->new(LocalAddr => "10.53.0.3",
   LocalPort => $localport, Proto => "udp") or die "$!";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

for (;;) {
	$sock->recv($buf, 512);

	print "**** request from " , $sock->peerhost, " port ", $sock->peerport, "\n";

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

	if ($qname eq "badcname.example.net") {
		$packet->push("answer",
			      new Net::DNS::RR($qname .
				       " 300 CNAME badcname.example.org"));
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
	} else {
		$packet->push("answer", new Net::DNS::RR("www.example.com 300 A 1.2.3.4"));
	}

	$sock->send($packet->data);

	print "RESPONSE:\n";
	$packet->print;
	print "\n";
}
