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

# This is a tool for sending queries via UDP to specified address and
# port, then exiting without waiting for a response.
#
# Usage: ditch.pl [-s <address>] [-p <port>] [filename]
#
# Input (in filename, if specified, otherwise stdin) is a series of one
# or more DNS names and types to send as queries, e.g.:
#
# www.example.com A
# www.example.org MX
#
# If not specified, address defaults to 127.0.0.1, port to 53.

require 5.006.001;

use strict;
use Getopt::Std;
use Net::DNS;
use Net::DNS::Packet;
use IO::File;
use IO::Socket;

sub usage {
    print ("Usage: ditch.pl [-s address] [-p port] [file]\n");
    exit 1;
}

my %options={};
getopts("s:p:t:", \%options);

my $addr = "127.0.0.1";
$addr = $options{s} if defined $options{s};

my $port = 53;
$port = $options{p} if defined $options{p};

my $file = "STDIN";
if (@ARGV >= 1) {
    my $filename = shift @ARGV;
    open FH, "<$filename" or die "$filename: $!";
    $file = "FH";
}

my $input = "";
while (defined(my $line = <$file>) ) {
    chomp $line;
    next if ($line =~ m/^ *#/);
    my @tokens = split (' ', $line);

    my $packet;
    if ($Net::DNS::VERSION > 0.68) {
            $packet = new Net::DNS::Packet();
            $@ and die $@;
    } else {
            my $err;
            ($packet, $err) = new Net::DNS::Packet();
            $err and die $err;
    }

    my $q = new Net::DNS::Question($tokens[0], $tokens[1], "IN");
    $packet->header->rd(1);
    $packet->push(question => $q);

    my $sock = IO::Socket::INET->new(PeerAddr => $addr, PeerPort => $port,
                                     Proto => "udp",) or die "$!";

    my $bytes = $sock->send($packet->data);
    #print ("sent $bytes bytes to $addr:$port:\n");
    #print ("  ", unpack("H* ", $packet->data), "\n");

    $sock->close;
}

close $file;
