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

dig_with_opts() {
  "$DIG" -p "${PORT}" "$@"
}

rndccmd() {
  "$RNDC" -c ../_common/rndc.conf -p "${CONTROLPORT}" -s "$@"
}

status=0
n=0

n=$((n + 1))
echo_i "querying for non-existing zone data ($n)"
ret=0
dig_with_opts @10.53.0.1 a.added.example a >dig.out.ns1.$n || ret=1
grep 'status: REFUSED' dig.out.ns1.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "adding a new zone into default NZD using rndc addzone ($n)"
rndccmd 10.53.0.1 addzone 'added.example { type primary; file "added.db"; };' 2>&1 | sed 's/^/I:ns1 /' | cat_i
sleep 2

n=$((n + 1))
echo_i "querying for existing zone data ($n)"
ret=0
dig_with_opts @10.53.0.1 a.added.example a >dig.out.ns1.$n || ret=1
grep 'status: NOERROR' dig.out.ns1.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "stopping ns1"
stop_server ns1

n=$((n + 1))
echo_i "dumping _default.nzd to _default.nzf ($n)"
$NZD2NZF ns1/_default.nzd >ns1/_default.nzf || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that _default.nzf contains the expected content ($n)"
grep 'zone "added.example" { type primary; file "added.db"; };' ns1/_default.nzf >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "deleting _default.nzd database"
rm -f ns1/_default.nzd

echo_i "starting ns1 which should migrate the .nzf to .nzd"
start_server --noclean --restart --port ${PORT} ns1

n=$((n + 1))
echo_i "querying for zone data from migrated zone config ($n)"
# retry loop in case the server restart above causes transient failures
_do_query() (
  dig_with_opts @10.53.0.1 a.added.example a >dig.out.ns1.$n \
    && grep 'status: NOERROR' dig.out.ns1.$n >/dev/null
)
ret=0
retry_quiet "10" _do_query || ret=1
n=$((n + 1))
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
exit $status
