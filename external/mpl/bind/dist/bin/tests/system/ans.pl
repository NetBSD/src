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
# This is the name server from hell.  It provides canned
# responses based on pattern matching the queries, and
# can be reprogrammed on-the-fly over a TCP connection.
#
# The server listens for queries on port 5300 (or PORT).
#
# The server listens for control connections on port 5301 (or EXTRAPORT1).
#
# A control connection is a TCP stream of lines like
#
#  /pattern/
#  name ttl type rdata
#  name ttl type rdata
#  ...
#  /pattern/
#  name ttl type rdata
#  name ttl type rdata
#  ...
#
# There can be any number of patterns, each associated
# with any number of response RRs.  Each pattern is a
# Perl regular expression.  If an empty pattern ("//") is
# received, the server will ignore all incoming queries (TCP
# connections will still be accepted, but both UDP queries
# and TCP queries will not be responded to).  If a non-empty
# pattern is then received over the same control connection,
# default behavior is restored.
#
# Each incoming query is converted into a string of the form
# "qname qtype" (the printable query domain name, space,
# printable query type) and matched against each pattern.
#
# The first pattern matching the query is selected, and
# the RR following the pattern line are sent in the
# answer section of the response.
#
# Each new control connection causes the current set of
# patterns and responses to be cleared before adding new
# ones.
#
# The server handles UDP and TCP queries.  Zone transfer
# responses work, but must fit in a single 64 k message.
#
# Now you can add TSIG, just specify key/key data with:
#
#  /pattern <key> <key_data>/
#  name ttl type rdata
#  name ttl type rdata
#
#  Note that this data will still be sent with any request for
#  pattern, only this data will be signed. Currently, this is only
#  done for TCP.


use IO::File;
use IO::Socket;
use Data::Dumper;
use Net::DNS;
use Net::DNS::Packet;
use strict;

# Ignore SIGPIPE so we won't fail if peer closes a TCP socket early
local $SIG{PIPE} = 'IGNORE';

# Flush logged output after every line
local $| = 1;

# We default to listening on 10.53.0.2 for historical reasons
# XXX: we should also be able to specify IPv6
my $server_addr = "10.53.0.2";
if (@ARGV > 0) {
	$server_addr = @ARGV[0];
}

my $mainport = int($ENV{'PORT'});
if (!$mainport) { $mainport = 5300; }
my $ctrlport = int($ENV{'EXTRAPORT1'});
if (!$ctrlport) { $ctrlport = 5301; }

# XXX: we should also be able to set the port numbers to listen on.
my $ctlsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $ctrlport, Proto => "tcp", Listen => 5, Reuse => 1) or die "$!";

my $udpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $mainport, Proto => "udp", Reuse => 1) or die "$!";

my $tcpsock = IO::Socket::INET->new(LocalAddr => "$server_addr",
   LocalPort => $mainport, Proto => "tcp", Listen => 5, Reuse => 1) or die "$!";

print "listening on $server_addr:$mainport,$ctrlport.\n";
print "Using Net::DNS $Net::DNS::VERSION\n";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";;
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

#my @answers = ();
my @rules;
my $udphandler;
my $tcphandler;

sub handleUDP {
	my ($buf) = @_;
	my $request;

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
	my $qtype = $questions[0]->qtype;
	my $qclass = $questions[0]->qclass;
	my $id = $request->header->id;

	my $packet = new Net::DNS::Packet($qname, $qtype, $qclass);
	$packet->header->qr(1);
	$packet->header->aa(1);
	$packet->header->id($id);

	# get the existing signature if any, and clear the additional section
	my $prev_tsig;
	while (my $rr = $request->pop("additional")) {
		$prev_tsig = $rr if ($rr->type eq "TSIG");
	}

	my $r;
	foreach $r (@rules) {
		my $pattern = $r->{pattern};
		my($dbtype, $key_name, $key_data) = split(/ /,$pattern);
		print "[handleUDP] $dbtype, $key_name, $key_data \n";
		if ("$qname $qtype" =~ /$dbtype/) {
			my $a;
			foreach $a (@{$r->{answer}}) {
				$packet->push("answer", $a);
			}
			if(defined($key_name) && defined($key_data)) {
				my $tsig;
				# Sign the packet
				print "  Signing the response with " .
				      "$key_name/$key_data\n";

				if ($Net::DNS::VERSION < 0.69) {
					$tsig = Net::DNS::RR->new(
						   "$key_name TSIG $key_data");
				} else {
					$tsig = Net::DNS::RR->new(
							name => $key_name,
							type => 'TSIG',
							key  => $key_data);
				}

				# These kluges are necessary because Net::DNS
				# doesn't know how to sign responses.  We
				# clear compnames so that the TSIG key and
				# algorithm name won't be compressed, and
				# add one to arcount because the signing
				# function will attempt to decrement it,
				# which is incorrect in a response. Finally
				# we set request_mac to the previous digest.
				$packet->{"compnames"} = {}
					if ($Net::DNS::VERSION < 0.70);
				$packet->{"header"}{"arcount"} += 1
					if ($Net::DNS::VERSION < 0.70);
				if (defined($prev_tsig)) {
					if ($Net::DNS::VERSION < 0.73) {
						my $rmac = pack('n H*',
							length($prev_tsig->mac)/2,
							$prev_tsig->mac);
						$tsig->{"request_mac"} =
							unpack("H*", $rmac);
					} else {
						$tsig->request_mac(
							 $prev_tsig->mac);
					}
				}
				
				$packet->sign_tsig($tsig);
			}
			last;
		}
	}
	#$packet->print;

	return $packet->data;
}

