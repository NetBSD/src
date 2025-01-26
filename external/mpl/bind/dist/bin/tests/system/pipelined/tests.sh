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

mdig_with_opts() {
  "$MDIG" -p "${PORT}" "${@}"
}

rndccmd() {
  "$RNDC" -c ../_common/rndc.conf -p "${CONTROLPORT}" -s "${@}"
}

pipequeries() {
  "$PIPEQUERIES" -p "${PORT}"
}

status=0
n=1
ret=0

echo_i "check pipelined TCP queries ($n)"
pipequeries <input >raw.$n || ret=1
awk '{ print $1 " " $5 }' <raw.$n >output.$n
sort <output.$n >output-sorted.$n
diff ref output-sorted.$n || {
  ret=1
  echo_i "diff sorted failed"
}
diff ref output.$n >/dev/null && {
  ret=1
  echo_i "diff out of order failed"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))
ret=0

echo_i "check pipelined TCP queries using mdig ($n)"
rndccmd 10.53.0.4 flush
wait_for_log 10 "flushing caches in all views succeeded" ns4/named.run
mdig_with_opts +noall +answer +vc -f input -b 10.53.0.4 @10.53.0.4 >raw.mdig.$n
awk '{ print $1 " " $5 }' <raw.mdig.$n >output.mdig.$n
sort <output.mdig.$n >output-sorted.mdig.$n
diff ref output-sorted.mdig.$n || {
  ret=1
  echo_i "diff sorted failed"
}
diff ref output.mdig.$n >/dev/null && {
  ret=1
  echo_i "diff out of order failed"
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))
ret=0

echo_i "check mdig -4 -6 ($n)"
mdig_with_opts -4 -6 -f input @10.53.0.4 >output.mdig.$n 2>&1 && ret=1
grep "only one of -4 and -6 allowed" output.mdig.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))
ret=0

echo_i "check mdig -4 with an IPv6 server address ($n)"
mdig_with_opts -4 -f input @fd92:7065:b8e:ffff::2 >output.mdig.$n 2>&1 && ret=1
grep "address family not supported" output.mdig.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))
n=$((n + 1))
ret=0

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
