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

wait_for_serial() (
  $DIG $DIGOPTS "@$1" "$2" SOA >"$4"
  serial=$(awk '$4 == "SOA" { print $7 }' "$4")
  [ "$3" -eq "${serial:--1}" ]
)

status=0
n=0

DIGOPTS="+tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../_common/rndc.conf -s"

switch_responses() {
  $DIG $DIGOPTS "@$1" "${2}.switch._control." TXT +time=5 +tries=1 +tcp >/dev/null 2>&1
}

# Set up initial_axfr handlers and trigger transfers
switch_responses 10.53.0.2 "initial_axfr"
switch_responses 10.53.0.4 "initial_axfr"
$RNDCCMD 10.53.0.1 refresh nil | sed 's/^/ns1 /' | cat_i
$RNDCCMD 10.53.0.3 refresh nil | sed 's/^/ns3 /' | cat_i

# Wait for initial AXFRs to complete
retry_quiet 10 wait_for_serial 10.53.0.1 nil. 1 dig.out.ns1.axfr || {
  echo_i "ns1 initial AXFR failed"
  exit 1
}
retry_quiet 10 wait_for_serial 10.53.0.3 nil. 1 dig.out.ns3.axfr || {
  echo_i "ns3 initial AXFR failed"
  exit 1
}

# Test 1: IXFR that re-adds an existing record -> DNS_R_UNCHANGED
n=$((n + 1))
echo_i "testing IXFR with unchanged rdataset ($n)"
ret=0

switch_responses 10.53.0.2 "unchanged_ixfr"
sleep 1

$RNDCCMD 10.53.0.1 refresh nil | sed 's/^/ns1 /' | cat_i
sleep 2

$DIG $DIGOPTS @10.53.0.1 nil. TXT | grep 'unchanged ixfr test' >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.1 a.nil. A | grep '10.0.0.61' >/dev/null || ret=1
grep "dns_diff_apply: update with no effect" ns1/named.run >/dev/null || ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Test 2: IXFR that deletes last record in rdataset -> DNS_R_NXRRSET
n=$((n + 1))
echo_i "testing IXFR with nxrrset ($n)"
ret=0

switch_responses 10.53.0.4 "nxrrset_ixfr"
sleep 1

$RNDCCMD 10.53.0.3 refresh nil | sed 's/^/ns3 /' | cat_i
sleep 2

$DIG $DIGOPTS @10.53.0.3 nil. TXT | grep 'nxrrset ixfr test' >/dev/null || ret=1
$DIG $DIGOPTS @10.53.0.3 b.nil. A | grep '10.0.0.62' >/dev/null && ret=1

if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
