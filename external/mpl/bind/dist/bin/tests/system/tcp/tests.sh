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
  "${DIG}" -p "${PORT}" "$@"
}

rndccmd() {
  "${RNDC}" -p "${CONTROLPORT}" -c ../_common/rndc.conf -s "$@"
}

status=0
n=0

n=$((n + 1))
echo_i "initializing TCP statistics ($n)"
ret=0
rndccmd 10.53.0.1 stats || ret=1
rndccmd 10.53.0.2 stats || ret=1
mv ns1/named.stats ns1/named.stats.test$n
mv ns2/named.stats ns2/named.stats.test$n
ntcp10="$(grep "TCP requests received" ns1/named.stats.test$n | tail -1 | awk '{print $1}')"
ntcp20="$(grep "TCP requests received" ns2/named.stats.test$n | tail -1 | awk '{print $1}')"
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking TCP request statistics (resolver) ($n)"
ret=0
dig_with_opts @10.53.0.3 txt.example. >dig.out.test$n
sleep 1
rndccmd 10.53.0.1 stats || ret=1
rndccmd 10.53.0.2 stats || ret=1
mv ns1/named.stats ns1/named.stats.test$n
mv ns2/named.stats ns2/named.stats.test$n
ntcp11="$(grep "TCP requests received" ns1/named.stats.test$n | tail -1 | awk '{print $1}')"
ntcp21="$(grep "TCP requests received" ns2/named.stats.test$n | tail -1 | awk '{print $1}')"
if [ "$ntcp10" -ge "$ntcp11" ]; then ret=1; fi
if [ "$ntcp20" -ne "$ntcp21" ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking TCP request statistics (forwarder) ($n)"
ret=0
dig_with_opts @10.53.0.4 txt.example. >dig.out.test$n
sleep 1
rndccmd 10.53.0.1 stats || ret=1
rndccmd 10.53.0.2 stats || ret=1
mv ns1/named.stats ns1/named.stats.test$n
mv ns2/named.stats ns2/named.stats.test$n
ntcp12="$(grep "TCP requests received" ns1/named.stats.test$n | tail -1 | awk '{print $1}')"
ntcp22="$(grep "TCP requests received" ns2/named.stats.test$n | tail -1 | awk '{print $1}')"
if [ "$ntcp11" -ne "$ntcp12" ]; then ret=1; fi
if [ "$ntcp21" -ge "$ntcp22" ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# -------- TCP high-water tests ----------
refresh_tcp_stats() {
  rndccmd 10.53.0.5 status >rndc.out.$n || ret=1
  TCP_CUR="$(sed -n "s/^tcp clients: \([0-9][0-9]*\).*/\1/p" rndc.out.$n)"
  TCP_LIMIT="$(sed -n "s/^tcp clients: .*\/\([0-9][0-9]*\)/\1/p" rndc.out.$n)"
  TCP_HIGH="$(sed -n "s/^TCP high-water: \([0-9][0-9]*\)/\1/p" rndc.out.$n)"
}

# Send a command to the tool script listening on 10.53.0.6.
send_command() {
  nextpart ans6/ans.run >/dev/null
  echo "$*" | send 10.53.0.6 "${CONTROLPORT}"
  wait_for_log_peek 10 "result=" ans6/ans.run || ret=1
  if ! nextpartpeek ans6/ans.run | grep -qF "result=OK"; then
    return 1
  fi
}

# Instructs ans6 to open $1 TCP connections to 10.53.0.5.
open_connections() {
  send_command "open" "${1}" 10.53.0.5 "${PORT}" || return 1
}

# Instructs ans6 to close $1 TCP connections to 10.53.0.5.
close_connections() {
  send_command "close" "${1}" || return 1
}

# Check TCP connections are working normally before opening
# multiple connections
n=$((n + 1))
echo_i "checking TCP query repsonse ($n)"
ret=0
dig_with_opts +tcp @10.53.0.5 txt.example >dig.out.test$n
grep "status: NXDOMAIN" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check TCP statistics after server startup before using them as a baseline for
# subsequent checks.
n=$((n + 1))
echo_i "TCP high-water: check initial statistics ($n)"
ret=0
refresh_tcp_stats
assert_int_equal "${TCP_CUR}" 0 "current TCP clients count" || ret=1
# We compare initial tcp-highwater value with 1 because as part of the
# system test startup, the script start.pl executes dig to check if target
# named is running, and that increments tcp-quota by one.
assert_int_equal "${TCP_HIGH}" 1 "tcp-highwater count" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Ensure the TCP high-water statistic gets updated after some TCP connections
# are established.
n=$((n + 1))
echo_i "TCP high-water: check value after some TCP connections are established ($n)"
ret=0
OLD_TCP_CUR="${TCP_CUR}"
TCP_ADDED=9
open_connections "${TCP_ADDED}" || ret=1
check_stats_added() {
  refresh_tcp_stats
  assert_int_equal "${TCP_CUR}" $((OLD_TCP_CUR + TCP_ADDED)) "current TCP clients count" || return 1
  assert_int_equal "${TCP_HIGH}" $((OLD_TCP_CUR + TCP_ADDED)) "TCP high-water value" || return 1
}
retry 2 check_stats_added || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Ensure the TCP high-water statistic remains unchanged after some TCP
# connections are closed.
n=$((n + 1))
echo_i "TCP high-water: check value after some TCP connections are closed ($n)"
ret=0
OLD_TCP_CUR="${TCP_CUR}"
OLD_TCP_HIGH="${TCP_HIGH}"
TCP_REMOVED=5
close_connections "${TCP_REMOVED}" || ret=1
check_stats_removed() {
  refresh_tcp_stats
  assert_int_equal "${TCP_CUR}" $((OLD_TCP_CUR - TCP_REMOVED)) "current TCP clients count" || return 1
  assert_int_equal "${TCP_HIGH}" "${OLD_TCP_HIGH}" "TCP high-water value" || return 1
}
retry 2 check_stats_removed || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Ensure the TCP high-water statistic never exceeds the configured TCP clients
# limit.
n=$((n + 1))
echo_i "TCP high-water: ensure tcp-clients is an upper bound ($n)"
ret=0
open_connections $((TCP_LIMIT + 1)) || ret=1
check_stats_limit() {
  refresh_tcp_stats
  assert_int_equal "${TCP_CUR}" "${TCP_LIMIT}" "current TCP clients count" || return 1
  assert_int_equal "${TCP_HIGH}" "${TCP_LIMIT}" "TCP high-water value" || return 1
}
retry 2 check_stats_limit || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# Check TCP connections are working normally before opening
# multiple connections
n=$((n + 1))
echo_i "checking TCP response recovery ($n)"
ret=0
# "0" closes all connections
close_connections 0 || ret=1
dig_with_opts +tcp @10.53.0.5 txt.example >dig.out.test$n || ret=1
grep "status: NXDOMAIN" dig.out.test$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

####################################################
# NOTE: The next test resets the debug level to 1. #
####################################################

n=$((n + 1))
echo_i "checking that BIND 9 doesn't crash on long TCP messages ($n)"
ret=0
# Avoid logging useless information.
rndccmd 10.53.0.1 trace 1 || ret=1
{ $PERL ../packet.pl -a "10.53.0.1" -p "${PORT}" -t tcp -r 300000 1996-alloc_dnsbuf-crash-test.pkt || ret=1; } | cat_i
dig_with_opts +tcp @10.53.0.1 txt.example >dig.out.test$n || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
