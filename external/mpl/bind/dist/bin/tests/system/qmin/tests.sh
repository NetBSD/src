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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"
CLEANQL="rm -f ans*/query.log"
status=0
n=0

n=$((n+1))
echo_i "query for .good is not minimized when qname-minimization is off ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.5 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.good. @10.53.0.5 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.good. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
cat << __EOF | $DIFF ans2/query.log - > /dev/null || ret=1
ADDR icky.icky.icky.ptang.zoop.boing.good.
ADDR ns3.good.
ADDR ns3.good.
ADDR a.bit.longer.ns.name.good.
ADDR a.bit.longer.ns.name.good.
__EOF
echo "ADDR icky.icky.icky.ptang.zoop.boing.good." | $DIFF ans3/query.log - > /dev/null || ret=1
echo "ADDR icky.icky.icky.ptang.zoop.boing.good." | $DIFF ans4/query.log - > /dev/null || ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .bad is not minimized when qname-minimization is off ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.5 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.bad. @10.53.0.5 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.bad. 1 IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
cat << __EOF | $DIFF ans2/query.log - > /dev/null || ret=1
ADDR icky.icky.icky.ptang.zoop.boing.bad.
ADDR ns3.bad.
ADDR ns3.bad.
ADDR a.bit.longer.ns.name.bad.
ADDR a.bit.longer.ns.name.bad.
__EOF
echo "ADDR icky.icky.icky.ptang.zoop.boing.bad." | $DIFF ans3/query.log - > /dev/null || ret=1
echo "ADDR icky.icky.icky.ptang.zoop.boing.bad." | $DIFF ans4/query.log - > /dev/null || ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .slow is not minimized when qname-minimization is off ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.5 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.slow. @10.53.0.5 > dig.out.test$n
sleep 5
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.slow. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
cat << __EOF | $DIFF ans2/query.log - > /dev/null || ret=1
ADDR icky.icky.icky.ptang.zoop.boing.slow.
ADDR ns3.slow.
ADDR ns3.slow.
ADDR a.bit.longer.ns.name.slow.
ADDR a.bit.longer.ns.name.slow.
__EOF
echo "ADDR icky.icky.icky.ptang.zoop.boing.slow." | $DIFF ans3/query.log - > /dev/null || ret=1
echo "ADDR icky.icky.icky.ptang.zoop.boing.slow." | $DIFF ans4/query.log - > /dev/null || ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .ugly is not minimized when qname-minimization is off ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.5 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.ugly. @10.53.0.5 > dig.out.test$n
sleep 5
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.ugly. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
cat << __EOF | $DIFF ans2/query.log - > /dev/null || ret=1
ADDR icky.icky.icky.ptang.zoop.boing.ugly.
ADDR ns3.ugly.
ADDR ns3.ugly.
ADDR a.bit.longer.ns.name.ugly.
ADDR a.bit.longer.ns.name.ugly.
__EOF
echo "ADDR icky.icky.icky.ptang.zoop.boing.ugly." | $DIFF ans3/query.log - > /dev/null || ret=1
echo "ADDR icky.icky.icky.ptang.zoop.boing.ugly." | $DIFF ans4/query.log - > /dev/null || ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .good is properly minimized when qname-minimization is in strict mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.good. @10.53.0.6 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.good. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR a.bit.longer.ns.name.good.
ADDR a.bit.longer.ns.name.good.
ADDR ns2.good.
ADDR ns3.good.
ADDR ns3.good.
NS boing.good.
NS good.
NS zoop.boing.good.
__EOF
cat << __EOF | $DIFF ans3/query.log - > /dev/null || ret=1
NS zoop.boing.good.
NS ptang.zoop.boing.good.
NS icky.ptang.zoop.boing.good.
__EOF
cat << __EOF | $DIFF ans4/query.log - > /dev/null || ret=1
NS icky.ptang.zoop.boing.good.
NS icky.icky.ptang.zoop.boing.good.
ADDR icky.icky.icky.ptang.zoop.boing.good.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .good is properly minimized when qname-minimization is in relaxed mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.7 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.good. @10.53.0.7 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.good. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR _.boing.good.
ADDR _.zoop.boing.good.
ADDR a.bit.longer.ns.name.good.
ADDR a.bit.longer.ns.name.good.
ADDR ns2.good.
ADDR ns3.good.
ADDR ns3.good.
__EOF
cat << __EOF | $DIFF ans3/query.log - > /dev/null || ret=1
ADDR _.ptang.zoop.boing.good.
ADDR _.icky.ptang.zoop.boing.good.
__EOF
cat << __EOF | $DIFF ans4/query.log - > /dev/null || ret=1
ADDR _.icky.icky.ptang.zoop.boing.good.
ADDR icky.icky.icky.ptang.zoop.boing.good.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .bad fails when qname-minimization is in strict mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.bad. @10.53.0.6 > dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR ns2.bad.
NS bad.
NS boing.bad.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .bad succeeds when qname-minimization is in relaxed mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.7 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.bad. @10.53.0.7 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.bad. 1 IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR _.boing.bad.
ADDR _.zoop.boing.bad.
ADDR a.bit.longer.ns.name.bad.
ADDR a.bit.longer.ns.name.bad.
ADDR ns2.bad.
ADDR ns3.bad.
ADDR ns3.bad.
__EOF
cat << __EOF | $DIFF ans3/query.log - > /dev/null || ret=1
ADDR _.ptang.zoop.boing.bad.
ADDR _.icky.ptang.zoop.boing.bad.
__EOF
cat << __EOF | $DIFF ans4/query.log - > /dev/null || ret=1
ADDR _.icky.icky.ptang.zoop.boing.bad.
ADDR icky.icky.icky.ptang.zoop.boing.bad.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .ugly fails when qname-minimization is in strict mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.ugly. @10.53.0.6 > dig.out.test$n
grep "status: SERVFAIL" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR ns2.ugly.
NS boing.ugly.
NS boing.ugly.
NS ugly.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))
$RNDCCMD 10.53.0.6 flush

