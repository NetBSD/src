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

# shellcheck source=../conf.sh
. ../conf.sh

dig_with_opts() {
  "${DIG}" -p "${PORT}" "${@}"
}

status=0
n=0

n=$((n + 1))
echo_i "checking max-query-count is in effect ($n)"
ret=0
dig_with_opts q.label1.tld1. @10.53.0.9 a >dig.out.ns9.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns9.test${n} >/dev/null || ret=1
grep "exceeded global max queries resolving" ns9/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
