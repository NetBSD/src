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

# If send_response is set, the server will respond, otherwise the query will
# be dropped.
my $send_response = 1;
# If slow_response is set, a lookup for the CNAME target (target.example) is
# delayed. Other lookups will not be delayed.
my $slow_response = 0;

my $localaddr = "10.53.0.2";

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $udpsock = IO::Socket::INET->new(LocalAddr => "$localaddr",
   LocalPort => $localport, Proto => "udp", Reuse => 1) or die "$!";

#
# Delegation
#
my $SOA = "example 300 IN SOA . . 0 0 0 0 300";
my $NS = "example 300 IN NS ns.example";
my $A = "ns.example 300 IN A $localaddr";
#
# Records to be TTL stretched
#
my $TXT = "data.example 2 IN TXT \"A text record with a 2 second ttl\"";
my $LONGTXT = "longttl.example 600 IN TXT \"A text record with a 600 second ttl\"";
my $CAA = "othertype.example 2 IN CAA 0 issue \"ca1.example.net\"";
my $negSOA = "example 2 IN SOA . . 0 0 0 0 300";
my $CNAME = "cname.example 7 IN CNAME target.example";
my $TARGET = "target.example 9 IN A $localaddr";
my $SHORTCNAME = "shortttl.cname.example 1 IN CNAME longttl.target.example";
my $LONGTARGET = "longttl.target.example 600 IN A $localaddr";

sub reply_handler {
    my ($qname, $qclass, $qtype) = @_;
    my ($rcode, @ans, @auth, @add);

    print ("request: $qname/$qtype\n");
    STDOUT->flush();

    # Control whether we send a response or not.
    # We always respond to control commands.
    if ($qname eq "enable" ) {
	if ($qtype eq "TXT") {
	    $send_response = 1;
            my $rr = new Net::DNS::RR("$qname 0 $qclass TXT \"$send_response\"");
            push @ans, $rr;
	}
	$rcode = "NOERROR";
        return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
    } elsif ($qname eq "disable" ) {
	if ($qtype eq "TXT") {
	    $send_response = 0;
            my $rr = new Net::DNS::RR("$qname 0 $qclass TXT \"$send_response\"");
            push @ans, $rr;
	}
	$rcode = "NOERROR";
        return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
    } elsif ($qname eq "slowdown" ) {
	if ($qtype eq "TXT") {
	    $send_response = 1;
	    $slow_response = 1;
            my $rr = new Net::DNS::RR("$qname 0 $qclass TXT \"$send_response\"");
            push @ans, $rr;
	}
	$rcode = "NOERROR";
        return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
    }

    # If we are not responding to queries we are done.
    return if (!$send_response);

    # Construct the response and send it.
    if ($qname eq "ns.example" ) {
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($A);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($SOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "example") {
	if ($qtype eq "NS") {
	    my $rr = new Net::DNS::RR($NS);
	    push @auth, $rr;
	    $rr = new Net::DNS::RR($A);
	    push @add, $rr;
	} elsif ($qtype eq "SOA") {
	    my $rr = new Net::DNS::RR($SOA);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($SOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "nodata.example") {
	my $rr = new Net::DNS::RR($negSOA);
	push @auth, $rr;
	$rcode = "NOERROR";
    } elsif ($qname eq "data.example") {
	if ($qtype eq "TXT") {
	    my $rr = new Net::DNS::RR($TXT);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "a-only.example") {
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR("a-only.example 2 IN A $localaddr");
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "cname.example") {
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($CNAME);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "target.example") {
	if ($slow_response) {
                print "  Sleeping 3 seconds\n";
		sleep(3);
	}
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($TARGET);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "shortttl.cname.example") {
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($SHORTCNAME);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "longttl.target.example") {
	if ($slow_response) {
                print "  Sleeping 3 seconds\n";
		sleep(3);
	}
	if ($qtype eq "A") {
	    my $rr = new Net::DNS::RR($LONGTARGET);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "longttl.example") {
	if ($qtype eq "TXT") {
	    my $rr = new Net::DNS::RR($LONGTXT);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "nxdomain.example") {
	my $rr = new Net::DNS::RR($negSOA);
	push @auth, $rr;
	$rcode = "NXDOMAIN";
    } elsif ($qname eq "othertype.example") {
	if ($qtype eq "CAA") {
	    my $rr = new Net::DNS::RR($CAA);
	    push @ans, $rr;
	} else {
	    my $rr = new Net::DNS::RR($negSOA);
	    push @auth, $rr;
	}
	$rcode = "NOERROR";
    } else {
        my $rr = new Net::DNS::RR($SOA);
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
