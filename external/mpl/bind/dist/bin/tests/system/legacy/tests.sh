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

DIGOPTS="-p ${PORT} +tries=1 +time=2"

# Check whether the SOA record for the name provided in $1 can be resolved by
# ns1.  Return 0 if resolution succeeds as expected; return 1 otherwise.
resolution_succeeds() {
  _ret=0
  $DIG $DIGOPTS +tcp +tries=3 +time=5 @10.53.0.1 ${1} SOA >dig.out.test$n || _ret=1
  grep "status: NOERROR" dig.out.test$n >/dev/null || _ret=1
  return $_ret
}

# Check whether the SOA record for the name provided in $1 can be resolved by
# ns1.  Return 0 if resolution fails as expected; return 1 otherwise.  Note that
# both a SERVFAIL response and timing out mean resolution failed, so the exit
# code of dig does not influence the result (the exit code for a SERVFAIL
# response is 0 while the exit code for not getting a response at all is not 0).
resolution_fails() {
  _servfail=0
  _timeout=0
  $DIG $DIGOPTS +tcp +time=5 @10.53.0.1 ${1} TXT >dig.out.test$n || true
  grep -F "status: SERVFAIL" dig.out.test$n >/dev/null && _servfail=1
  grep -F "timed out" dig.out.test$n >/dev/null && _timeout=1
  if [ $_servfail -eq 1 ] || [ $_timeout -eq 1 ]; then
    return 0
  else
    return 1
  fi
}

status=0
n=0

n=$((n + 1))
echo_i "checking formerr edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.8 ednsformerr soa >dig.out.1.test$n || ret=1
grep "status: FORMERR" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null && ret=1
$DIG $DIGOPTS +noedns @10.53.0.8 ednsformerr soa >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to formerr edns server succeeds ($n)"
ret=0
resolution_succeeds ednsformerr. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking notimp edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.9 ednsnotimp soa >dig.out.1.test$n || ret=1
grep "status: NOTIMP" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null && ret=1
$DIG $DIGOPTS +noedns @10.53.0.9 ednsnotimp soa >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to notimp edns server fails ($n)"
ret=0
resolution_fails ednsnotimp. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking refused edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.10 ednsrefused soa >dig.out.1.test$n || ret=1
grep "status: REFUSED" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null && ret=1
$DIG $DIGOPTS +noedns @10.53.0.10 ednsrefused soa >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to refused edns server fails ($n)"
ret=0
resolution_fails ednsrefused. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking drop edns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.2 dropedns soa >dig.out.1.test$n && ret=1
grep "timed out" dig.out.1.test$n >/dev/null || ret=1
grep ";; no servers could be reached" dig.out.1.test$n >/dev/null || ret=1
$DIG $DIGOPTS +noedns @10.53.0.2 dropedns soa >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n >/dev/null && ret=1
$DIG $DIGOPTS +noedns +tcp @10.53.0.2 dropedns soa >dig.out.3.test$n || ret=1
grep "status: NOERROR" dig.out.3.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.3.test$n >/dev/null && ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.2 dropedns soa >dig.out.4.test$n && ret=1
grep "timed out" dig.out.4.test$n >/dev/null || ret=1
grep ";; no servers could be reached" dig.out.4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to drop edns server fails ($n)"
ret=0
resolution_fails dropedns. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking drop edns + no tcp server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.3 dropedns-notcp soa >dig.out.1.test$n && ret=1
grep "timed out" dig.out.1.test$n >/dev/null || ret=1
grep ";; no servers could be reached" dig.out.1.test$n >/dev/null || ret=1
$DIG $DIGOPTS +noedns +tcp @10.53.0.3 dropedns-notcp soa >dig.out.2.test$n && ret=1
grep "connection refused" dig.out.2.test$n >/dev/null || ret=1
$DIG $DIGOPTS +noedns @10.53.0.3 dropedns-notcp soa >dig.out.3.test$n || ret=1
grep "status: NOERROR" dig.out.3.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.3.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to drop edns + no tcp server fails ($n)"
ret=0
resolution_fails dropedns-notcp. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking plain dns server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.4 plain soa >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null && ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.4 plain soa >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to plain dns server succeeds ($n)"
ret=0
resolution_succeeds plain. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking plain dns + no tcp server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.5 plain-notcp soa >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null && ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.5 plain-notcp soa >dig.out.2.test$n && ret=1
grep "connection refused" dig.out.2.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to plain dns + no tcp server succeeds ($n)"
ret=0
resolution_succeeds plain-notcp. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))

echo_i "checking edns 512 server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.6 edns512 txt >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null || ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.6 edns512 txt >dig.out.2.test$n || ret=1
grep "status: NOERROR" dig.out.2.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.2.test$n >/dev/null || ret=1
$DIG $DIGOPTS +edns +dnssec @10.53.0.6 edns512 txt >dig.out.3.test$n && ret=1
grep "timed out" dig.out.3.test$n >/dev/null || ret=1
grep ";; no servers could be reached" dig.out.3.test$n >/dev/null || ret=1
$DIG $DIGOPTS +edns +dnssec +bufsize=512 +ignore @10.53.0.6 edns512 soa >dig.out.4.test$n || ret=1
grep "status: NOERROR" dig.out.4.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.4.test$n >/dev/null || ret=1
grep "flags:.* tc[ ;]" dig.out.4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to edns 512 server succeeds ($n)"
ret=0
retry_quiet 3 resolution_succeeds edns512. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking edns 512 + no tcp server setup ($n)"
ret=0
$DIG $DIGOPTS +edns @10.53.0.7 edns512-notcp soa >dig.out.1.test$n || ret=1
grep "status: NOERROR" dig.out.1.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.1.test$n >/dev/null || ret=1
$DIG $DIGOPTS +edns +tcp @10.53.0.7 edns512-notcp soa >dig.out.2.test$n && ret=1
grep "connection refused" dig.out.2.test$n >/dev/null || ret=1
$DIG $DIGOPTS +edns +dnssec @10.53.0.7 edns512-notcp soa >dig.out.3.test$n && ret=1
grep "timed out" dig.out.3.test$n >/dev/null || ret=1
grep ";; no servers could be reached" dig.out.3.test$n >/dev/null || ret=1
$DIG $DIGOPTS +edns +dnssec +bufsize=512 +ignore @10.53.0.7 edns512-notcp soa >dig.out.4.test$n || ret=1
grep "status: NOERROR" dig.out.4.test$n >/dev/null || ret=1
grep "EDNS: version:" dig.out.4.test$n >/dev/null || ret=1
grep "flags:.* tc[ ;]" dig.out.4.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to edns 512 + no tcp server fails ($n)"
ret=0
resolution_fails edns512-notcp. || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking recursive lookup to edns 512 + no tcp server does not cause query loops ($n)"
ret=0
sent=$(grep -c -F "sending packet to 10.53.0.7" ns1/named.run)
if [ $sent -ge 10 ]; then
  echo_i "ns1 sent $sent queries to ns7, expected less than 10"
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

stop_server --use-rndc --port ${CONTROLPORT} ns1
cp ns1/named2.conf ns1/named.conf
start_server --noclean --restart --port ${PORT} ns1

n=$((n + 1))
echo_i "checking recursive lookup to edns 512 + no tcp + trust anchor fails ($n)"
# retry loop in case the server restart above causes transient failure
for try in 0 1 2 3 4 5 6 7 8 9; do
  ret=0
  resolution_fails edns512-notcp. || ret=1
  [ "$ret" -eq 0 ] && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
