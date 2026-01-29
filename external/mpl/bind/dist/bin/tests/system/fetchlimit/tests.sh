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

DIGCMD="$DIG @10.53.0.3 -p ${PORT} +tcp +tries=1 +time=1"

rndccmd() (
  "$RNDC" -c ../_common/rndc.conf -p "${CONTROLPORT}" -s "$@"
)

dig_with_opts() (
  "$DIG" -p "$PORT" "$@"
)

sendcmd() (
  SERVER="${1}"
  COMMAND="${2}"
  COMMAND_ARGS="${3}"
  dig_with_opts "@${SERVER}" "${COMMAND_ARGS}.${COMMAND}._control." TXT +time=5 +tries=1 +tcp >/dev/null 2>&1
)

burst() {
  server=${1}
  num=${4:-20}
  rm -f burst.input.$$
  while [ $num -gt 0 ]; do
    num=$((num - 1))
    if [ "${5}" = "dup" ]; then
      # burst with duplicate queries
      echo "${2}${3}.lamesub.example A" >>burst.input.$$
    else
      # burst with unique queries
      echo "${num}${2}${3}.lamesub.example A" >>burst.input.$$
    fi
  done
  $PERL ../ditch.pl -p ${PORT} -s ${server} -b ${EXTRAPORT8} burst.input.$$
  rm -f burst.input.$$
}

stat() {
  clients=$(rndccmd ${1} status | grep "recursive clients" \
    | sed 's;.*: \([^/][^/]*\)/.*;\1;')
  echo_i "clients: $clients"
  [ "$clients" = "" ] && return 1
  [ "$clients" -ge $2 ] || return 1
  [ "$clients" -le $3 ] || return 1
  return 0
}

_wait_for_message() (
  nextpartpeek "$1" >wait_for_message.$n
  grep -F "$2" wait_for_message.$n >/dev/null
)

wait_for_message() (
  retry_quiet 20 _wait_for_message "$@"
)

n=0
status=0

n=$((n + 1))
echo_i "checking recursing clients are dropped at the per-server limit ($n)"
ret=0
# make the server lame and restart
rndccmd 10.53.0.3 flush
sendcmd 10.53.0.4 send-responses "disable"
for try in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
  burst 10.53.0.3 a $try
  # fetches-per-server is at 400, but at 20qps against a lame server,
  # we'll reach 200 at the tenth second, and the quota should have been
  # tuned to less than that by then.
  [ $try -le 5 ] && low=$((try * 10))
  stat 10.53.0.3 20 200 || ret=1
  [ $ret -eq 1 ] && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "dumping ADB data ($n)"
