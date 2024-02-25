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

DIGOPTS="+tcp +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"

wait_for_serial() (
  $DIG $DIGOPTS "@$1" "$2" SOA >"$4"
  serial=$(awk '$4 == "SOA" { print $7 }' "$4")
  [ "$3" -eq "${serial:--1}" ]
)

status=0
n=0

n=$((n + 1))
echo_i "waiting for zone transfer to complete ($n)"
ret=0
for i in 1 2 3 4 5 6 7 8 9; do
  $DIG $DIGOPTS soa example. @10.53.0.2 >dig.ns2.test$n || true
  grep SOA dig.ns2.test$n >/dev/null && break
  sleep 1
done
for i in 1 2 3 4 5 6 7 8 9; do
  $DIG $DIGOPTS soa dynamic. @10.53.0.2 >dig.ns2.test$n || true
  grep SOA dig.ns2.test$n >/dev/null && break
  sleep 1
done

n=$((n + 1))
echo_i "testing case preserving responses - no acl ($n)"
ret=0
$DIG $DIGOPTS mx example. @10.53.0.1 >dig.ns1.test$n || ret=1
grep "0.mail.eXaMpLe" dig.ns1.test$n >/dev/null || ret=1
grep "mAiL.example" dig.ns1.test$n >/dev/null || ret=1
test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "testing no-case-compress acl '{ 10.53.0.2; }' ($n)"
ret=0

# check that we preserve zone case for non-matching query (10.53.0.1)
$DIG $DIGOPTS mx example. -b 10.53.0.1 @10.53.0.1 >dig.ns1.test$n || ret=1
grep "0.mail.eXaMpLe" dig.ns1.test$n >/dev/null || ret=1
grep "mAiL.example" dig.ns1.test$n >/dev/null || ret=1

# check that we don't preserve zone case for match (10.53.0.2)
$DIG $DIGOPTS mx example. -b 10.53.0.2 @10.53.0.2 >dig.ns2.test$n || ret=1
grep "0.mail.example" dig.ns2.test$n >/dev/null || ret=1
grep "mail.example" dig.ns2.test$n >/dev/null || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "testing load of dynamic zone with various \$ORIGIN values ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.1 >dig.ns1.test$n || ret=1
digcomp dig.ns1.test$n dynamic.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "transfer of dynamic zone with various \$ORIGIN values ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.2 >dig.ns2.test$n || ret=1
digcomp dig.ns2.test$n dynamic.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "change SOA owner case via update ($n)"
$NSUPDATE <<EOF
server 10.53.0.1 ${PORT}
zone dynamic
update add dYNAMIc 0 SOA mname1. . 2000042408 20 20 1814400 3600
send
EOF
$DIG $DIGOPTS axfr dynamic @10.53.0.1 >dig.ns1.test$n || ret=1
digcomp dig.ns1.test$n postupdate.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
ret=0
echo_i "wait for zone to transfer ($n)"
retry_quiet 20 wait_for_serial 10.53.0.2 dynamic 2000042408 dig.ns2.test$n || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check SOA owner case is transferred to secondary ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.2 >dig.ns2.test$n || ret=1
digcomp dig.ns2.test$n postupdate.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

#update delete Ns1.DyNaMIC. 300 IN A 10.53.0.1
n=$((n + 1))
echo_i "change A record owner case via update ($n)"
$NSUPDATE <<EOF
server 10.53.0.1 ${PORT}
zone dynamic
update add Ns1.DyNaMIC. 300 IN A 10.53.0.1
send
EOF
$DIG $DIGOPTS axfr dynamic @10.53.0.1 >dig.ns1.test$n || ret=1
digcomp dig.ns1.test$n postns1.good || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
ret=0
echo_i "wait for zone to transfer ($n)"
retry_quiet 20 wait_for_serial 10.53.0.2 dynamic 2000042409 dig.ns2.test$n || ret=1

test $ret -eq 0 || echo_i "failed"
status=$((status + ret))

n=$((n + 1))
echo_i "check A owner case is transferred to secondary ($n)"
ret=0
$DIG $DIGOPTS axfr dynamic @10.53.0.2 >dig.ns2.test$n || ret=1
digcomp dig.ns2.test$n postns1.good || ret=1
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
