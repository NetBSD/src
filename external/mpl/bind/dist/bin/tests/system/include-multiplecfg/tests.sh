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

# Test of include statement with glob expression.

set -e

. ../conf.sh

DIGOPTS="+tcp +nosea +nostat +nocmd +norec +noques +noadd +nostats -p ${PORT}"

status=0
n=0

# Test 1 - check if zone1 was loaded.
n=$((n + 1))
echo_i "checking glob include of zone1 config ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 zone1.com. a >dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n >/dev/null || ret=1
grep '^zone1.com.' dig.out.ns2.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Test 2 - check if zone2 was loaded.
n=$((n + 1))
echo_i "checking glob include of zone2 config ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 zone2.com. a >dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n >/dev/null || ret=1
grep '^zone2.com.' dig.out.ns2.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Test 3 - check if standard file path (no magic chars) works.
n=$((n + 1))
echo_i "checking include of standard file path config ($n)"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 mars.com. a >dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n >/dev/null || ret=1
grep '^mars.com.' dig.out.ns2.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Test 4: named-checkconf correctly parses glob includes.
n=$((n + 1))
echo_i "checking named-checkconf with glob include ($n)"
ret=0
(
  cd ns2
  $CHECKCONF named.conf
) || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
