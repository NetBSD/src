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
use Getopt::Long;
use Net::DNS::Nameserver;

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };
sub term { };

$SIG{INT} = \&rmpid;
if ($Net::DNS::VERSION > 1.41) {
    $SIG{TERM} = \&term;
} else {
    $SIG{TERM} = \&rmpid;
}

my $localaddr = "10.53.0.3";

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $verbose = 0;
my $ttl = 60;
my $zone = "example.broken";
my $nsname = "ns3.$zone";
my $synth = "synth-then-dname.$zone";
my $synth2 = "synth2-then-dname.$zone";

sub reply_handler {
    my ($qname, $qclass, $qtype, $peerhost, $query, $conn) = @_;
    my ($rcode, @ans, @auth, @add);

    print ("request: $qname/$qtype\n");
    STDOUT->flush();

    if ($qname eq "example.broken") {
        if ($qtype eq "SOA") {
	    my $rr = new Net::DNS::RR("$qname $ttl $qclass SOA . . 0 0 0 0 0");
	    push @ans, $rr;
        } elsif ($qtype eq "NS") {
	    my $rr = new Net::DNS::RR("$qname $ttl $qclass NS $nsname");
	    push @ans, $rr;
	    $rr = new Net::DNS::RR("$nsname $ttl $qclass A $localaddr");
	    push @add, $rr;
        }
        $rcode = "NOERROR";
    } elsif ($qname eq "cname-to-$synth2") {
        my $rr = new Net::DNS::RR("$qname $ttl $qclass CNAME name.$synth2");
	push @ans, $rr;
        $rr = new Net::DNS::RR("name.$synth2 $ttl $qclass CNAME name");
	push @ans, $rr;
        $rr = new Net::DNS::RR("$synth2 $ttl $qclass DNAME .");
	push @ans, $rr;
	$rcode = "NOERROR";
    } elsif ($qname eq "$synth" || $qname eq "$synth2") {
	if ($qtype eq "DNAME") {
	    my $rr = new Net::DNS::RR("$qname $ttl $qclass DNAME .");
	    push @ans, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "name.$synth") {
	my $rr = new Net::DNS::RR("$qname $ttl $qclass CNAME name.");
	push @ans, $rr;
	$rr = new Net::DNS::RR("$synth $ttl $qclass DNAME .");
	push @ans, $rr;
	$rcode = "NOERROR";
    } elsif ($qname eq "name.$synth2") {
	my $rr = new Net::DNS::RR("$qname $ttl $qclass CNAME name.");
	push @ans, $rr;
	$rr = new Net::DNS::RR("$synth2 $ttl $qclass DNAME .");
	push @ans, $rr;
	$rcode = "NOERROR";
    # The following three code branches referring to the "example.dname"
    # zone are necessary for the resolver variant of the CVE-2021-25215
    # regression test to work.  A named instance cannot be used for
    # serving the DNAME records below as a version of BIND vulnerable to
    # CVE-2021-25215 would crash while answering the queries asked by
    # the tested resolver.
    } elsif ($qname eq "ns3.example.dname") {
	if ($qtype eq "A") {
		my $rr = new Net::DNS::RR("$qname $ttl $qclass A 10.53.0.3");
		push @ans, $rr;
	}
	if ($qtype eq "AAAA") {
		my $rr = new Net::DNS::RR("example.dname. $ttl $qclass SOA . . 0 0 0 0 $ttl");
		push @auth, $rr;
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "self.example.self.example.dname") {
	my $rr = new Net::DNS::RR("self.example.dname. $ttl $qclass DNAME dname.");
	push @ans, $rr;
	$rr = new Net::DNS::RR("$qname $ttl $qclass CNAME self.example.dname.");
	push @ans, $rr;
	$rcode = "NOERROR";
    } elsif ($qname eq "self.example.dname") {
	if ($qtype eq "DNAME") {
		my $rr = new Net::DNS::RR("$qname $ttl $qclass DNAME dname.");
		push @ans, $rr;
	}
	$rcode = "NOERROR";
    } else {
	$rcode = "REFUSED";
    }
    return ($rcode, \@ans, \@auth, \@add, { aa => 1 });
}

GetOptions(
    'port=i' => \$localport,
    'verbose!' => \$verbose,
);

my $ns = Net::DNS::Nameserver->new(
    LocalAddr => $localaddr,
    LocalPort => $localport,
    ReplyHandler => \&reply_handler,
    Verbose => $verbose,
);

if ($Net::DNS::VERSION >= 1.42) {
    $ns->start_server();
    select(undef, undef, undef, undef);
    $ns->stop_server();
    unlink "ans.pid";
} else {
    $ns->main_loop;
}