n=$((n+1))
echo_i "query for .ugly succeeds when qname-minimization is in relaxed mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.7 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.ugly. @10.53.0.7 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.ugly. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sleep 1

sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR _.boing.ugly.
ADDR _.boing.ugly.
ADDR a.bit.longer.ns.name.ugly.
ADDR a.bit.longer.ns.name.ugly.
ADDR icky.icky.icky.ptang.zoop.boing.ugly.
ADDR ns2.ugly.
ADDR ns3.ugly.
ADDR ns3.ugly.
__EOF
echo "ADDR icky.icky.icky.ptang.zoop.boing.ugly." | $DIFF ans3/query.log - > /dev/null || ret=1
echo "ADDR icky.icky.icky.ptang.zoop.boing.ugly." | $DIFF ans4/query.log - > /dev/null || ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))
$RNDCCMD 10.53.0.7 flush

n=$((n+1))
echo_i "information that minimization was unsuccessful for .ugly is logged ($n)"
ret=0
grep "success resolving 'icky.icky.icky.ptang.zoop.boing.ugly/A' after disabling qname minimization due to 'FORMERR'" ns7/named.run > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .slow is properly minimized when qname-minimization is on ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS icky.icky.icky.ptang.zoop.boing.slow. @10.53.0.6 > dig.out.test$n
sleep 5
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "icky.icky.icky.ptang.zoop.boing.slow. 1	IN A	192.0.2.1" dig.out.test$n > /dev/null || ret=1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR a.bit.longer.ns.name.slow.
ADDR a.bit.longer.ns.name.slow.
ADDR ns2.slow.
ADDR ns3.slow.
ADDR ns3.slow.
NS boing.slow.
NS slow.
NS zoop.boing.slow.
__EOF
cat << __EOF | $DIFF ans3/query.log - > /dev/null || ret=1
NS zoop.boing.slow.
NS ptang.zoop.boing.slow.
NS icky.ptang.zoop.boing.slow.
__EOF
cat << __EOF | $DIFF ans4/query.log - > /dev/null || ret=1
NS icky.ptang.zoop.boing.slow.
NS icky.icky.ptang.zoop.boing.slow.
ADDR icky.icky.icky.ptang.zoop.boing.slow.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .ip6.arpa succeeds and skips on proper boundaries when qname-minimization is on ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS -x 2001:4f8::1 @10.53.0.6 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.f.4.0.1.0.0.2.ip6.arpa. 1 IN PTR nee.com." dig.out.test$n > /dev/null || ret=1
sleep 1
grep -v ADDR ans2/query.log > ans2/query.log.trimmed
cat << __EOF | $DIFF ans2/query.log.trimmed - > /dev/null || ret=1
NS 1.0.0.2.ip6.arpa.
NS 8.f.4.0.1.0.0.2.ip6.arpa.
NS 0.0.0.0.8.f.4.0.1.0.0.2.ip6.arpa.
NS 0.0.0.0.0.0.8.f.4.0.1.0.0.2.ip6.arpa.
NS 0.0.0.0.0.0.0.0.8.f.4.0.1.0.0.2.ip6.arpa.
PTR 1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.8.f.4.0.1.0.0.2.ip6.arpa.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for multiple label name skips after 7th label ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS more.icky.icky.icky.ptang.zoop.boing.good. @10.53.0.6 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "more.icky.icky.icky.ptang.zoop.boing.good. 1 IN	A 192.0.2.2" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR a.bit.longer.ns.name.good.
ADDR a.bit.longer.ns.name.good.
ADDR ns2.good.
ADDR ns3.good.
ADDR ns3.good.
NS boing.good.
NS good.
NS zoop.boing.good.
__EOF
cat << __EOF | $DIFF ans3/query.log - > /dev/null || ret=1
NS zoop.boing.good.
NS ptang.zoop.boing.good.
NS icky.ptang.zoop.boing.good.
__EOF
# There's no NS icky.icky.icky.ptang.zoop.boing.good. query - we skipped it.
cat << __EOF | $DIFF ans4/query.log - > /dev/null || ret=1
NS icky.ptang.zoop.boing.good.
NS icky.icky.ptang.zoop.boing.good.
ADDR more.icky.icky.icky.ptang.zoop.boing.good.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "qname minimization is disabled when forwarding ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.7 flush
$DIG $DIGOPTS a.bit.longer.ns.name.fwd. @10.53.0.7 > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a.bit.longer.ns.name.fwd. 1	IN	A	10.53.0.4" dig.out.test$n >/dev/null || ret=1
sleep 1
cat << __EOF | $DIFF ans2/query.log - > /dev/null || ret=1
ADDR a.bit.longer.ns.name.fwd.
__EOF
for ans in ans2; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "qname minimization resolves unusual ip6.arpa. names ($n)"
ret=0
$CLEANQL
$DIG $DIGOPTS test1.test2.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.9.0.9.4.1.1.1.1.8.2.6.0.1.0.0.2.ip6.arpa. txt @10.53.0.7 > dig.out.test$n 2>&1
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
# Expected output in dig.out.test$n:
# ;; ANSWER SECTION:
# test1.test2.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.9.0.9.4.1.1.1.1.8.2.6.0.1.0.0.2.ip6.arpa. 1 IN TXT "long_ip6_name"
grep 'ip6\.arpa.*TXT.*long_ip6_name' dig.out.test$n > /dev/null || ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

