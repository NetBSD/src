#!/bin/sh

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

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"

#
# Shared secrets.
#
md5="97rnFx24Tfna4mHPfgnerA=="
sha1="FrSt77yPTFx6hTs4i2tKLB9LmE0="
sha224="hXfwwwiag2QGqblopofai9NuW28q/1rH4CaTnA=="
sha256="R16NojROxtxH/xbDl//ehDsHm5DjWTQ2YXV+hGC2iBY="
sha384="OaDdoAk2LAcLtYeUnsT7A9XHjsb6ZEma7OCvUpMraQIJX6HetGrlKmF7yglO1G2h"
sha512="jI/Pa4qRu96t76Pns5Z/Ndxbn3QCkwcxLOgt9vgvnJw5wqTRvNyk3FtD6yIMd1dWVlqZ+Y4fe6Uasc0ckctEmg=="

status=0

if $FEATURETEST --md5
then
	echo_i "fetching using hmac-md5 (old form)"
	ret=0
	$DIG $DIGOPTS example.nil. -y "md5:$md5" @10.53.0.1 soa > dig.out.md5.old || ret=1
	grep -i "md5.*TSIG.*NOERROR" dig.out.md5.old > /dev/null || ret=1
	if [ $ret -eq 1 ] ; then
		echo_i "failed"; status=1
	fi

	echo_i "fetching using hmac-md5 (new form)"
	ret=0
	$DIG $DIGOPTS example.nil. -y "hmac-md5:md5:$md5" @10.53.0.1 soa > dig.out.md5.new || ret=1
	grep -i "md5.*TSIG.*NOERROR" dig.out.md5.new > /dev/null || ret=1
	if [ $ret -eq 1 ] ; then
		echo_i "failed"; status=1
	fi
else
	echo_i "skipping using hmac-md5"
fi

echo_i "fetching using hmac-sha1"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha1:sha1:$sha1" @10.53.0.1 soa > dig.out.sha1 || ret=1
grep -i "sha1.*TSIG.*NOERROR" dig.out.sha1 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha224"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha224:sha224:$sha224" @10.53.0.1 soa > dig.out.sha224 || ret=1
grep -i "sha224.*TSIG.*NOERROR" dig.out.sha224 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha256"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha256:sha256:$sha256" @10.53.0.1 soa > dig.out.sha256 || ret=1
grep -i "sha256.*TSIG.*NOERROR" dig.out.sha256 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha384"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha384:sha384:$sha384" @10.53.0.1 soa > dig.out.sha384 || ret=1
grep -i "sha384.*TSIG.*NOERROR" dig.out.sha384 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha512"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha512:sha512:$sha512" @10.53.0.1 soa > dig.out.sha512 || ret=1
grep -i "sha512.*TSIG.*NOERROR" dig.out.sha512 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

#
#
#	Truncated TSIG
#
#
if $FEATURETEST --md5
then
	echo_i "fetching using hmac-md5 (trunc)"
	ret=0
	$DIG $DIGOPTS example.nil. -y "hmac-md5-80:md5-trunc:$md5" @10.53.0.1 soa > dig.out.md5.trunc || ret=1
	grep -i "md5-trunc.*TSIG.*NOERROR" dig.out.md5.trunc > /dev/null || ret=1
	if [ $ret -eq 1 ] ; then
		echo_i "failed"; status=1
	fi
else
	echo_i "skipping using hmac-md5 (trunc)"
fi

echo_i "fetching using hmac-sha1 (trunc)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha1-80:sha1-trunc:$sha1" @10.53.0.1 soa > dig.out.sha1.trunc || ret=1
grep -i "sha1.*TSIG.*NOERROR" dig.out.sha1.trunc > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha224 (trunc)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha224-112:sha224-trunc:$sha224" @10.53.0.1 soa > dig.out.sha224.trunc || ret=1
grep -i "sha224-trunc.*TSIG.*NOERROR" dig.out.sha224.trunc > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha256 (trunc)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha256-128:sha256-trunc:$sha256" @10.53.0.1 soa > dig.out.sha256.trunc || ret=1
grep -i "sha256-trunc.*TSIG.*NOERROR" dig.out.sha256.trunc > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha384 (trunc)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha384-192:sha384-trunc:$sha384" @10.53.0.1 soa > dig.out.sha384.trunc || ret=1
grep -i "sha384-trunc.*TSIG.*NOERROR" dig.out.sha384.trunc > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha512-256 (trunc)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha512-256:sha512-trunc:$sha512" @10.53.0.1 soa > dig.out.sha512.trunc || ret=1
grep -i "sha512-trunc.*TSIG.*NOERROR" dig.out.sha512.trunc > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi


#
#
#	Check for bad truncation.
#
#
if $FEATURETEST --md5
then
	echo_i "fetching using hmac-md5-80 (BADTRUNC)"
	ret=0
	$DIG $DIGOPTS example.nil. -y "hmac-md5-80:md5:$md5" @10.53.0.1 soa > dig.out.md5-80 || ret=1
	grep -i "md5.*TSIG.*BADTRUNC" dig.out.md5-80 > /dev/null || ret=1
	if [ $ret -eq 1 ] ; then
		echo_i "failed"; status=1
	fi
else
	echo_i "skipping using hmac-md5-80 (BADTRUNC)"
fi

echo_i "fetching using hmac-sha1-80 (BADTRUNC)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha1-80:sha1:$sha1" @10.53.0.1 soa > dig.out.sha1-80 || ret=1
grep -i "sha1.*TSIG.*BADTRUNC" dig.out.sha1-80 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha224-112 (BADTRUNC)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha224-112:sha224:$sha224" @10.53.0.1 soa > dig.out.sha224-112 || ret=1
grep -i "sha224.*TSIG.*BADTRUNC" dig.out.sha224-112 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha256-128 (BADTRUNC)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha256-128:sha256:$sha256" @10.53.0.1 soa > dig.out.sha256-128 || ret=1
grep -i "sha256.*TSIG.*BADTRUNC" dig.out.sha256-128 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha384-192 (BADTRUNC)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha384-192:sha384:$sha384" @10.53.0.1 soa > dig.out.sha384-192 || ret=1
grep -i "sha384.*TSIG.*BADTRUNC" dig.out.sha384-192 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "fetching using hmac-sha512-256 (BADTRUNC)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-sha512-256:sha512:$sha512" @10.53.0.1 soa > dig.out.sha512-256 || ret=1
grep -i "sha512.*TSIG.*BADTRUNC" dig.out.sha512-256 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "attempting fetch with bad tsig algorithm"
ret=0
$DIG $DIGOPTS example.nil. -y "badalgo:invalid:$sha512" @10.53.0.1 soa > dig.out.badalgo 2>&1 || ret=1
grep -i "Couldn't create key invalid: algorithm is unsupported" dig.out.badalgo > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "checking both OPT and TSIG records are returned when TC=1"
ret=0
$DIG -p ${PORT} +ignore +bufsize=512 large.example.nil -y "hmac-sha1:sha1:$sha1" @10.53.0.1 txt > dig.out.large 2>&1 || ret=1
grep "flags:.* tc[ ;]" dig.out.large > /dev/null || ret=1
grep "status: NOERROR" dig.out.large > /dev/null || ret=1
grep "EDNS:" dig.out.large > /dev/null || ret=1
grep -i "sha1.*TSIG.*NOERROR" dig.out.sha1 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
fi

echo_i "check that dnssec-keygen won't generate TSIG keys"
ret=0
$KEYGEN -a hmac-sha256 -b 128 -n host example.net > keygen.out3 2>&1 && ret=1
grep "unknown algorithm" keygen.out3 > /dev/null || ret=1

echo_i "check that a 'BADTIME' response with 'QR=0' is handled as a request"
ret=0
$PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t tcp < badtime > /dev/null || ret=1
$DIG -p ${PORT} @10.53.0.1 version.bind txt ch > dig.out.verify || ret=1
grep "status: NOERROR" dig.out.verify > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i "failed"; status=1
fi

if "$PERL" -e 'use Net::DNS; use Net::DNS::Packet;' > /dev/null 2>&1
then
  echo_i "check that TSIG in the wrong place returns FORMERR"
  ret=0
  $PERL ../packet.pl -a 10.53.0.1 -p ${PORT} -t udp -d < badlocation > packet.out
  grep "rcode  = FORMERR" packet.out > /dev/null || ret=1
  if [ $ret -eq 1 ] ; then
    echo_i "failed"; status=1
  fi
fi

echo_i "check that a malformed truncated response to a TSIG query is handled"
ret=0
$DIG -p $PORT @10.53.0.1 bad-tsig > dig.out.bad-tsig || ret=1
grep "status: SERVFAIL" dig.out.bad-tsig > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
    echo_i "failed"; status=1
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
