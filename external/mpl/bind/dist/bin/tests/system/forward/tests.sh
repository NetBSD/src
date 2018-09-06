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

DIGOPTS="-p ${PORT}"

root=10.53.0.1
hidden=10.53.0.2
f1=10.53.0.3
f2=10.53.0.4

status=0

echo_i "checking that a forward zone overrides global forwarders"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example1. txt @$hidden > dig.out.hidden || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example1. txt @$f1 > dig.out.f1 || ret=1
digcomp dig.out.hidden dig.out.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a forward first zone no forwarders recurses"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example2. txt @$root > dig.out.root || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example2. txt @$f1 > dig.out.f1 || ret=1
digcomp dig.out.root dig.out.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a forward only zone no forwarders fails"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example2. txt @$root > dig.out.root || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example2. txt @$f1 > dig.out.f1 || ret=1
digcomp dig.out.root dig.out.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that global forwarders work"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example4. txt @$hidden > dig.out.hidden || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example4. txt @$f1 > dig.out.f1 || ret=1
digcomp dig.out.hidden dig.out.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a forward zone works"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example1. txt @$hidden > dig.out.hidden || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example1. txt @$f2 > dig.out.f2 || ret=1
digcomp dig.out.hidden dig.out.f2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that forwarding doesn't spontaneously happen"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example2. txt @$root > dig.out.root || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example2. txt @$f2 > dig.out.f2 || ret=1
digcomp dig.out.root dig.out.f2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a forward zone with no specified policy works"
ret=0
$DIG $DIGOPTS +noadd +noauth txt.example3. txt @$hidden > dig.out.hidden || ret=1
$DIG $DIGOPTS +noadd +noauth txt.example3. txt @$f2 > dig.out.f2 || ret=1
digcomp dig.out.hidden dig.out.f2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that a forward only doesn't recurse"
ret=0
$DIG $DIGOPTS txt.example5. txt @$f2 > dig.out.f2 || ret=1
grep "SERVFAIL" dig.out.f2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for negative caching of forwarder response"
# prime the cache, shutdown the forwarder then check that we can
# get the answer from the cache.  restart forwarder.
ret=0
$DIG $DIGOPTS nonexist. txt @10.53.0.5 > dig.out.f2 || ret=1
grep "status: NXDOMAIN" dig.out.f2 > /dev/null || ret=1
$PERL ../stop.pl . ns4 || ret=1
$DIG $DIGOPTS nonexist. txt @10.53.0.5 > dig.out.f2 || ret=1
grep "status: NXDOMAIN" dig.out.f2 > /dev/null || ret=1
$PERL ../start.pl --restart --noclean --port ${PORT} . ns4 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that forward only zone overrides empty zone"
ret=0
$DIG $DIGOPTS 1.0.10.in-addr.arpa TXT @10.53.0.4 > dig.out.f2
grep "status: NOERROR" dig.out.f2 > /dev/null || ret=1
$DIG $DIGOPTS 2.0.10.in-addr.arpa TXT @10.53.0.4 > dig.out.f2
grep "status: NXDOMAIN" dig.out.f2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that DS lookups for grafting forward zones are isolated"
ret=0
$DIG $DIGOPTS grafted A @10.53.0.4 > dig.out.q1
$DIG $DIGOPTS grafted DS @10.53.0.4 > dig.out.q2
$DIG $DIGOPTS grafted A @10.53.0.4 > dig.out.q3
$DIG $DIGOPTS grafted AAAA @10.53.0.4 > dig.out.q4
grep "status: NOERROR" dig.out.q1 > /dev/null || ret=1
grep "status: NXDOMAIN" dig.out.q2 > /dev/null || ret=1
grep "status: NOERROR" dig.out.q3 > /dev/null || ret=1
grep "status: NOERROR" dig.out.q4 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that rfc1918 inherited 'forward first;' zones are warned about"
ret=0
$CHECKCONF rfc1918-inherited.conf | grep "forward first;" >/dev/null || ret=1
$CHECKCONF rfc1918-notinherited.conf | grep "forward first;" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that ULA inherited 'forward first;' zones are warned about"
ret=0
$CHECKCONF ula-inherited.conf | grep "forward first;" >/dev/null || ret=1
$CHECKCONF ula-notinherited.conf | grep "forward first;" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