# Below are test cases for GL #2665: The QNAME minimization (if enabled) should
# also occur on the second query, after the RRsets have expired from cache.
# BIND will still have the entries in cache, but marked stale. These stale
# entries should not prevent the resolver from minimizing the QNAME.
# We query for the test domain a.b.stale. in all cases (QNAME minimization off,
# strict mode, and relaxed mode) and expect it to behave the same the second
# time when we have a stale delegation structure in cache.
n=$((n+1))
echo_i "query for .stale is not minimized when qname-minimization is off ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.5 flush
$DIG $DIGOPTS @10.53.0.5 txt a.b.stale. > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a\.b\.stale\..*1.*IN.*TXT.*peekaboo" dig.out.test$n > /dev/null || ret=1
sleep 1
echo "TXT a.b.stale." | $DIFF ans2/query.log - > /dev/null || ret=1
echo "TXT a.b.stale." | $DIFF ans3/query.log - > /dev/null || ret=1
test -f  ans4/query.log && ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .stale is properly minimized when qname-minimization is in strict mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.6 flush
$DIG $DIGOPTS @10.53.0.6 txt a.b.stale. > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a\.b\.stale\..*1.*IN.*TXT.*hooray" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR ns.b.stale.
ADDR ns2.stale.
NS b.stale.
NS stale.
__EOF
test -f  ans3/query.log && ret=1
sort ans4/query.log > ans4/query.log.sorted
cat << __EOF | $DIFF ans4/query.log.sorted - > /dev/null || ret=1
ADDR ns.b.stale.
NS b.stale.
TXT a.b.stale.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .stale is properly minimized when qname-minimization is in relaxed mode ($n)"
ret=0
$CLEANQL
$RNDCCMD 10.53.0.7 flush
$DIG $DIGOPTS @10.53.0.7 txt a.b.stale. > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a\.b\.stale\..*1.*IN.*TXT.*hooray" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR _.b.stale.
ADDR ns.b.stale.
ADDR ns2.stale.
__EOF
test -f  ans3/query.log && ret=1
sort ans4/query.log > ans4/query.log.sorted
cat << __EOF | $DIFF ans4/query.log.sorted - > /dev/null || ret=1
ADDR ns.b.stale.
TXT a.b.stale.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "sleep 2, allow entries in cache to go stale"
sleep 2

