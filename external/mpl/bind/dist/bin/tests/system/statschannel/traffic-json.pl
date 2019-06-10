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

# traffic-json.pl:
# Parses the JSON version of the RSSAC002 traffic stats into a
# normalized format.

use JSON;

my $file = $ARGV[0];
open(INPUT, "<$file");
my $text = do{local$/;<INPUT>};
close(INPUT);

my $ref = decode_json($text);

my $tcprcvd = $ref->{traffic}->{"dns-tcp-requests-sizes-received-ipv4"};
my $type = "tcp request-size ";
foreach $key (keys %{$tcprcvd}) {
    print $type . $key . ": ". $tcprcvd->{$key} ."\n";
}

my $tcpsent = $ref->{traffic}->{"dns-tcp-responses-sizes-sent-ipv4"};
my $type = "tcp response-size ";
foreach $key (keys %{$tcpsent}) {
    print $type . $key . ": ". $tcpsent->{$key} ."\n";
}

my $udprcvd = $ref->{traffic}->{"dns-udp-requests-sizes-received-ipv4"};
my $type = "udp request-size ";
foreach $key (keys %{$udprcvd}) {
    print $type . $key . ": ". $udprcvd->{$key} ."\n";
}

my $udpsent = $ref->{traffic}->{"dns-udp-responses-sizes-sent-ipv4"};
my $type = "udp response-size ";
foreach $key (keys %{$udpsent}) {
    print $type . $key . ": ". $udpsent->{$key} ."\n";
}
