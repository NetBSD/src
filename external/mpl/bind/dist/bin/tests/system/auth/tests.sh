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

set -e

. ../conf.sh

DIGOPTS="+tcp -p ${PORT}"

status=0
n=0

n=$((n + 1))
echo_i "wait for zones to finish transferring to ns2 ($n)"
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  for zone in example.com example.net; do
    $DIG $DIGOPTS @10.53.0.2 soa $zone >dig.out.test$n || ret=1
    grep "ANSWER: 1," dig.out.test$n >/dev/null || ret=1
  done
  [ $ret -eq 0 ] && break
  sleep 1
done
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

#
# If recursion is unrequested or unavailable, then cross-zone CNAME records
# should not be followed. If both requested and available, they should be.
#
n=$((n + 1))
echo_i "check that cross-zone CNAME record does not return target data (rd=0/ra=0) ($n)"
ret=0
$DIG $DIGOPTS +norec @10.53.0.1 www.example.com >dig.out.test$n || ret=1
grep "ANSWER: 1," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa;" dig.out.test$n >/dev/null || ret=1
grep "www.example.com.*CNAME.*server.example.net" dig.out.test$n >/dev/null || ret=1
grep "server.example.net.*A.*10.53.0.100" dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that cross-zone CNAME record does not return target data (rd=1/ra=0) ($n)"
ret=0
$DIG $DIGOPTS +rec @10.53.0.1 www.example.com >dig.out.test$n || ret=1
grep "ANSWER: 1," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa rd;" dig.out.test$n >/dev/null || ret=1
grep "www.example.com.*CNAME.*server.example.net" dig.out.test$n >/dev/null || ret=1
grep "server.example.net.*A.*10.53.0.100" dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that cross-zone CNAME record does not return target data (rd=0/ra=1) ($n)"
ret=0
$DIG $DIGOPTS +norec @10.53.0.2 www.example.com >dig.out.test$n || ret=1
grep "ANSWER: 1," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa ra;" dig.out.test$n >/dev/null || ret=1
grep "www.example.com.*CNAME.*server.example.net" dig.out.test$n >/dev/null || ret=1
grep "server.example.net.*A.*10.53.0.100" dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that cross-zone CNAME records return target data (rd=1/ra=1) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 www.example.com >dig.out.test$n || ret=1
grep "ANSWER: 2," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa rd ra;" dig.out.test$n >/dev/null || ret=1
grep "www.example.com.*CNAME.*server.example.net" dig.out.test$n >/dev/null || ret=1
grep "server.example.net.*A.*10.53.0.100" dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

