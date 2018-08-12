#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
echo_i "fetching using hmac-md5 (trunc)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-md5-80:md5-trunc:$md5" @10.53.0.1 soa > dig.out.md5.trunc || ret=1
grep -i "md5-trunc.*TSIG.*NOERROR" dig.out.md5.trunc > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
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
echo_i "fetching using hmac-md5-80 (BADTRUNC)"
ret=0
$DIG $DIGOPTS example.nil. -y "hmac-md5-80:md5:$md5" @10.53.0.1 soa > dig.out.md5-80 || ret=1
grep -i "md5.*TSIG.*BADTRUNC" dig.out.md5-80 > /dev/null || ret=1
if [ $ret -eq 1 ] ; then
	echo_i "failed"; status=1
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

echo "I:check that multiple dnssec-keygen calls don't emit dns_dnssec_findmatchingkeys warning"
ret=0
$KEYGEN -r $RANDFILE -a hmac-sha256 -b 128 -n host example.net > keygen.out1 2>&1 || ret=1
grep dns_dnssec_findmatchingkeys keygen.out1 > /dev/null && ret=1
$KEYGEN -r $RANDFILE -a hmac-sha256 -b 128 -n host example.net > keygen.out2 2>&1 || ret=1
grep dns_dnssec_findmatchingkeys keygen.out2 > /dev/null && ret=1
if [ $ret -eq 1 ] ; then
	echo "I: failed"; status=1
fi

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
