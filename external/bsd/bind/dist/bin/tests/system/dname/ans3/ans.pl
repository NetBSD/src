#!/usr/bin/env perl
#
# Copyright (C) 2017  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

use IO::File;
use Getopt::Long;
use Net::DNS::Nameserver;

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

my $localaddr = "10.53.0.3";
my $localport = 5300;
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

$ns->main_loop;