#
# In-zone CNAME records should always be followed regardless of RD and RA.
#
n=$((n + 1))
echo_i "check that in-zone CNAME records return target data (rd=0/ra=0) ($n)"
ret=0
$DIG $DIGOPTS +norec @10.53.0.1 inzone.example.com >dig.out.test$n || ret=1
grep "ANSWER: 2," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa;" dig.out.test$n >/dev/null || ret=1
grep "inzone.example.com.*CNAME.*a.example.com" dig.out.test$n >/dev/null || ret=1
grep "a.example.com.*A.*10.53.0.1" dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone CNAME records returns target data (rd=1/ra=0) ($n)"
ret=0
$DIG $DIGOPTS +rec @10.53.0.1 inzone.example.com >dig.out.test$n || ret=1
grep "ANSWER: 2," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa rd;" dig.out.test$n >/dev/null || ret=1
grep "inzone.example.com.*CNAME.*a.example.com" dig.out.test$n >/dev/null || ret=1
grep "a.example.com.*A.*10.53.0.1" dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone CNAME records return target data (rd=0/ra=1) ($n)"
ret=0
$DIG $DIGOPTS +norec @10.53.0.2 inzone.example.com >dig.out.test$n || ret=1
grep "ANSWER: 2," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa ra;" dig.out.test$n >/dev/null || ret=1
grep "inzone.example.com.*CNAME.*a.example.com" dig.out.test$n >/dev/null || ret=1
grep "a.example.com.*A.*10.53.0.1" dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone CNAME records return target data (rd=1/ra=1) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 inzone.example.com >dig.out.test$n || ret=1
grep "ANSWER: 2," dig.out.test$n >/dev/null || ret=1
grep "flags: qr aa rd ra;" dig.out.test$n >/dev/null || ret=1
grep "inzone.example.com.*CNAME.*a.example.com" dig.out.test$n >/dev/null || ret=1
grep "a.example.com.*A.*10.53.0.1" dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone CNAME records does not return target data when QTYPE is CNAME (rd=1/ra=1) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -t cname inzone.example.com >dig.out.test$n || ret=1
grep 'ANSWER: 1,' dig.out.test$n >/dev/null || ret=1
grep 'flags: qr aa rd ra;' dig.out.test$n >/dev/null || ret=1
grep 'inzone\.example\.com\..*CNAME.a\.example\.com\.' dig.out.test$n >/dev/null || ret=1
grep 'a\.example\.com\..*A.10\.53\.0\.1' dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone CNAME records does not return target data when QTYPE is ANY (rd=1/ra=1) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -t any inzone.example.com >dig.out.test$n || ret=1
grep 'ANSWER: 1,' dig.out.test$n >/dev/null || ret=1
grep 'flags: qr aa rd ra;' dig.out.test$n >/dev/null || ret=1
grep 'inzone\.example\.com\..*CNAME.a\.example\.com\.' dig.out.test$n >/dev/null || ret=1
grep 'a\.example\.com\..*A.10\.53\.0\.1' dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone DNAME records does not return target data when QTYPE is CNAME (rd=1/ra=1) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -t cname inzone.dname.example.com >dig.out.test$n || ret=1
grep 'ANSWER: 2,' dig.out.test$n >/dev/null || ret=1
grep 'flags: qr aa rd ra;' dig.out.test$n >/dev/null || ret=1
grep 'dname\.example\.com\..*DNAME.example\.com\.' dig.out.test$n >/dev/null || ret=1
grep 'inzone\.dname\.example\.com\..*CNAME.inzone\.example\.com\.' dig.out.test$n >/dev/null || ret=1
grep 'inzone\.example\.com\..*CNAME.a\.example\.com\.' dig.out.test$n >/dev/null && ret=1
grep 'a\.example\.com\..*A.10\.53\.0\.1' dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that in-zone DNAME records does not return target data when QTYPE is ANY (rd=1/ra=1) ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -t any inzone.dname.example.com >dig.out.test$n || ret=1
grep 'ANSWER: 2,' dig.out.test$n >/dev/null || ret=1
grep 'flags: qr aa rd ra;' dig.out.test$n >/dev/null || ret=1
grep 'dname\.example\.com\..*DNAME.example\.com\.' dig.out.test$n >/dev/null || ret=1
grep 'inzone\.dname\.example\.com\..*CNAME.inzone\.example\.com\.' dig.out.test$n >/dev/null || ret=1
grep 'inzone\.example\.com.*CNAME.a\.example\.com\.' dig.out.test$n >/dev/null && ret=1
grep 'a\.example\.com.*A.10\.53\.0\.1' dig.out.test$n >/dev/null && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check that CHAOS addresses are compared correctly ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 +noall +answer ch test.example.chaos >dig.out.test$n || ret=1
lines=$(wc -l <dig.out.test$n)
[ ${lines:-0} -eq 2 ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check delegation response to ANY query ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.1 foo.child.example.net any >dig.out.test$n || ret=1
grep "ANSWER: 0, AUTHORITY: 1, ADDITIONAL: 2" dig.out.test$n >/dev/null || ret=1
grep 'child\.example\.net\..300.IN.NS.ns\.child\.example\.net\.$' dig.out.test$n >/dev/null || ret=1
grep 'ns\.child\.example\.net\..300.IN.A.10\.53\.0\.1$' dig.out.test$n >/dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
