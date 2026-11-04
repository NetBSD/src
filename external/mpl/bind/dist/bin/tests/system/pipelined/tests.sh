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
# On FreeBSD, the TCP connect() call can transiently fail with
# EADDRINUSE even after the netmgr retried it in place: the socket is
# already bound, so retrying on the same source port cannot help.
# pipequeries then bails out before any query is sent, which leaves
# the ns4 cache cold, so it is safe to simply run it again (and the
# out-of-order check below remains meaningful on a repeated run).
#
# This loop is a workaround for the pipequeries.c implementation.  If
# pipequeries is ever rewritten in pure Python (using the test suite's
# own DNS machinery, which can pick a fresh source port per attempt),
# this retry should no longer be necessary and can be dropped.
pq_left=10
while :; do
  ret=0
  pipequeries <input >raw.$n 2>pipequeries.err.$n || ret=1
  cat pipequeries.err.$n >&2
  pq_left=$((pq_left - 1))
  if [ $ret -eq 0 ] || [ $pq_left -le 0 ]; then
    break
  fi
  if ! grep "address in use" pipequeries.err.$n >/dev/null; then
    break
  fi
  echo_i "retrying pipequeries after a transient connect failure"
  sleep 1
done
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
