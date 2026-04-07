#!/usr/bin/env perl

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

use strict;
use warnings;

use IO::File;
use IO::Socket;
use Getopt::Long;
use Net::DNS;
use Time::HiRes qw(usleep nanosleep);

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

my $localaddr = "10.53.0.8";

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $udpsock = IO::Socket::INET->new(LocalAddr => "$localaddr",
   LocalPort => $localport, Proto => "udp", Reuse => 1) or die "$!";

#
# YWH records
#
my $ywhSOA = "target.stale 300 IN SOA . . 0 0 0 0 300";
my $ywhNS = "target.stale 300 IN NS ns.target.stale";
my $ywhA = "ns.target.stale 300 IN A $localaddr";
my $ywhWWW = "www.target.stale 2 IN A 10.0.0.1";

sub reply_handler {
    my ($qname, $qclass, $qtype) = @_;
    my ($rcode, @ans, @auth, @add);

    print ("request: $qname/$qtype\n");
    STDOUT->flush();

    # Control what response we send.
    if ($qname eq "update" ) {
        if ($qtype eq "TXT") {
            $ywhWWW = "www.target.stale 2 IN A 10.0.0.2";
            my $rr = new Net::DNS::RR("$qname 0 $qclass TXT \"update\"");
            push @ans, $rr;
        }
        $rcode = "NOERROR";
        return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
    } elsif ($qname eq "restore" ) {
        if ($qtype eq "TXT") {
            $ywhWWW = "www.target.stale 2 IN A 10.0.0.1";
            my $rr = new Net::DNS::RR("$qname 0 $qclass TXT \"restore\"");
            push @ans, $rr;
        }
        $rcode = "NOERROR";
        return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
    }

    if ($qname eq "target.stale") {
	if ($qtype eq "SOA") {
            my $rr = new Net::DNS::RR($ywhSOA);
            push @ans, $rr;
        } elsif ($qtype eq "NS") {
            my $rr = new Net::DNS::RR($ywhNS);
            push @ans, $rr;
	    $rr = new Net::DNS::RR($ywhA);
	    push @add, $rr;
        }
	$rcode = "NOERROR";
    } elsif ($qname eq "ns.target.stale") {
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($ywhA);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($ywhSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "www.target.stale") {
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($ywhWWW);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($ywhSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } else {
        my $rr = new Net::DNS::RR($ywhSOA);
	push @auth, $rr;
	$rcode = "NXDOMAIN";
    }

    # mark the answer as authoritative (by setting the 'aa' flag)
    return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
}

GetOptions(
    'port=i' => \$localport,
);

my $rin;
my $rout;

for (;;) {
	$rin = '';
	vec($rin, fileno($udpsock), 1) = 1;

	select($rout = $rin, undef, undef, undef);

	if (vec($rout, fileno($udpsock), 1)) {
		my ($buf, $request, $err);
		$udpsock->recv($buf, 512);

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
		my $qclass = $questions[0]->qclass;
		my $qtype = $questions[0]->qtype;
		my $id = $request->header->id;

		my ($rcode, $ans, $auth, $add, $headermask) = reply_handler($qname, $qclass, $qtype);

		if (!defined($rcode)) {
			print "  Silently ignoring query\n";
			next;
		}

		my $reply = Net::DNS::Packet->new();
		$reply->header->qr(1);
		$reply->header->aa(1) if $headermask->{'aa'};
		$reply->header->id($id);
		$reply->header->rcode($rcode);
		$reply->push("question",   @questions);
		$reply->push("answer",     @$ans)  if $ans;
		$reply->push("authority",  @$auth) if $auth;
		$reply->push("additional", @$add)  if $add;

		my $num_chars = $udpsock->send($reply->data);
		print "  Sent $num_chars bytes via UDP\n";
	}
}
