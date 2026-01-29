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

# shellcheck disable=SC2034
. ../conf.sh

dig_plus_opts() {
  $DIG +tcp +noadd +nosea +nostat +noquest +nocomm +nocmd -p "${PORT}" "$@"
}

status=0
n=0

test_start() {
  n=$((n + 1))
  echo_i "$* ($n)"
  ret=0
}

test_end() {
  [ $ret = 0 ] || echo_i "failed"
  status=$((status + ret))
}

#
# Wait up to 10 seconds for the servers to finish starting before testing.
#
for i in 1 2 3 4 5 6 7 8 9 10; do
  ret=0
  $DIG +tcp -p "${PORT}" example @10.53.0.2 soa >dig.out.ns2.test$n || ret=1
  grep "status: NOERROR" dig.out.ns2.test$n >/dev/null || ret=1
  grep "flags:.* aa[ ;]" dig.out.ns2.test$n >/dev/null || ret=1
  $DIG +tcp -p "${PORT}" example @10.53.0.3 soa >dig.out.ns3.test$n || ret=1
  grep "status: NOERROR" dig.out.ns3.test$n >/dev/null || ret=1
  grep "flags:.* aa[ ;]" dig.out.ns3.test$n >/dev/null || ret=1
  nr=$(grep -c 'x[0-9].*sending notify to' ns2/named.run)
  [ "$nr" -ge 22 ] || ret=1
  [ $ret = 0 ] && break
  sleep 1
done

test_start "checking initial status"

dig_plus_opts a.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
grep "10.0.0.1" dig.out.ns2.test$n >/dev/null || ret=1

dig_plus_opts a.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
grep "10.0.0.1" dig.out.ns3.test$n >/dev/null || ret=1

digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

test_end

test_start "checking startup notify rate limit"
awk '/x[0-9].*sending notify to/ {
	split($2, a, ":");
	this = a[1] * 3600 + a[2] * 60 + a[3];
	if (lasta1 && lasta1 > a[1]) {
		fix = 3600 * 24;
	}
	this += fix;
	if (last) {
		delta = this - last;
		print delta;

		total += delta;
		if (!maxdelta || delta > maxdelta) {
			maxdelta = delta;
		}
		if (!mindelta || delta < mindelta) {
			mindelta = delta;
		}
	}
	lasta1 = a[1];
	last = this;
	count++;
}
END {
	average = total / count;
	print "mindelta:", mindelta;
	print "maxdelta:" maxdelta;
	print "count:", count;
	print "average:", average;
	if (average < 0.180) exit(1);
	if (count < 22) exit(1);
}' ns2/named.run >awk.out.ns2.test$n || ret=1
test_end

# See [GL#4689]
test_start "checking server behaviour with invalid notify-source-v6 address"
grep "zone ./IN: sending notify to fd92:7065:b8e:fffe::a35:4#" ns1/named.run >/dev/null || ret=1
grep "dns_request_create: failed address not available" ns1/named.run >/dev/null || ret=1
test_end

nextpart ns3/named.run >/dev/null

sleep 1 # make sure filesystem time stamp is newer for reload.
rm -f ns2/example.db
cp -f ns2/example2.db ns2/example.db
echo_i "reloading with example2 using HUP and waiting up to 45 seconds"
kill -HUP "$(cat ns2/named.pid)"
wait_for_log_re 45 "transfer of 'example/IN' from 10.53.0.2#.*success" ns3/named.run

test_start "checking notify message was logged"
grep 'notify from 10.53.0.2#[0-9][0-9]*: serial 2$' ns3/named.run >/dev/null || ret=1
grep 'refused notify from non-primary: fd92:7065:b8e:ffff::2#[0-9][0-9]*$' ns3/named.run >/dev/null || ret=1
test_end

if $FEATURETEST --have-fips-dh; then
  test_start "checking notify over TLS successful"
  grep "zone tls-x1/IN: notify to 10.53.0.2#${TLSPORT} successful" ns3/named.run >/dev/null || ret=1
  grep "zone tls-x2/IN: notify to 10.53.0.2#${EXTRAPORT1} successful" ns3/named.run >/dev/null || ret=1
  grep "zone tls-x3/IN: notify to 10.53.0.2#${EXTRAPORT1} successful" ns3/named.run >/dev/null || ret=1
  grep "zone tls-x5/IN: notify to 10.53.0.2#${EXTRAPORT3} successful" ns3/named.run >/dev/null || ret=1
  test_end

  test_start "checking notify over TLS failed"
  grep "zone tls-x4/IN: notify to 10.53.0.2#${EXTRAPORT1} failed: TLS peer certificate verification failed" ns3/named.run >/dev/null || ret=1
  grep "zone tls-x6/IN: notify to 10.53.0.2#${EXTRAPORT4} failed: TLS peer certificate verification failed" ns3/named.run >/dev/null || ret=1
  test_end