# namelen:
# given a stream of data, reads a DNS-formatted name and returns its
# total length, thus making it possible to skip past it.
sub namelen {
	my ($data) = @_;
	my $len = 0;
	my $label_len = 0;
	do {
		$label_len = unpack("c", $data);
		$data = substr($data, $label_len + 1);
		$len += $label_len + 1;
	} while ($label_len != 0);
	return ($len);
}

# packetlen:
# given a stream of data, reads a DNS wire-format packet and returns
# its total length, making it possible to skip past it.
sub packetlen {
	my ($data) = @_;
	my $q;
	my $rr;
	my $header;
	my $offset;

	#
	# decode/encode were introduced in Net::DNS 0.68
	# parse is no longer a method and calling it here makes perl croak.
	#
	my $decode = 0;
	$decode = 1 if ($Net::DNS::VERSION >= 0.68);

	if ($decode) {
		($header, $offset) = Net::DNS::Header->decode(\$data);
	} else {
		($header, $offset) = Net::DNS::Header->parse(\$data);
	}
		
	for (1 .. $header->qdcount) {
		if ($decode) {
			($q, $offset) =
				 Net::DNS::Question->decode(\$data, $offset);
		} else {
			($q, $offset) =
				 Net::DNS::Question->parse(\$data, $offset);
		}
	}
	for (1 .. $header->ancount) {
		if ($decode) {
			($q, $offset) = Net::DNS::RR->decode(\$data, $offset);
		} else {
			($q, $offset) = Net::DNS::RR->parse(\$data, $offset);
		}
	}
	for (1 .. $header->nscount) {
		if ($decode) {
			($q, $offset) = Net::DNS::RR->decode(\$data, $offset);
		} else {
			($q, $offset) = Net::DNS::RR->parse(\$data, $offset);
		}
	}
	for (1 .. $header->arcount) {
		if ($decode) {
			($q, $offset) = Net::DNS::RR->decode(\$data, $offset);
		} else {
			($q, $offset) = Net::DNS::RR->parse(\$data, $offset);
		}
	}
	return $offset;
}

# sign_tcp_continuation:
# This is a hack to correct the problem that Net::DNS has no idea how
# to sign multiple-message TCP responses.  Several data that are included
# in the digest when signing a query or the first message of a response are
# omitted when signing subsequent messages in a TCP stream.
#
# Net::DNS::Packet->sign_tsig() has the ability to use a custom signing
# function (specified by calling Packet->sign_func()).  We use this
# function as the signing function for TCP continuations, and it removes
# the unwanted data from the digest before calling the default sign_hmac
# function.
sub sign_tcp_continuation {
	my ($key, $data) = @_;

	# copy out first two bytes: size of the previous MAC
	my $rmacsize = unpack("n", $data);
	$data = substr($data, 2);

	# copy out previous MAC
	my $rmac = substr($data, 0, $rmacsize);
	$data = substr($data, $rmacsize);

	# try parsing out the packet information
	my $plen = packetlen($data);
	my $pdata = substr($data, 0, $plen);
	$data = substr($data, $plen);

	# remove the keyname, ttl, class, and algorithm name
	$data = substr($data, namelen($data));
	$data = substr($data, 6);
	$data = substr($data, namelen($data));

	# preserve the TSIG data
	my $tdata = substr($data, 0, 8);

	# prepare a new digest and sign with it
	$data = pack("n", $rmacsize) . $rmac . $pdata . $tdata;
	return Net::DNS::RR::TSIG::sign_hmac($key, $data);
}

