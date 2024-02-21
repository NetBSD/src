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

#shellcheck source=conf.sh
. ../conf.sh

dig_with_opts() {
  "$DIG" -p "${PORT}" "$@"
}

wait_for_pid() (
  for pid in "$@"; do
    kill -0 "$pid" 2>/dev/null && return 1
  done
  return 0
)

status=0
n=0

n=$((n + 1))
echo_i "check lookups against TTL=0 records ($n)"
i=0
ret=0
passes=10
dig_with_opts @10.53.0.2 axfr example | grep -v "^ds0" \
  | awk '$2 == "0" { print "-q", $1, $4; print "-q", "zzz"$1, $4;}' >query.list

# add 1/5 second per query
timeout=$(($(wc -l <query.list) / 5))
while [ $i -lt $passes ]; do
  (dig_with_opts -d +qr @10.53.0.3 -f query.list >"dig.out$i.1.test$n" 2>&1) &
  pid1="$!"
  (dig_with_opts -d +qr @10.53.0.3 -f query.list >"dig.out$i.2.test$n" 2>&1) &
  pid2="$!"
  (dig_with_opts -d +qr @10.53.0.3 -f query.list >"dig.out$i.3.test$n" 2>&1) &
  pid3="$!"
  (dig_with_opts -d +qr @10.53.0.3 -f query.list >"dig.out$i.4.test$n" 2>&1) &
  pid4="$!"
  (dig_with_opts -d +qr @10.53.0.3 -f query.list >"dig.out$i.5.test$n" 2>&1) &
  pid5="$!"
  (dig_with_opts -d +qr @10.53.0.3 -f query.list >"dig.out$i.6.test$n" 2>&1) &
  pid6="$!"

  retry_quiet "$timeout" wait_for_pid "$pid1" "$pid2" "$pid3" "$pid4" "$pid5" "$pid6" || {
    echo_i "wait_for_pid failed"
    ret=1
  }
  kill -TERM "$pid1" "$pid2" "$pid3" "$pid4" "$pid5" "$pid6" 2>/dev/null || true

  wait "$pid1" || {
    echo_i "wait $pid1 (dig.out$i.1.test$n) failed with $?"
    ret=1
  }
  wait "$pid2" || {
    echo_i "wait $pid2 (dig.out$i.2.test$n) failed with $?"
    ret=1
  }
  wait "$pid3" || {
    echo_i "wait $pid3 (dig.out$i.3.test$n) failed with $?"
    ret=1
  }
  wait "$pid4" || {
    echo_i "wait $pid4 (dig.out$i.4.test$n) failed with $?"
    ret=1
  }
  wait "$pid5" || {
    echo_i "wait $pid5 (dig.out$i.5.test$n) failed with $?"
    ret=1
  }
  wait "$pid6" || {
    echo_i "wait $pid6 (dig.out$i.6.test$n) failed with $?"
    ret=1
  }

  grep "status: SERVFAIL" "dig.out$i.1.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.2.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.3.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.4.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.5.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.6.test$n" >/dev/null && ret=1
  [ $ret = 1 ] && break
  i=$((i + 1))
  echo_i "successfully completed pass $i of $passes"
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

repeat_query() (
  i=0
  while [ "$i" -lt "$1" ]; do
    dig_with_opts +short "@$2" "$3" | tee "dig.out$i.test$n" || return 1
    i=$((i + 1))
  done
)

count_unique() (
  repeat_query "$@" | sort -u | wc -l
)

n=$((n + 1))
echo_i "check repeated recursive lookups of non recurring TTL=0 responses get new values ($n)"
ret=0
repeats=9
count=$(count_unique "$repeats" 10.53.0.3 foo.increment)
if [ "$count" -ne "$repeats" ]; then
  echo_i "failed (count=$count, repeats=$repeats)"
  ret=1
fi
status=$((status + ret))

n=$((n + 1))
echo_i "check lookups against TTL=1 records ($n)"
i=0
passes=10
ret=0
while [ $i -lt $passes ]; do
  dig_with_opts @10.53.0.3 www.one.tld >"dig.out$i.1.test$n" || ret=1
  dig_with_opts @10.53.0.3 www.one.tld >"dig.out$i.2.test$n" || ret=1
  dig_with_opts @10.53.0.3 www.one.tld >"dig.out$i.3.test$n" || ret=1
  dig_with_opts @10.53.0.3 www.one.tld >"dig.out$i.4.test$n" || ret=1
  dig_with_opts @10.53.0.3 www.one.tld >"dig.out$i.5.test$n" || ret=1
  dig_with_opts @10.53.0.3 www.one.tld >"dig.out$i.6.test$n" || ret=1
  grep "status: SERVFAIL" "dig.out$i.1.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.2.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.3.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.4.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.5.test$n" >/dev/null && ret=1
  grep "status: SERVFAIL" "dig.out$i.6.test$n" >/dev/null && ret=1
  [ $ret = 1 ] && break
  i=$((i + 1))
  echo_i "successfully completed pass $i of $passes"
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ "$status" -eq 0 ] || exit 1
