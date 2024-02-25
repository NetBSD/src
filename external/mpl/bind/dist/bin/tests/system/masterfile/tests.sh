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

DIGOPTS="-p ${PORT}"

status=0
n=0

ret=0
n=$((n + 1))
echo_i "test master file \$INCLUDE semantics ($n)"
$DIG $DIGOPTS +nostats +nocmd include. axfr @10.53.0.1 >dig.out.$n

echo_i "test master file BIND 8 compatibility TTL and \$TTL semantics ($n)"
$DIG $DIGOPTS +nostats +nocmd ttl2. axfr @10.53.0.1 >>dig.out.$n

echo_i "test of master file RFC1035 TTL and \$TTL semantics ($n)"
$DIG $DIGOPTS +nostats +nocmd ttl2. axfr @10.53.0.1 >>dig.out.$n

diff dig.out.$n knowngood.dig.out || status=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

ret=0
n=$((n + 1))
echo_i "test that the nameserver is running with a missing master file ($n)"
$DIG $DIGOPTS +tcp +noall +answer example soa @10.53.0.2 >dig.out.$n
grep SOA dig.out.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

ret=0
n=$((n + 1))
echo_i "test that the nameserver returns SERVFAIL for a missing master file ($n)"
$DIG $DIGOPTS +tcp +all missing soa @10.53.0.2 >dig.out.$n
grep "status: SERVFAIL" dig.out.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

ret=0
n=$((n + 1))
echo_i "test owner inheritance after "'$INCLUDE'" ($n)"
$CHECKZONE -Dq example zone/inheritownerafterinclude.db >checkzone.out$n
diff checkzone.out$n zone/inheritownerafterinclude.good || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
