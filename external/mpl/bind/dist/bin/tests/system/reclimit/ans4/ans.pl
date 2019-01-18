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

use strict;
use warnings;

use IO::File;
use IO::Socket;
use Net::DNS;

my $localaddr = "10.53.0.4";
my $limit = getlimit();
my $no_more_waiting = 0;
my @delayed_response;
my $timeout;

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $udpsock = IO::Socket::INET->new(LocalAddr => "$localaddr",
   LocalPort => $localport, Proto => "udp", Reuse => 1) or die "$!";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

my $count = 0;
my $send_response = 1;

sub getlimit {
    if ( -e "ans.limit") {
	open(FH, "<", "ans.limit");
	my $line = <FH>;
	chomp $line;
	close FH;
	if ($line =~ /^\d+$/) {
	    return $line;
	}
    }

    return 0;
}

# If $wait == 0 is returned, returned reply will be sent immediately.
# If $wait == 1 is returned, sending the returned reply might be delayed; see
# comments inside handle_UDP() for details.
sub reply_handler {
    my ($qname, $qclass, $qtype) = @_;
    my ($rcode, @ans, @auth, @add, $wait);

    print ("request: $qname/$qtype\n");
    STDOUT->flush();

    $wait = 0;
    $count += 1;

    if ($qname eq "count" ) {
	if ($qtype eq "TXT") {
	    my ($ttl, $rdata) = (0, "$count");
	    my $rr = new Net::DNS::RR("$qname $ttl $qclass $qtype $rdata");
	    push @ans, $rr;
	    print ("\tcount: $count\n");
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "reset" ) {
	$count = 0;
	$send_response = 1;
	$limit = getlimit();
	$rcode = "NOERROR";
	print ("\tlimit: $limit\n");
    } elsif ($qname eq "direct.example.org" ) {
	if ($qtype eq "A") {
	    my ($ttl, $rdata) = (3600, $localaddr);
	    my $rr = new Net::DNS::RR("$qname $ttl $qclass $qtype $rdata");
	    push @ans, $rr;
	    print ("\twait=$wait ans: $qname $ttl $qclass $qtype $rdata\n");
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "indirect1.example.org" ||
	     $qname eq "indirect2.example.org" ||
	     $qname eq "indirect3.example.org" ||
	     $qname eq "indirect4.example.org" ||
	     $qname eq "indirect5.example.org" ||
	     $qname eq "indirect6.example.org" ||
	     $qname eq "indirect7.example.org" ||
	     $qname eq "indirect8.example.org") {
	if ($qtype eq "A") {
	    my ($ttl, $rdata) = (3600, $localaddr);
	    my $rr = new Net::DNS::RR("$qname $ttl $qclass $qtype $rdata");
	    push @ans, $rr;
	    print ("\twait=$wait ans: $qname $ttl $qclass $qtype $rdata\n");
	}
	$rcode = "NOERROR";
    } elsif ($qname =~ /^ns1\.(\d+)\.example\.org$/) {
	my $next = $1 + 1;
	$wait = 1;
	if ($limit == 0) {
	    my $rr = new Net::DNS::RR("$1.example.org 86400 $qclass NS ns1.$next.example.org");
	    push @auth, $rr;
	    print ("\twait=$wait auth: $1.example.org 86400 $qclass NS ns1.$next.example.org\n");
	} else {
	    $send_response = 1;
	    if ($qtype eq "A") {
		my ($ttl, $rdata) = (3600, $localaddr);
		my $rr = new Net::DNS::RR("$qname $ttl $qclass $qtype $rdata");
		print("\tresponse: $qname $ttl $qclass $qtype $rdata\n");
		push @ans, $rr;
	    }
	}
	$rcode = "NOERROR";
    } elsif ($qname eq "direct.example.net" ) {
        if ($qtype eq "A") {
            my ($ttl, $rdata) = (3600, $localaddr);
            my $rr = new Net::DNS::RR("$qname $ttl $qclass $qtype $rdata");
            push @ans, $rr;
	    print ("\twait=$wait ans: $qname $ttl $qclass $qtype $rdata\n");
        }
        $rcode = "NOERROR";
    } elsif( $qname =~ /^ns1\.(\d+)\.example\.net$/ ) {
        my $next = ($1 + 1) * 16;
        for (my $i = 1; $i < 16; $i++) {
            my $s = $next + $i;
            my $rr = new Net::DNS::RR("$1.example.net 86400 $qclass NS ns1.$s.example.net");
            push @auth, $rr;
	    print ("\twait=$wait auth: $1.example.net 86400 $qclass NS ns1.$s.example.net\n");
            $rr = new Net::DNS::RR("ns1.$s.example.net 86400 $qclass A 10.53.0.7");
	    print ("\twait=$wait add: ns1.$s.example.net 86400 $qclass A 10.53.0.7\n");
            push @add, $rr;
        }
        $rcode = "NOERROR";
    } else {
	$rcode = "NXDOMAIN";
	    print ("\twait=$wait NXDOMAIN\n");
    }

    return ($rcode, \@ans, \@auth, \@add, $wait);
}

sub handleUDP {
	my ($buf, $peer) = @_;
	my ($request, $rcode, $ans, $auth, $add, $wait);

	$request = new Net::DNS::Packet(\$buf, 0);
	$@ and die $@;

	my ($question) = $request->question;
	my $qname = $question->qname;
	my $qclass = $question->qclass;
	my $qtype = $question->qtype;

	($rcode, $ans, $auth, $add, $wait) = reply_handler($qname, $qclass, $qtype);

	my $reply = $request->reply();

	$reply->header->rcode($rcode);
	$reply->header->aa(@$ans ? 1 : 0);
	$reply->header->id($request->header->id);
	$reply->{answer} = $ans if $ans;
	$reply->{authority} = $auth if $auth;
	$reply->{additional} = $add if $add;

	if ($wait) {
		# reply_handler() asked us to delay sending this reply until
		# another reply with $wait == 1 is generated or a timeout
		# occurs.
		if (@delayed_response) {
			# A delayed reply is already queued, so we can now send
			# both the delayed reply and the current reply.
			send_delayed_response();
			return $reply;
		} elsif ($no_more_waiting) {
			# It was determined before that there is no point in
			# waiting for "accompanying" queries.  Thus, send the
			# current reply immediately.
			return $reply;
		} else {
			# No delayed reply is queued and the client is expected
			# to send an "accompanying" query shortly.  Do not send
			# the current reply right now, just save it for later
			# and wait for an "accompanying" query to be received.
			@delayed_response = ($reply, $peer);
			$timeout = 0.5;
			return;
		}
	} else {
		# Send reply immediately.
		return $reply;
	}
}

sub send_delayed_response {
	my ($reply, $peer) = @delayed_response;
	# Truncation to 512 bytes is required for triggering "NS explosion" on
	# builds without IPv6 support
	$udpsock->send($reply->data(512), 0, $peer);
	undef @delayed_response;
	undef $timeout;
	print ("send_delayed_response\n");
}

# Main
my $rin;
my $rout;
for (;;) {
	$rin = '';
	vec($rin, fileno($udpsock), 1) = 1;

	select($rout = $rin, undef, undef, $timeout);

	if (vec($rout, fileno($udpsock), 1)) {
		my ($buf, $peer, $reply);
		$udpsock->recv($buf, 512);
		$peer = $udpsock->peername();
		$reply = handleUDP($buf, $peer);
		# Truncation to 512 bytes is required for triggering "NS
		# explosion" on builds without IPv6 support
		$udpsock->send($reply->data(512), 0, $peer) if $reply;
	} else {
		# An "accompanying" query was expected to come in, but did not.
		# Assume the client never sends "accompanying" queries to
		# prevent pointlessly waiting for them ever again.
		$no_more_waiting = 1;
		# Send the delayed reply to the query which caused us to wait.
		send_delayed_response();
	}
}
