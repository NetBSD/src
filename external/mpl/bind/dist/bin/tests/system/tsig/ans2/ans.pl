# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

#
# An adhoc server that returns a TC=1 response with the final byte
# removed to generate UNEXPECTEDEND form dns_message_parse.
#

use IO::File;
use IO::Socket;

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }
printf "localport %u\n", $localport;

my $sock = IO::Socket::INET->new(LocalAddr => "10.53.0.2",
   LocalPort => $localport, Proto => "udp") or die "$!";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

sub arraystring {
    my $string = join("", @_);
    return $string;
}

for (;;) {
	$from = $sock->recv($buf, 512);
	($port, $ip_address) = unpack_sockaddr_in($from);
	$l = length($buf);
	printf "received %u bytes from %s#%u\n", $l, inet_ntoa($ip_address), $port;
	@up = unpack("C[$l]", $buf);
	$up[2] |= 0x80; # QR
	$up[2] |= 0x02; # TC
	$up[3] |= 0x80; # RA
	$l -= 1;	# truncate the response 1 byte
	$replydata = pack("C[$l]", @up);
	printf "sent %u bytes\n", $sock->send($replydata);
}
