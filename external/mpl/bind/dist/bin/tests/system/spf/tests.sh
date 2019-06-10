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

n=1
status=0

echo_i "checking that SPF warnings have been correctly generated ($n)"
ret=0

grep "zone spf/IN: loaded serial 0" ns1/named.run > /dev/null || ret=1
grep "'y.spf' found type SPF" ns1/named.run > /dev/null || ret=1
grep "'spf' found type SPF" ns1/named.run > /dev/null && ret=1

grep "zone warn/IN: loaded serial 0" ns1/named.run > /dev/null || ret=1
grep "'y.warn' found type SPF" ns1/named.run > /dev/null || ret=1
grep "'warn' found type SPF" ns1/named.run > /dev/null && ret=1

grep "zone nowarn/IN: loaded serial 0" ns1/named.run > /dev/null || ret=1
grep "'y.nowarn' found type SPF" ns1/named.run > /dev/null && ret=1
grep "'nowarn' found type SPF" ns1/named.run > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