fi

test_start "checking example2 loaded"
dig_plus_opts a.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
grep "10.0.0.2" dig.out.ns2.test$n >/dev/null || ret=1
test_end

test_start "checking example2 contents have been transferred after HUP reload"

dig_plus_opts a.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
grep "10.0.0.2" dig.out.ns2.test$n >/dev/null || ret=1

dig_plus_opts a.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
grep "10.0.0.2" dig.out.ns3.test$n >/dev/null || ret=1

digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

test_end

echo_i "stopping primary and restarting with example4 then waiting up to 45 seconds"
stop_server ns2

rm -f ns2/example.db
cp -f ns2/example4.db ns2/example.db

start_server --noclean --restart --port "${PORT}" ns2
wait_for_log_re 45 "transfer of 'example/IN' from 10.53.0.2#.*success" ns3/named.run

test_start "checking notify message was logged"
grep 'notify from 10.53.0.2#[0-9][0-9]*: serial 4$' ns3/named.run >/dev/null || ret=1
test_end

test_start "checking example4 loaded"
dig_plus_opts a.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
grep "10.0.0.4" dig.out.ns2.test$n >/dev/null || ret=1
test_end

test_start "checking example4 contents have been transferred after restart"

dig_plus_opts a.example. @10.53.0.2 a >dig.out.ns2.test$n || ret=1
grep "10.0.0.4" dig.out.ns2.test$n >/dev/null || ret=1

dig_plus_opts a.example. @10.53.0.3 a >dig.out.ns3.test$n || ret=1
grep "10.0.0.4" dig.out.ns3.test$n >/dev/null || ret=1

digcomp dig.out.ns2.test$n dig.out.ns3.test$n || ret=1

test_end

test_start "checking notify to alternate port with primary server inheritance"
$NSUPDATE <<EOF
server 10.53.0.2 ${PORT}
zone x21
update add added.x21 0 in txt "test string"
send
EOF
fn="dig.out.ns4.test$n"
for i in 1 2 3 4 5 6 7 8 9; do
  dig_plus_opts added.x21. @10.53.0.4 txt -p "$EXTRAPORT1" >"$fn" || ret=1
  grep "test string" "$fn" >/dev/null && break
  sleep 1
done
grep "test string" "$fn" >/dev/null || ret=1
test_end

test_start "checking notify to multiple views using tsig"
$NSUPDATE <<EOF
server 10.53.0.5 ${PORT}
zone x21
key $DEFAULT_HMAC:a aaaaaaaaaaaaaaaaaaaa
update add added.x21 0 in txt "test string"
send
EOF
fnb="dig.out.b.ns5.test$n"
fnc="dig.out.c.ns5.test$n"
for i in 1 2 3 4 5 6 7 8 9; do
  dig_plus_opts added.x21. -y "${DEFAULT_HMAC}:b:bbbbbbbbbbbbbbbbbbbb" @10.53.0.5 \
    txt >"$fnb" || ret=1
  dig_plus_opts added.x21. -y "${DEFAULT_HMAC}:c:cccccccccccccccccccc" @10.53.0.5 \
    txt >"$fnc" || ret=1
  grep "test string" "$fnb" >/dev/null \
    && grep "test string" "$fnc" >/dev/null \
    && break
  sleep 1
done
grep "test string" "$fnb" >/dev/null || ret=1
grep "test string" "$fnc" >/dev/null || ret=1
grep "sending notify to 10.53.0.5#[0-9]* : TSIG (b)" ns5/named.run >/dev/null || ret=1
grep "sending notify to 10.53.0.5#[0-9]* : TSIG (c)" ns5/named.run >/dev/null || ret=1
test_end

test_start "checking notify-source uses port option correctly"
grep "10.53.0.3#${EXTRAPORT2}: received notify for zone 'notify-source-port-test'" ns2/named.run >/dev/null || ret=1
test_end

# notify messages were sent to unresponsive 10.53.10.53 during the tests
# above, which should time out at some point; we need to wait for them to
# appear in the logs in case the tests run faster than the notify timeouts

test_start "checking notify to retry over TCP within 45 seconds"
nextpartreset ns3/named.run
wait_for_log 45 'retrying over TCP' ns3/named.run || ret=1
test_end

# the TCP timeout is set to 15 seconds, double that for some leeway
test_start "checking notify retries expire within 30 seconds"
nextpartreset ns3/named.run
wait_for_log 30 'retries exceeded' ns3/named.run || ret=1
test_end

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