sub handleTCP {
	my ($buf) = @_;
	my $request;

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
	my $qtype = $questions[0]->qtype;
	my $qclass = $questions[0]->qclass;
	my $id = $request->header->id;

	my $opaque;

	my $packet = new Net::DNS::Packet($qname, $qtype, $qclass);
	$packet->header->qr(1);
	$packet->header->aa(1);
	$packet->header->id($id);

	# get the existing signature if any, and clear the additional section
	my $prev_tsig;
	my $signer;
	my $continuation = 0;
	if ($Net::DNS::VERSION < 0.81) {
		while (my $rr = $request->pop("additional")) {
			if ($rr->type eq "TSIG") {
				$prev_tsig = $rr;
			}
		}
	}

	my @results = ();
	my $count_these = 0;

	my $r;
	foreach $r (@rules) {
		my $pattern = $r->{pattern};
		my($dbtype, $key_name, $key_data) = split(/ /,$pattern);
		print "[handleTCP] $dbtype, $key_name, $key_data \n";
		if ("$qname $qtype" =~ /$dbtype/) {
			$count_these++;
			my $a;
			foreach $a (@{$r->{answer}}) {
				$packet->push("answer", $a);
			}
			if (defined($key_name) && defined($key_data)) {
				my $tsig;
				# sign the packet
				print "  Signing the data with " . 
				      "$key_name/$key_data\n";

				if ($Net::DNS::VERSION < 0.69) {
					$tsig = Net::DNS::RR->new(
						   "$key_name TSIG $key_data");
				} elsif ($Net::DNS::VERSION >= 0.81 &&
					 $continuation) {
				} elsif ($Net::DNS::VERSION >= 0.75 &&
					 $continuation) {
					$tsig = $prev_tsig;
				} else {
					$tsig = Net::DNS::RR->new(
							name => $key_name,
							type => 'TSIG',
							key  => $key_data);
				}

				# These kluges are necessary because Net::DNS
				# doesn't know how to sign responses.  We
				# clear compnames so that the TSIG key and
				# algorithm name won't be compressed, and
				# add one to arcount because the signing
				# function will attempt to decrement it,
				# which is incorrect in a response. Finally
				# we set request_mac to the previous digest.
				$packet->{"compnames"} = {}
					if ($Net::DNS::VERSION < 0.70);
				$packet->{"header"}{"arcount"} += 1
					if ($Net::DNS::VERSION < 0.70);
				if (defined($prev_tsig)) {
					if ($Net::DNS::VERSION < 0.73) {
						my $rmac = pack('n H*',
							length($prev_tsig->mac)/2,
							$prev_tsig->mac);
						$tsig->{"request_mac"} =
							unpack("H*", $rmac);
					} elsif ($Net::DNS::VERSION < 0.81) {
						$tsig->request_mac(
							 $prev_tsig->mac);
					}
				}
				
				$tsig->sign_func($signer) if defined($signer);
				$tsig->continuation($continuation) if
					 ($Net::DNS::VERSION >= 0.71 &&
					  $Net::DNS::VERSION <= 0.74 );
				if ($Net::DNS::VERSION < 0.81) {
					$packet->sign_tsig($tsig);
				} elsif ($continuation) {
					$opaque = $packet->sign_tsig($opaque);
				} else {
					$opaque = $packet->sign_tsig($request);
				}
				$signer = \&sign_tcp_continuation
					if ($Net::DNS::VERSION < 0.70);
				$continuation = 1;

				my $copy =
					Net::DNS::Packet->new(\($packet->data));
				$prev_tsig = $copy->pop("additional");
			}
			#$packet->print;
			push(@results,$packet->data);
			$packet = new Net::DNS::Packet($qname, $qtype, $qclass);
			$packet->header->qr(1);
			$packet->header->aa(1);
			$packet->header->id($id);
		}
	}
	print " A total of $count_these patterns matched\n";
	return \@results;
}

# Main
my $rin;
my $rout;
for (;;) {
	$rin = '';
	vec($rin, fileno($ctlsock), 1) = 1;
	vec($rin, fileno($tcpsock), 1) = 1;
	vec($rin, fileno($udpsock), 1) = 1;

	select($rout = $rin, undef, undef, undef);

	if (vec($rout, fileno($ctlsock), 1)) {
		warn "ctl conn";
		my $conn = $ctlsock->accept;
		my $rule = ();
		@rules = ();
		while (my $line = $conn->getline) {
			chomp $line;
			if ($line =~ m!^/(.*)/$!) {
				if (length($1) == 0) {
					$udphandler = sub { return; };
					$tcphandler = sub { return; };
				} else {
					$udphandler = \&handleUDP;
					$tcphandler = \&handleTCP;
					$rule = { pattern => $1, answer => [] };
					push(@rules, $rule);
				}
			} else {
				push(@{$rule->{answer}},
				     new Net::DNS::RR($line));
			}
		}
		$conn->close;
		#print Dumper(@rules);
		#print "+=+=+ $rules[0]->{'pattern'}\n";
		#print "+=+=+ $rules[0]->{'answer'}->[0]->{'rname'}\n";
		#print "+=+=+ $rules[0]->{'answer'}->[0]\n";
	} elsif (vec($rout, fileno($udpsock), 1)) {
		printf "UDP request\n";
		my $buf;
		$udpsock->recv($buf, 512);
		my $result = &$udphandler($buf);
		if (defined($result)) {
			my $num_chars = $udpsock->send($result);
			print "  Sent $num_chars bytes via UDP\n";
		}
	} elsif (vec($rout, fileno($tcpsock), 1)) {
		my $conn = $tcpsock->accept;
		my $buf;
		for (;;) {
			my $lenbuf;
			my $n = $conn->sysread($lenbuf, 2);
			last unless $n == 2;
			my $len = unpack("n", $lenbuf);
			$n = $conn->sysread($buf, $len);
			last unless $n == $len;
			print "TCP request\n";
			my $result = &$tcphandler($buf);
			if (defined($result)) {
				foreach my $response (@$result) {
					$len = length($response);
					$n = $conn->syswrite(pack("n", $len), 2);
					$n = $conn->syswrite($response, $len);
					print "    Sent: $n chars via TCP\n";
				}
			}
		}
		$conn->close;
	}
}
