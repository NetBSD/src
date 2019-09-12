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

status=0
n=0

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"

echo_i "checking that unsigned TLD zone DNSKEY referenced by DLV validates as secure ($n)"
ret=0
$DIG $DIGOPTS child1.utld dnskey @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that unsigned TLD child zone DNSKEY referenced by DLV validates as secure ($n)"
ret=0
$DIG $DIGOPTS grand.child1.utld dnskey @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that no chain of trust SOA referenced by DLV validates as secure ($n)"
ret=0
$DIG $DIGOPTS child1.druz soa @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that no chain of trust child SOA referenced by DLV validates as secure ($n)"
ret=0
$DIG $DIGOPTS grand.child1.druz soa @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test that a child zone that is signed with an unsupported algorithm,
# referenced by a good DLV zone, yields an insecure response.
echo_i "checking that unsupported algorithm TXT referenced by DLV validates as insecure ($n)"
ret=0
$DIG $DIGOPTS foo.unsupported-algorithm.utld txt @10.53.0.3 > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS foo.unsupported-algorithm.utld txt @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
grep -q "foo\.unsupported-algorithm\.utld\..*TXT.*\"foo\"" dig.out.ns5.test$n || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test that a child zone that is signed with a disabled algorithm,
# referenced by a good DLV zone, yields an insecure response.
echo_i "checking that disabled algorithm TXT referenced by DLV validates as insecure ($n)"
ret=0
$DIG $DIGOPTS foo.disabled-algorithm.utld txt @10.53.0.3 > dig.out.ns3.test$n || ret=1
$DIG $DIGOPTS foo.disabled-algorithm.utld txt @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep "status: NOERROR" dig.out.ns5.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns5.test$n > /dev/null && ret=1
grep -q "foo\.disabled-algorithm\.utld\..*TXT.*\"foo\"" dig.out.ns5.test$n || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test that a child zone that is signed with a known algorithm, referenced by
# a DLV zone that is signed with a disabled algorithm, yields a bogus
# response.
echo_i "checking that good signed TXT referenced by disabled algorithm DLV validates as bogus ($n)"
ret=0
$DIG $DIGOPTS foo.child3.utld txt @10.53.0.8 > dig.out.ns8.test$n || ret=1
grep "status: SERVFAIL" dig.out.ns8.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns8.test$n > /dev/null && ret=1
grep -q "foo\.child3\.utld\..*TXT.*\"foo\"" dig.out.ns8.test$n && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test that a child zone that is signed with a known algorithm, referenced by
# a DLV zone that is signed with an unsupported algorithm, yields a bogus
# response.
echo_i "checking that good signed TXT referenced by unsupported algorithm DLV validates as bogus ($n)"
ret=0
$DIG $DIGOPTS foo.child5.utld txt @10.53.0.7 > dig.out.ns7.test$n || ret=1
grep "status: SERVFAIL" dig.out.ns7.test$n > /dev/null || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns7.test$n > /dev/null && ret=1
grep -q "foo\.child5\.utld\..*TXT.*\"foo\"" dig.out.ns7.test$n && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
