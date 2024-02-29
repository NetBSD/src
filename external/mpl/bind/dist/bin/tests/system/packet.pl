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

# This is a tool for sending an arbitrary packet via UDP or TCP to an
# arbitrary address and port.  The packet is specified in a file or on
# the standard input, in the form of a series of bytes in hexadecimal.
# Whitespace is ignored, as is anything following a '#' symbol.
#
# For example, the following input would generate normal query for
# isc.org/NS/IN":
#
#     # QID:
#     0c d8
#     # header:
#     01 00 00 01 00 00 00 00 00 00
#     # qname isc.org:
#     03 69 73 63 03 6f 72 67 00
#     # qtype NS:
#     00 02
#     # qclass IN:
#     00 01
#
# Note that we do not wait for a response for the server.  This is simply
# a way of injecting arbitrary packets to test server resposnes.
#
# Usage: packet.pl [-a <address>] [-d] [-p <port>] [-t (udp|tcp)] [-r <repeats>] [filename]
#
# Options:
# -a <address>:  specify address (XXX: no IPv6 support yet)
# -p <port>:     specify port
# -t <protocol>: specify UDP or TCP
# -r <num>:      send packet <num> times
# -d:            dump response packets
#
# If not specified, address defaults to 127.0.0.1, port to 53, protocol
# to udp, and file to stdin.

require 5.006.001;

use strict;
use Getopt::Std;
use IO::File;
use IO::Socket;

sub usage {
    print ("Usage: packet.pl [-a address] [-d] [-p port] [-t (tcp|udp)] [-r <repeats>] [file]\n");
    exit 1;
}

my $sock;
my $proto;

sub dumppacket {
    use Net::DNS;
    use Net::DNS::Packet;

    my $rin;
    my $rout;
    $rin = '';
    vec($rin, fileno($sock), 1) = 1;
    select($rout = $rin, undef, undef, 1);
    if (vec($rout, fileno($sock), 1)) {
        my $buf;

        if ($proto eq "udp") {
            $sock->recv($buf, 512);
        } else {
            my $n = $sock->sysread($buf, 2);
            return unless $n == 2;
            my $len = unpack("n", $buf);
            $n = $sock->sysread($buf, $len);
            return unless $n == $len;
        }

        my $response;
        if ($Net::DNS::VERSION > 0.68) {
            $response = new Net::DNS::Packet(\$buf, 0);
            $@ and die $@;
        } else {
            my $err;
            ($response, $err) = new Net::DNS::Packet(\$buf, 0);
            $err and die $err;
        }
        $response->print;
    }
}

my %options={};
getopts("a:dp:t:r:", \%options);

my $addr = "127.0.0.1";
$addr = $options{a} if defined $options{a};

my $port = 53;
$port = $options{p} if defined $options{p};

$proto = "udp";
$proto = lc $options{t} if defined $options{t};
usage if ($proto !~ /^(udp|tcp)$/);

my $repeats = 1;
$repeats = $options{r} if defined $options{r};

my $file = "STDIN";
if (@ARGV >= 1) {
    my $filename = shift @ARGV;
    open FH, "<$filename" or die "$filename: $!";
    $file = "FH";
}

my $input = "";
while (defined(my $line = <$file>) ) {
    chomp $line;
    $line =~ s/#.*$//;
    $input .= $line;
}

$input =~ s/\s+//g;
my $data = pack("H*", $input);
my $len = length $data;

my $output = unpack("H*", $data);
print ("sending $repeats time(s): $output\n");

$sock = IO::Socket::INET->new(PeerAddr => $addr, PeerPort => $port,
				 Blocking => 0,
				 Proto => $proto,) or die "$!";

STDOUT->autoflush(1);

my $bytes = 0;
while ($repeats > 0) {
    if ($proto eq "udp") {
	$bytes += $sock->send($data);
    } else {
	$bytes += $sock->syswrite(pack("n", $len), 2);
	$bytes += $sock->syswrite($data, $len);
    }

    $repeats = $repeats - 1;

    if ($repeats % 1000 == 0) {
	print ".";
    }
}

$sock->shutdown(SHUT_WR);
if (defined $options{d}) {
    dumppacket;
}

$sock->close;
close $file;
print ("\nsent $bytes bytes to $addr:$port\n");
