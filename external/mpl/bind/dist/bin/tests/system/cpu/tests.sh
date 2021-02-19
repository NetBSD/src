#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. ../conf.sh

status=0
n=0

n=$((n+1))
echo_i "stop server ($n)"
ret=0
$PERL ../stop.pl cpu ns1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

n=$((n+1))
echo_i "start server with taskset ($n)"
ret=0
start_server --noclean --taskset fff0 --restart --port "${PORT}" cpu ns1 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

n=$((n+1))
echo_i "check ps output ($n)"
ret=0
ps -T -o pid,psr,time,comm -e > ps.out
pid=$(cat ns1/named.pid)
echo_i "pid=$pid"
psr=$(awk -v pid="$pid" '$1 == pid && $4 == "isc-net-0000" {print $2}' < ps.out)
echo_i "psr=$psr"
# The next available cpu relative to the existing affinity mask is 4.
test "$psr" -eq 4 || ret=1
test "$ret" -eq 0 || echo_i "failed"
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