ret=0
info=$(rndccmd 10.53.0.3 fetchlimit | grep 10.53.0.4 | sed 's/.*quota .*(\([0-9]*\).*atr \([.0-9]*\).*/\2 \1/')
echo_i $info
set -- $info
quota=$2
[ ${quota:-200} -lt 200 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking servfail statistics ($n)"
ret=0
rm -f ns3/named.stats
rndccmd 10.53.0.3 stats
for try in 1 2 3 4 5; do
  [ -f ns3/named.stats ] && break
  sleep 1
done
sspill=$(grep 'spilled due to server' ns3/named.stats | sed 's/\([0-9][0-9]*\) spilled.*/\1/')
[ -z "$sspill" ] && sspill=0
fails=$(grep 'queries resulted in SERVFAIL' ns3/named.stats | sed 's/\([0-9][0-9]*\) queries.*/\1/')
[ -z "$fails" ] && fails=0
[ "$fails" -ge "$sspill" ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking lame server recovery ($n)"
ret=0
sendcmd 10.53.0.4 send-responses "enable"
for try in 1 2 3 4 5; do
  burst 10.53.0.3 b $try
  stat 10.53.0.3 0 200 || ret=1
  [ $ret -eq 1 ] && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "dumping ADB data ($n)"
ret=0
info=$(rndccmd 10.53.0.3 fetchlimit | grep 10.53.0.4 | sed 's/.*quota .*(\([0-9]*\).*atr \([.0-9]*\).*/\2 \1/')
echo_i $info
set -- $info
[ ${2:-${quota}} -lt $quota ] || ret=1
quota=$2
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking lame server recovery (continued) ($n)"
ret=0
for try in 1 2 3 4 5 6 7 8 9 10; do
  burst 10.53.0.3 c $try
  stat 10.53.0.3 0 20 || ret=1
  [ $ret -eq 1 ] && break
  sleep 1
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "dumping ADB data ($n)"
ret=0
info=$(rndccmd 10.53.0.3 fetchlimit | grep 10.53.0.4 | sed 's/.*quota .*(\([0-9]*\).*atr \([.0-9]*\).*/\2 \1/')
echo_i $info
set -- $info
[ ${2:-${quota}} -gt $quota ] || ret=1
quota=$2
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

cp ns3/named2.conf ns3/named.conf
rndc_reconfig ns3 10.53.0.3

n=$((n + 1))
echo_i "checking lame server clients are dropped at the per-domain limit ($n)"
ret=0
fail=0
success=0
sendcmd 10.53.0.4 send-responses "disable"
for try in 1 2 3 4 5; do
  burst 10.53.0.3 d $try 300
  $DIGCMD a ${try}.example >dig.out.ns3.$n.$try
  grep "status: NOERROR" dig.out.ns3.$n.$try >/dev/null 2>&1 \
    && success=$((success + 1))
  grep "status: SERVFAIL" dig.out.ns3.$n.$try >/dev/null 2>&1 \
    && fail=$(($fail + 1))
  stat 10.53.0.3 40 40 || ret=1
  allowed=$(rndccmd 10.53.0.3 fetchlimit | awk '/lamesub/ { print $6 }')
  [ "${allowed:-0}" -eq 40 ] || ret=1
  [ $ret -eq 1 ] && break
  sleep 1
done
echo_i "$success successful valid queries, $fail SERVFAIL"
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking drop statistics ($n)"
ret=0
rm -f ns3/named.stats
rndccmd 10.53.0.3 stats
for try in 1 2 3 4 5; do
  [ -f ns3/named.stats ] && break
  sleep 1
done
zspill=$(grep 'spilled due to zone' ns3/named.stats | sed 's/\([0-9][0-9]*\) spilled.*/\1/')
[ -z "$zspill" ] && zspill=0
drops=$(grep 'queries dropped' ns3/named.stats | sed 's/\([0-9][0-9]*\) queries.*/\1/')
[ -z "$drops" ] && drops=0
[ "$drops" -ge "$zspill" ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

cp ns3/named3.conf ns3/named.conf
rndc_reconfig ns3 10.53.0.3

n=$((n + 1))
echo_i "checking lame server clients are dropped below the hard limit ($n)"
ret=0
fail=0
exceeded=0
success=0
sendcmd 10.53.0.4 send-responses "disable"
for try in 1 2 3 4 5; do
  burst 10.53.0.3 b $try 400
  $DIGCMD +time=2 a ${try}.example >dig.out.ns3.$n.$try
  stat 10.53.0.3 1 400 || exceeded=$((exceeded + 1))
  grep "status: NOERROR" dig.out.ns3.$n.$try >/dev/null 2>&1 \
    && success=$((success + 1))
  grep "status: SERVFAIL" dig.out.ns3.$n.$try >/dev/null 2>&1 \
    && fail=$(($fail + 1))
  sleep 1
done
echo_i "$success successful valid queries (expected 5)"
[ "$success" -eq 5 ] || {
  echo_i "failed"
  ret=1
}
echo_i "$fail SERVFAIL responses (expected 0)"
[ "$fail" -eq 0 ] || {
  echo_i "failed"
  ret=1
}
echo_i "clients count exceeded 400 on $exceeded trials (expected 0)"
[ "$exceeded" -eq 0 ] || {
  echo_i "failed"
  ret=1
}
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking drop statistics ($n)"
ret=0
rm -f ns3/named.stats
touch ns3/named.stats
rndccmd 10.53.0.3 stats
wait_for_log 5 "queries dropped due to recursive client limit" ns3/named.stats || ret=1
drops=$(grep 'queries dropped due to recursive client limit' ns3/named.stats | sed 's/\([0-9][0-9]*\) queries.*/\1/')
[ "${drops:-0}" -ne 0 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

nextpart ns5/named.run >/dev/null

n=$((n + 1))
echo_i "checking clients are dropped at the clients-per-query limit ($n)"
ret=0
sendcmd 10.53.0.4 send-responses "enable"
for try in 1 2 3 4 5; do
  burst 10.53.0.5 latency $try 20 "dup"
  sleep 1
done
wait_for_message ns5/named.run "clients-per-query increased to 10" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking drop statistics ($n)"
ret=0
rm -f ns5/named.stats
rndccmd 10.53.0.5 stats
for try in 1 2 3 4 5; do
  [ -f ns5/named.stats ] && break
  sleep 1
done
zspill=$(grep 'spilled due to clients per query' ns5/named.stats | sed 's/ *\([0-9][0-9]*\) spilled.*/\1/')
[ -z "$zspill" ] && zspill=0
# ns5 configuration:
#   clients-per-query 5
#   max-clients-per-query 10
# expected spills:
#   15 (out of 20) spilled for the first burst, and 10 (out of 20) spilled for
#   the next 4 bursts (because of auto-tuning): 15 + (4 * 10) == 55
expected=55
[ "$zspill" -eq "$expected" ] || ret=1
echo_i "$zspill clients spilled (expected $expected)"
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "stop ns5"
stop_server --use-rndc --port ${CONTROLPORT} ns5
cp ns5/named2.conf ns5/named.conf
echo_i "start ns5"
start_server --noclean --restart --port ${PORT} ns5

nextpart ns5/named.run >/dev/null

n=$((n + 1))
echo_i "checking clients are dropped at the clients-per-query limit with stale-answer-client-timeout ($n)"
ret=0
sendcmd 10.53.0.4 send-responses "enable"
for try in 1 2 3 4 5; do
  burst 10.53.0.5 latency $try 20 "dup"
  sleep 1
done
wait_for_message ns5/named.run "clients-per-query increased to 10" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking drop statistics ($n)"
ret=0
rm -f ns5/named.stats
rndccmd 10.53.0.5 stats
for try in 1 2 3 4 5; do
  [ -f ns5/named.stats ] && break
  sleep 1
done
zspill=$(grep 'spilled due to clients per query' ns5/named.stats | sed 's/ *\([0-9][0-9]*\) spilled.*/\1/')
[ -z "$zspill" ] && zspill=0
# ns5 configuration:
#   clients-per-query 5
#   max-clients-per-query 10
# expected spills:
#   15 (out of 20) spilled for the first burst, and 10 (out of 20) spilled for
#   the next 4 bursts (because of auto-tuning): 15 + (4 * 10) == 55
expected=55
[ "$zspill" -eq "$expected" ] || ret=1
echo_i "$zspill clients spilled (expected $expected)"
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking a warning is logged if max-clients-per-query < clients-per-query ($n)"
ret=0
cp ns5/named3.conf ns5/named.conf
rndc_reconfig ns5 10.53.0.5
wait_for_message ns5/named.run "configured clients-per-query (10) exceeds max-clients-per-query (5); automatically adjusting max-clients-per-query to (10)" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
