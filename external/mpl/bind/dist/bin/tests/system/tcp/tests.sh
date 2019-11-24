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

DIGOPTS="-p ${PORT}"
RNDCCMD="$RNDC -p ${CONTROLPORT} -c ../common/rndc.conf"
SEND="$PERL $SYSTEMTESTTOP/send.pl 10.53.0.6 ${CONTROLPORT}"

status=0

echo_i "initialize counters"
$RNDCCMD -s 10.53.0.1 stats > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
ntcp10=`grep "TCP requests received" ns1/named.stats | tail -1 | awk '{print $1}'`
ntcp20=`grep "TCP requests received" ns2/named.stats | tail -1 | awk '{print $1}'`
#echo ntcp10 ':' "$ntcp10"
#echo ntcp20 ':' "$ntcp20"

echo_i "check TCP transport"
ret=0
$DIG $DIGOPTS @10.53.0.3 txt.example. > dig.out.3
sleep 1
$RNDCCMD -s 10.53.0.1 stats > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
ntcp11=`grep "TCP requests received" ns1/named.stats | tail -1 | awk '{print $1}'`
ntcp21=`grep "TCP requests received" ns2/named.stats | tail -1 | awk '{print $1}'`
#echo ntcp11 ':' "$ntcp11"
#echo ntcp21 ':' "$ntcp21"
if [ "$ntcp10" -ge "$ntcp11" ]; then ret=1; fi
if [ "$ntcp20" -ne "$ntcp21" ]; then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "check TCP forwarder"
ret=0
$DIG $DIGOPTS @10.53.0.4 txt.example. > dig.out.4
sleep 1
$RNDCCMD -s 10.53.0.1 stats > /dev/null 2>&1
$RNDCCMD -s 10.53.0.2 stats > /dev/null 2>&1
ntcp12=`grep "TCP requests received" ns1/named.stats | tail -1 | awk '{print $1}'`
ntcp22=`grep "TCP requests received" ns2/named.stats | tail -1 | awk '{print $1}'`
#echo ntcp12 ':' "$ntcp12"
#echo ntcp22 ':' "$ntcp22"
if [ "$ntcp11" -ne "$ntcp12" ]; then ret=1; fi
if [ "$ntcp21" -ge "$ntcp22" ];then ret=1; fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# -------- TCP high-water tests ----------
n=0

refresh_tcp_stats() {
	$RNDCCMD -s 10.53.0.5 status > rndc.out.$n || ret=1
	TCP_CUR="$(sed -n "s/^tcp clients: \([0-9][0-9]*\).*/\1/p" rndc.out.$n)"
	TCP_LIMIT="$(sed -n "s/^tcp clients: .*\/\([0-9][0-9]*\)/\1/p" rndc.out.$n)"
	TCP_HIGH="$(sed -n "s/^TCP high-water: \([0-9][0-9]*\)/\1/p" rndc.out.$n)"
}

wait_for_log() {
	msg=$1
	file=$2
	for i in 1 2 3 4 5 6 7 8 9 10; do
		nextpart "$file" | grep "$msg" > /dev/null && return
		sleep 1
	done
	echo_i "exceeded time limit waiting for '$msg' in $file"
	ret=1
}

# Send a command to the tool script listening on 10.53.0.6.
send_command() {
	nextpart ans6/ans.run > /dev/null
	echo "$*" | $SEND
	wait_for_log "result=OK" ans6/ans.run
}

# Instructs ans6 to open $1 TCP connections to 10.53.0.5.
open_connections() {
	send_command "open" "${1}" 10.53.0.5 "${PORT}"
}

# Instructs ans6 to close $1 TCP connections to 10.53.0.5.
close_connections() {
	send_command "close" "${1}"
}

# Check TCP statistics after server startup before using them as a baseline for
# subsequent checks.
n=$((n + 1))
echo_i "TCP high-water: check initial statistics ($n)"
ret=0
refresh_tcp_stats
assert_int_equal "${TCP_CUR}" 1 "current TCP clients count" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Ensure the TCP high-water statistic gets updated after some TCP connections
# are established.
n=$((n + 1))
echo_i "TCP high-water: check value after some TCP connections are established ($n)"
ret=0
OLD_TCP_CUR="${TCP_CUR}"
TCP_ADDED=9
open_connections "${TCP_ADDED}"
refresh_tcp_stats
assert_int_equal "${TCP_CUR}" $((OLD_TCP_CUR + TCP_ADDED)) "current TCP clients count" || ret=1
assert_int_equal "${TCP_HIGH}" $((OLD_TCP_CUR + TCP_ADDED)) "TCP high-water value" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Ensure the TCP high-water statistic remains unchanged after some TCP
# connections are closed.
n=$((n + 1))
echo_i "TCP high-water: check value after some TCP connections are closed ($n)"
ret=0
OLD_TCP_CUR="${TCP_CUR}"
OLD_TCP_HIGH="${TCP_HIGH}"
TCP_REMOVED=5
close_connections "${TCP_REMOVED}"
refresh_tcp_stats
assert_int_equal "${TCP_CUR}" $((OLD_TCP_CUR - TCP_REMOVED)) "current TCP clients count" || ret=1
assert_int_equal "${TCP_HIGH}" "${OLD_TCP_HIGH}" "TCP high-water value" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Ensure the TCP high-water statistic never exceeds the configured TCP clients
# limit.
n=$((n + 1))
echo_i "TCP high-water: ensure tcp-clients is an upper bound ($n)"
ret=0
open_connections $((TCP_LIMIT + 1))
refresh_tcp_stats
assert_int_equal "${TCP_CUR}" "${TCP_LIMIT}" "current TCP clients count" || ret=1
assert_int_equal "${TCP_HIGH}" "${TCP_LIMIT}" "TCP high-water value" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
