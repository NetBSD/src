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

DIGOPTS="-p ${PORT}"

status=0

echo_i "1000 A records"
$DIG $DIGOPTS +tcp +norec 1000.example. @10.53.0.1 a > dig.out.1000 || status=1
# $DIG $DIGOPTS 1000.example. @10.53.0.1 a > knowngood.dig.out.1000
digcomp knowngood.dig.out.1000 dig.out.1000 || status=1

echo_i "2000 A records"
$DIG $DIGOPTS +tcp +norec 2000.example. @10.53.0.1 a > dig.out.2000 || status=1
# $DIG $DIGOPTS 2000.example. @10.53.0.1 a > knowngood.dig.out.2000
digcomp knowngood.dig.out.2000 dig.out.2000 || status=1

echo_i "3000 A records"
$DIG $DIGOPTS +tcp +norec 3000.example. @10.53.0.1 a > dig.out.3000 || status=1
# $DIG $DIGOPTS 3000.example. @10.53.0.1 a > knowngood.dig.out.3000
digcomp knowngood.dig.out.3000 dig.out.3000 || status=1

echo_i "4000 A records"
$DIG $DIGOPTS +tcp +norec 4000.example. @10.53.0.1 a > dig.out.4000 || status=1
# $DIG $DIGOPTS 4000.example. @10.53.0.1 a > knowngood.dig.out.4000
digcomp knowngood.dig.out.4000 dig.out.4000 || status=1

echo_i "exactly maximum rrset"
$DIG $DIGOPTS +tcp +norec +noedns a-maximum-rrset.example. @10.53.0.1 a > dig.out.a-maximum-rrset \
	|| status=1
# $DIG $DIGOPTS a-maximum-rrset.example. @10.53.0.1 a > knowngood.dig.out.a-maximum-rrset
digcomp knowngood.dig.out.a-maximum-rrset dig.out.a-maximum-rrset || status=1

echo_i "exceed maximum rrset (5000 A records)"
$DIG $DIGOPTS +tcp +norec +noadd 5000.example. @10.53.0.1 a > dig.out.exceed || status=1
# Look for truncation bit (tc).
grep 'flags: .*tc.*;' dig.out.exceed > /dev/null || {
    echo_i "TC bit was not set"
    status=1
}

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