n=$((n+1))
echo_i "query for .stale is not minimized when qname-minimization is off (stale cache) ($n)"
ret=0
$CLEANQL
$DIG $DIGOPTS @10.53.0.5 txt a.b.stale. > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a\.b\.stale\..*1.*IN.*TXT.*peekaboo" dig.out.test$n > /dev/null || ret=1
sleep 1
echo "TXT a.b.stale." | $DIFF ans2/query.log - > /dev/null || ret=1
echo "TXT a.b.stale." | $DIFF ans3/query.log - > /dev/null || ret=1
test -f  ans4/query.log && ret=1
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .stale is properly minimized when qname-minimization is in strict mode (stale cache) ($n)"
ret=0
$CLEANQL
$DIG $DIGOPTS @10.53.0.6 txt a.b.stale. > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a\.b\.stale\..*1.*IN.*TXT.*hooray" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
NS b.stale.
NS stale.
__EOF
test -f  ans3/query.log && ret=1
sort ans4/query.log > ans4/query.log.sorted
cat << __EOF | $DIFF ans4/query.log.sorted - > /dev/null || ret=1
NS b.stale.
TXT a.b.stale.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "query for .stale is properly minimized when qname-minimization is in relaxed mode (stale cache) ($n)"
ret=0
$CLEANQL
$DIG $DIGOPTS @10.53.0.7 txt a.b.stale. > dig.out.test$n
grep "status: NOERROR" dig.out.test$n > /dev/null || ret=1
grep "a\.b\.stale\..*1.*IN.*TXT.*hooray" dig.out.test$n > /dev/null || ret=1
sleep 1
sort ans2/query.log > ans2/query.log.sorted
cat << __EOF | $DIFF ans2/query.log.sorted - > /dev/null || ret=1
ADDR _.b.stale.
__EOF
test -f  ans3/query.log && ret=1
sort ans4/query.log > ans4/query.log.sorted
cat << __EOF | $DIFF ans4/query.log.sorted - > /dev/null || ret=1
TXT a.b.stale.
__EOF
for ans in ans2 ans3 ans4; do mv -f $ans/query.log query-$ans-$n.log 2>/dev/null || true; done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
