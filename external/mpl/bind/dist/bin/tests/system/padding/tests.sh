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

n=0
status=0

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -c ../_common/rndc.conf -p ${CONTROLPORT} -s"

getcookie() {
  awk '$2 == "COOKIE:" {
		print $3;
	}' <$1
}

echo_i "checking that dig handles padding ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +qr +padding=128 foo.example @10.53.0.2 >dig.out.test$n
grep "; PAD" dig.out.test$n >/dev/null || ret=1
grep "; QUERY SIZE: 128" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that dig added padding ($n)"
ret=0
n=$((n + 1))
nextpart ns2/named.stats >/dev/null
$RNDCCMD 10.53.0.2 stats
wait_for_log_peek 5 "--- Statistics Dump ---" ns2/named.stats || ret=1
nextpart ns2/named.stats | grep "EDNS padding option received" >/dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that padding is added for TCP responses ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +vc +padding=128 foo.example @10.53.0.2 >dig.out.test$n
grep "; PAD" dig.out.test$n >/dev/null || ret=1
grep "rcvd: 128" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that padding is added to valid cookie responses ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +cookie foo.example @10.53.0.2 >dig.out.testc
cookie=$(getcookie dig.out.testc)
$DIG $DIGOPTS +cookie=$cookie +padding=128 foo.example @10.53.0.2 >dig.out.test$n
grep "; PAD" dig.out.test$n >/dev/null || ret=1
grep "rcvd: 128" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that padding must be requested (TCP) ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +vc foo.example @10.53.0.2 >dig.out.test$n
grep "; PAD" dig.out.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that padding must be requested (valid cookie) ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +cookie=$cookie foo.example @10.53.0.2 >dig.out.test$n
grep "; PAD" dig.out.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that padding can be filtered out ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +vc +padding=128 -b 10.53.0.8 foo.example @10.53.0.2 >dig.out.test$n
grep "; PAD" dig.out.test$n >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that a TCP and padding server config enables padding ($n)"
ret=0
n=$((n + 1))
nextpart ns2/named.stats >/dev/null
$RNDCCMD 10.53.0.2 stats
wait_for_log_peek 5 "--- Statistics Dump ---" ns2/named.stats || ret=1
opad=$(nextpart ns2/named.stats | awk '/EDNS padding option received/ { print $1}')
$DIG $DIGOPTS foo.example @10.53.0.3 >dig.out.test$n
$RNDCCMD 10.53.0.2 stats
wait_for_log_peek 5 "--- Statistics Dump ---" ns2/named.stats || ret=1
npad=$(nextpart ns2/named.stats | awk '/EDNS padding option received/ { print $1}')
if [ "$opad" -eq "$npad" ]; then
  echo_i "error: opad ($opad) == npad ($npad)"
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that a padding server config should enforce TCP ($n)"
ret=0
n=$((n + 1))
nextpart ns2/named.stats >/dev/null
$RNDCCMD 10.53.0.2 stats
wait_for_log_peek 5 "--- Statistics Dump ---" ns2/named.stats || ret=1
opad=$(nextpart ns2/named.stats | awk '/EDNS padding option received/ { print $1}')
$DIG $DIGOPTS foo.example @10.53.0.4 >dig.out.test$n
$RNDCCMD 10.53.0.2 stats
wait_for_log_peek 5 "--- Statistics Dump ---" ns2/named.stats || ret=1
npad=$(nextpart ns2/named.stats | awk '/EDNS padding option received/ { print $1}')
if [ "$opad" -ne "$npad" ]; then
  echo_i "error: opad ($opad) != npad ($npad)"
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "checking that zero-length padding option has no effect ($n)"
ret=0
n=$((n + 1))
$DIG $DIGOPTS +qr +ednsopt=12 foo.example @10.53.0.2 >dig.out.test$n.1
grep "; PAD" dig.out.test$n.1 >/dev/null || ret=1
$DIG $DIGOPTS +qr +ednsopt=12:00 foo.example @10.53.0.2 >dig.out.test$n.2
grep "; PAD" dig.out.test$n.2 >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
