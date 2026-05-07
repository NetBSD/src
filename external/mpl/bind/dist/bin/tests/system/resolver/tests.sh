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

rndccmd() {
  "${RNDC}" -c ../_common/rndc.conf -p "${CONTROLPORT}" -s "${@}"
}

status=0
n=0

n=$((n + 1))
echo_i "checking non-cachable NXDOMAIN response handling ($n)"
ret=0
dig_with_opts +tcp nxdomain.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking non-cachable NODATA response handling ($n)"
ret=0
dig_with_opts +tcp nodata.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

rndccmd 10.53.0.1 stats || ret=1 # Get the responses, RTT and timeout statistics before the following timeout tests
grep -F 'responses received' ns1/named.stats >ns1/named.stats.responses-before || true
grep -F 'queries with RTT' ns1/named.stats >ns1/named.stats.rtt-before || true
mv ns1/named.stats ns1/named.stats-before

# 'resolver-query-timeout' is set to 5 seconds in ns1, so dig with a lower
# timeout value should give up earlier than that.
n=$((n + 1))
echo_i "checking no response handling with a shorter than resolver-query-timeout timeout ($n)"
ret=0
dig_with_opts +tcp +tries=1 +timeout=3 noresponse.example.net @10.53.0.1 a >dig.out.ns1.test${n} && ret=1
grep -F "no servers could be reached" dig.out.ns1.test${n} >/dev/null || ret=1
grep -F "EDE: 22 (No Reachable Authority)" dig.out.ns1.test${n} >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# 'resolver-query-timeout' is set to 5 seconds in ns1, which is lower than the
# current single query timeout value MAX_SINGLE_QUERY_TIMEOUT of 9 seconds, so
# the "hung fetch" timer should kick in, interrupt the non-responsive query and
# send a SERVFAIL answer.
n=$((n + 1))
echo_i "checking no response handling with a longer than resolver-query-timeout timeout ($n)"
ret=0
dig_with_opts +tcp +tries=1 +timeout=7 noresponse.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep -F "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
grep -F "EDE: 22 (No Reachable Authority)" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that the timeout didn't skew the resolver responses counters ($n)"
ret=0
rndccmd 10.53.0.1 stats || ret=1
grep -F 'responses received' ns1/named.stats >ns1/named.stats.responses-after || true
grep -F 'queries with RTT' ns1/named.stats >ns1/named.stats.rtt-after || true
mv ns1/named.stats ns1/named.stats-after
diff ns1/named.stats.responses-before ns1/named.stats.responses-after >/dev/null || ret=1
diff ns1/named.stats.rtt-before ns1/named.stats.rtt-after >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

# 'resolver-query-timeout' is set to 5 seconds in ns1, so named should
# interrupt the non-responsive query and send a SERVFAIL answer before dig's
# own timeout fires, which is set to 7 seconds. This time, exampleudp.net is
# contacted using UDP transport by the resolver.
n=$((n + 1))
echo_i "checking no response handling with a longer than resolver-query-timeout timeout (UDP recursion) ($n)"
ret=0
dig_with_opts +tcp +tries=1 +timeout=7 noresponse.exampleudp.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep -F "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
grep -F "EDE: 22 (No Reachable Authority)" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking handling of bogus referrals ($n)"
# If the server has the "INSIST(!external)" bug, this query will kill it.
dig_with_opts +tcp www.example.com. a @10.53.0.1 >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "check handling of cname + other data / 1 ($n)"
dig_with_opts +tcp cname1.example.com. a @10.53.0.1 >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "check handling of cname + other data / 2 ($n)"
dig_with_opts +tcp cname2.example.com. a @10.53.0.1 >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "check that server is still running ($n)"
dig_with_opts +tcp www.example.com. a @10.53.0.1 >/dev/null || {
  echo_i "failed"
  status=$((status + 1))
}

n=$((n + 1))
echo_i "checking answer IPv4 address filtering (deny) ($n)"
ret=0
dig_with_opts +tcp www.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking answer IPv6 address filtering (deny) ($n)"
ret=0
dig_with_opts +tcp www.example.net @10.53.0.1 aaaa >dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking answer IPv4 address filtering (accept) ($n)"
ret=0
dig_with_opts +tcp www.example.org @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking answer IPv6 address filtering (accept) ($n)"
ret=0
dig_with_opts +tcp www.example.org @10.53.0.1 aaaa >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking CNAME target filtering (deny) ($n)"
ret=0
dig_with_opts +tcp badcname.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking CNAME target filtering (accept) ($n)"
ret=0
dig_with_opts +tcp goodcname.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking long CNAME chain target filtering (deny) ($n)"
ret=0
dig_with_opts +tcp longcname1.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep -F "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
grep -F "max. restarts reached" dig.out.ns1.test${n} >/dev/null || ret=1
lines=$(grep -F "CNAME" dig.out.ns1.test${n} | wc -l)
test ${lines:-1} -eq 12 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DNAME target filtering (deny) ($n)"
ret=0
dig_with_opts +tcp foo.baddname.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "DNAME target foo.baddname.example.org denied for foo.baddname.example.net/IN" ns1/named.run >/dev/null || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DNAME target filtering (accept) ($n)"
ret=0
dig_with_opts +tcp foo.gooddname.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking DNAME target filtering (accept due to subdomain) ($n)"
ret=0
dig_with_opts +tcp www.dname.sub.example.org @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that the resolver accepts a referral response with a non-empty ANSWER section ($n)"
ret=0
dig_with_opts @10.53.0.1 foo.glue-in-answer.example.org. A >dig.ns1.out.${n} || ret=1
grep "status: NOERROR" dig.ns1.out.${n} >/dev/null || ret=1
grep "foo.glue-in-answer.example.org.*192.0.2.1" dig.ns1.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that the resolver limits the number of NS records it follows in a referral response ($n)"
# ns5 is the recusor being tested.  ns4 holds the sourcens zone containing
# names with varying numbers of NS records pointing to non-existent
# nameservers in the targetns zone on ns6.
ret=0
rndccmd 10.53.0.5 flush || ret=1 # Ensure cache is empty before doing this test
count_fetches() {
  actual=$(nextpartpeek ns5/named.run | grep -c " fetch: ns.fake${nscount}")
  [ "${actual:-0}" -eq "${expected}" ] || return 1
  return 0
}
for nscount in 1 2 3 4 5 6 7 8 9 10; do
  # Verify number of NS records at source server
  dig_with_opts +norecurse @10.53.0.4 target${nscount}.sourcens ns >dig.ns4.out.${nscount}.${n}
  sourcerecs=$(grep NS dig.ns4.out.${nscount}.${n} | grep -cv ';')
  test "${sourcerecs}" -eq "${nscount}" || ret=1
  test "${sourcerecs}" -eq "${nscount}" || echo_i "NS count incorrect for target${nscount}.sourcens"

  # Expected queries = 2 * number of NS records allowed at the first level, up to a maximum of 6.
  expected=$((nscount * 2))
  if [ "$expected" -gt 6 ]; then expected=6; fi
  # Count the number of logged fetches
  nextpart ns5/named.run >/dev/null
  dig_with_opts @10.53.0.5 target${nscount}.sourcens A >dig.ns5.out.${nscount}.${n} || ret=1
  retry_quiet 5 count_fetches ns5/named.run $nscount $expected || {
    echo_i "query count error: $nscount NS records: expected queries $expected, actual $actual"
    ret=1
  }
done
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

stop_server ns4
touch ns4/named.noaa
start_server --noclean --restart --port ${PORT} ns4 || ret=1

n=$((n + 1))
echo_i "RT21594 regression test check setup ($n)"
ret=0
# Check that "aa" is not being set by the authoritative server.
dig_with_opts +tcp . @10.53.0.4 soa >dig.ns4.out.${n} || ret=1
grep 'flags: qr rd;' dig.ns4.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "RT21594 regression test positive answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative positive answers.
dig_with_opts +tcp . @10.53.0.5 soa >dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "RT21594 regression test NODATA answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative nodata answers.
dig_with_opts +tcp . @10.53.0.5 txt >dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "RT21594 regression test NXDOMAIN answers ($n)"
ret=0
# Check that resolver accepts the non-authoritative positive answers.
dig_with_opts +tcp noexistent @10.53.0.5 txt >dig.ns5.out.${n} || ret=1
grep "status: NXDOMAIN" dig.ns5.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

stop_server ns4
rm ns4/named.noaa
start_server --noclean --restart --port ${PORT} ns4 || ret=1

n=$((n + 1))
echo_i "check that replacement of additional data by a negative cache no data entry clears the additional RRSIGs ($n)"
ret=0
dig_with_opts +tcp mx example.net @10.53.0.7 >dig.ns7.out.${n} || ret=1
grep "status: NOERROR" dig.ns7.out.${n} >/dev/null || ret=1
if [ $ret = 1 ]; then echo_i "mx priming failed"; fi
$NSUPDATE <<EOF
server 10.53.0.6 ${PORT}
zone example.net
update delete mail.example.net A
update add mail.example.net 0 AAAA ::1
send
EOF
dig_with_opts +tcp a mail.example.net @10.53.0.7 >dig.ns7.out.${n} || ret=2
grep "status: NOERROR" dig.ns7.out.${n} >/dev/null || ret=2
grep "ANSWER: 0" dig.ns7.out.${n} >/dev/null || ret=2
if [ $ret = 2 ]; then echo_i "ncache priming failed"; fi
dig_with_opts +tcp mx example.net @10.53.0.7 >dig.ns7.out.${n} || ret=3
grep "status: NOERROR" dig.ns7.out.${n} >/dev/null || ret=3
dig_with_opts +tcp rrsig mail.example.net +norec @10.53.0.7 >dig.ns7.out.${n} || ret=4
grep "status: NOERROR" dig.ns7.out.${n} >/dev/null || ret=4
grep "ANSWER: 0" dig.ns7.out.${n} >/dev/null || ret=4
if [ $ret != 0 ]; then
  echo_i "failed"
  ret=1
fi
status=$((status + ret))

if [ $ret != 0 ]; then
  echo_i "failed"
  ret=1
fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking that update a nameservers address has immediate effects ($n)"
ret=0
dig_with_opts +tcp TXT foo.moves @10.53.0.7 >dig.ns7.foo.${n} || ret=1
grep "From NS 5" dig.ns7.foo.${n} >/dev/null || ret=1
$NSUPDATE <<EOF
server 10.53.0.7 ${PORT}
zone server
update delete ns.server A
update add ns.server 300 A 10.53.0.4
send
EOF
sleep 1
dig_with_opts +tcp TXT bar.moves @10.53.0.7 >dig.ns7.bar.${n} || ret=1
grep "From NS 4" dig.ns7.bar.${n} >/dev/null || ret=1

if [ $ret != 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "checking that update a nameservers glue has immediate effects ($n)"
ret=0
dig_with_opts +tcp TXT foo.child.server @10.53.0.7 >dig.ns7.foo.${n} || ret=1
grep "From NS 5" dig.ns7.foo.${n} >/dev/null || ret=1
$NSUPDATE <<EOF
server 10.53.0.7 ${PORT}
zone server
update delete ns.child.server A
update add ns.child.server 300 A 10.53.0.4
send
EOF
sleep 1
dig_with_opts +tcp TXT bar.child.server @10.53.0.7 >dig.ns7.bar.${n} || ret=1
grep "From NS 4" dig.ns7.bar.${n} >/dev/null || ret=1

if [ $ret != 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "checking empty RFC 1918 reverse zones ($n)"
ret=0
# Check that "aa" is being set by the resolver for RFC 1918 zones
# except the one that has been deliberately disabled
dig_with_opts @10.53.0.7 -x 10.1.1.1 >dig.ns4.out.1.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.1.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 192.168.1.1 >dig.ns4.out.2.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.2.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.16.1.1 >dig.ns4.out.3.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.3.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.17.1.1 >dig.ns4.out.4.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.4.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.18.1.1 >dig.ns4.out.5.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.5.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.19.1.1 >dig.ns4.out.6.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.6.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.21.1.1 >dig.ns4.out.7.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.7.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.22.1.1 >dig.ns4.out.8.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.8.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.23.1.1 >dig.ns4.out.9.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.9.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.24.1.1 >dig.ns4.out.11.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.11.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.25.1.1 >dig.ns4.out.12.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.12.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.26.1.1 >dig.ns4.out.13.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.13.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.27.1.1 >dig.ns4.out.14.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.14.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.28.1.1 >dig.ns4.out.15.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.15.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.29.1.1 >dig.ns4.out.16.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.16.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.30.1.1 >dig.ns4.out.17.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.17.${n} >/dev/null || ret=1
dig_with_opts @10.53.0.7 -x 172.31.1.1 >dig.ns4.out.18.${n} || ret=1
grep 'flags: qr aa rd ra;' dig.ns4.out.18.${n} >/dev/null || ret=1
# but this one should NOT be authoritative
dig_with_opts @10.53.0.7 -x 172.20.1.1 >dig.ns4.out.19.${n} || ret=1
grep 'flags: qr rd ra;' dig.ns4.out.19.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "checking that removal of a delegation is honoured ($n)"
ret=0
dig_with_opts @10.53.0.5 www.to-be-removed.tld A >dig.ns5.prime.${n}
grep "status: NOERROR" dig.ns5.prime.${n} >/dev/null || {
  ret=1
  echo_i "priming failed"
}
cp ns4/tld2.db ns4/tld.db
rndc_reload ns4 10.53.0.4 tld
old=
for i in 0 1 2 3 4 5 6 7 8 9; do
  foo=0
  dig_with_opts @10.53.0.5 ns$i.to-be-removed.tld A >/dev/null
  dig_with_opts @10.53.0.5 www.to-be-removed.tld A >dig.ns5.out.${n}
  grep "status: NXDOMAIN" dig.ns5.out.${n} >/dev/null || foo=1
  [ $foo = 0 ] && break
  $NSUPDATE <<EOF
server 10.53.0.6 ${PORT}
zone to-be-removed.tld
update add to-be-removed.tld 100 NS ns${i}.to-be-removed.tld
update delete to-be-removed.tld NS ns${old}.to-be-removed.tld
send
EOF
  old=$i
  sleep 1
done
[ $ret = 0 ] && ret=$foo
if [ $ret != 0 ]; then
  echo_i "failed"
  status=1
fi

n=$((n + 1))
echo_i "check for improved error message with SOA mismatch ($n)"
ret=0
dig_with_opts @10.53.0.1 www.sub.broken aaaa >dig.out.ns1.test${n} || ret=1
grep "not subdomain of zone" ns1/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

cp ns7/named2.conf ns7/named.conf
rndccmd 10.53.0.7 reconfig 2>&1 | sed 's/^/ns7 /' | cat_i

n=$((n + 1))
echo_i "check resolution on the listening port ($n)"
ret=0
dig_with_opts +tcp +tries=2 +time=5 mx example.net @10.53.0.7 >dig.ns7.out.${n} || ret=2
grep "status: NOERROR" dig.ns7.out.${n} >/dev/null || ret=1
grep "ANSWER: 1" dig.ns7.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then
  echo_i "failed"
  ret=1
fi
status=$((status + ret))

n=$((n + 1))
echo_i "check prefetch (${n})"
ret=0
# read prefetch value from config.
PREFETCH=$(sed -n "s/[[:space:]]*prefetch \([0-9]\).*/\1/p" ns5/named.conf)
dig_with_opts @10.53.0.5 fetch.tld txt >dig.out.1.${n} || ret=1
ttl1=$(awk '/"A" "short" "ttl"/ { print $2 }' dig.out.1.${n})
interval=$((ttl1 - PREFETCH + 1))
# sleep so we are in prefetch range
sleep ${interval:-0}
# trigger prefetch
dig_with_opts @10.53.0.5 fetch.tld txt >dig.out.2.${n} || ret=1
ttl2=$(awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n})
sleep 1
# check that prefetch occurred
dig_with_opts @10.53.0.5 fetch.tld txt >dig.out.3.${n} || ret=1
ttl=$(awk '/"A" "short" "ttl"/ { print $2 }' dig.out.3.${n})
test "${ttl:-0}" -gt "${ttl2:-1}" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check prefetch of validated DS's RRSIG TTL is updated (${n})"
ret=0
dig_with_opts +dnssec @10.53.0.5 ds.example.net ds >dig.out.1.${n} || ret=1
dsttl1=$(awk '$4 == "DS" && $7 == "2" { print $2 }' dig.out.1.${n})
interval=$((dsttl1 - PREFETCH + 1))
# sleep so we are in prefetch range
sleep ${interval:-0}
# trigger prefetch
dig_with_opts @10.53.0.5 ds.example.net ds >dig.out.2.${n} || ret=1
dsttl2=$(awk '$4 == "DS" && $7 == "2" { print $2 }' dig.out.2.${n})
sleep 1
# check that prefetch occurred
dig_with_opts @10.53.0.5 ds.example.net ds +dnssec >dig.out.3.${n} || ret=1
dsttl=$(awk '$4 == "DS" && $7 == "2" { print $2 }' dig.out.3.${n})
sigttl=$(awk '$4 == "RRSIG" && $5 == "DS" { print $2 }' dig.out.3.${n})
test "${dsttl:-0}" -gt "${dsttl2:-1}" || ret=1
test "${sigttl:-0}" -gt "${dsttl2:-1}" || ret=1
test "${dsttl:-0}" -eq "${sigttl:-1}" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check prefetch disabled (${n})"
ret=0
dig_with_opts @10.53.0.7 fetch.example.net txt >dig.out.1.${n} || ret=1
ttl1=$(awk '/"A" "short" "ttl"/ { print $2 }' dig.out.1.${n})
interval=$((ttl1 - PREFETCH + 1))
# sleep so we are in expire range
sleep ${interval:-0}
tmp_ttl=$ttl1
no_prefetch() {
  # fetch record and ensure its ttl is in range 0 < ttl < tmp_ttl.
  # since prefetch is disabled, updated ttl must be a lower value than
  # the previous one.
  dig_with_opts @10.53.0.7 fetch.example.net txt >dig.out.2.${n} || return 1
  ttl2=$(awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n})
  # check that prefetch has not occurred
  if [ "$ttl2" -ge "${tmp_ttl}" ]; then
    return 1
  fi
  tmp_ttl=$ttl2
}
retry_quiet 3 no_prefetch || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check prefetch qtype * (${n})"
ret=0
dig_with_opts @10.53.0.5 fetchall.tld any >dig.out.1.${n} || ret=1
ttl1=$(awk '/^fetchall.tld/ { print $2 - 3; exit }' dig.out.1.${n})
# sleep so we are in prefetch range
sleep "${ttl1:-0}"
# trigger prefetch
dig_with_opts @10.53.0.5 fetchall.tld any >dig.out.2.${n} || ret=1
ttl2=$(awk '/^fetchall.tld/ { print $2; exit }' dig.out.2.${n})
sleep 1
# check that prefetch occurred;
# note that only the first record is prefetched,
# because of the order of the records in the cache
dig_with_opts @10.53.0.5 fetchall.tld any >dig.out.3.${n} || ret=1
ttl3=$(awk '/^fetchall.tld/ { print $2; exit }' dig.out.3.${n})
test "${ttl3:-0}" -gt "${ttl2:-1}" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that E was logged on EDNS queries in the query log (${n})"
ret=0
dig_with_opts @10.53.0.5 +edns edns.fetchall.tld any >dig.out.2.${n} || ret=1
grep "query: edns.fetchall.tld IN ANY +E" ns5/named.run >/dev/null || ret=1
dig_with_opts @10.53.0.5 +noedns noedns.fetchall.tld any >dig.out.2.${n} || ret=1
grep "query: noedns.fetchall.tld IN ANY" ns5/named.run >/dev/null || ret=1
grep "query: noedns.fetchall.tld IN ANY +E" ns5/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that '-t aaaa' in .digrc does not have unexpected side effects ($n)"
ret=0
echo "-t aaaa" >.digrc
(
  HOME="$(pwd)"
  export HOME
  dig_with_opts @10.53.0.4 . >dig.out.1.${n}
) || ret=1
(
  HOME="$(pwd)"
  export HOME
  dig_with_opts @10.53.0.4 . A >dig.out.2.${n}
) || ret=1
(
  HOME="$(pwd)"
  export HOME
  dig_with_opts @10.53.0.4 -x 127.0.0.1 >dig.out.3.${n}
) || ret=1
grep ';\..*IN.*AAAA$' dig.out.1.${n} >/dev/null || ret=1
grep ';\..*IN.*A$' dig.out.2.${n} >/dev/null || ret=1
grep 'extra type option' dig.out.2.${n} >/dev/null && ret=1
grep ';1\.0\.0\.127\.in-addr\.arpa\..*IN.*PTR$' dig.out.3.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

edns=$($FEATURETEST --edns-version)

n=$((n + 1))
echo_i "check that EDNS version is logged (${n})"
ret=0
dig_with_opts @10.53.0.5 +edns edns0.fetchall.tld any >dig.out.2.${n} || ret=1
grep "query: edns0.fetchall.tld IN ANY +E(0)" ns5/named.run >/dev/null || ret=1
if test "${edns:-0}" != 0; then
  dig_with_opts @10.53.0.5 +edns=1 edns1.fetchall.tld any >dig.out.2.${n} || ret=1
  grep "query: edns1.fetchall.tld IN ANY +E(1)" ns5/named.run >/dev/null || ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if test "${edns:-0}" != 0; then
  n=$((n + 1))
  echo_i "check that edns-version is honoured (${n})"
  ret=0
  dig_with_opts @10.53.0.5 +edns no-edns-version.tld >dig.out.1.${n} || ret=1
  grep "query: no-edns-version.tld IN A -E(1)" ns6/named.run >/dev/null || ret=1
  dig_with_opts @10.53.0.5 +edns edns-version.tld >dig.out.2.${n} || ret=1
  grep "query: edns-version.tld IN A -E(0)" ns7/named.run >/dev/null || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

n=$((n + 1))
echo_i "check that CNAME nameserver is logged correctly (${n})"
ret=0
dig_with_opts soa all-cnames @10.53.0.5 >dig.out.ns5.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns5.test${n} >/dev/null || ret=1
grep "skipping nameserver 'cname.tld' because it is a CNAME, while resolving 'all-cnames/SOA'" ns5/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that unexpected opcodes are handled correctly (${n})"
ret=0
dig_with_opts soa all-cnames @10.53.0.5 +opcode=15 +cd +rec +ad +zflag >dig.out.ns5.test${n} || ret=1
grep "status: NOTIMP" dig.out.ns5.test${n} >/dev/null || ret=1
grep "flags:[^;]* qr[; ]" dig.out.ns5.test${n} >/dev/null || ret=1
grep "flags:[^;]* ra[; ]" dig.out.ns5.test${n} >/dev/null && ret=1
grep "flags:[^;]* rd[; ]" dig.out.ns5.test${n} >/dev/null && ret=1
grep "flags:[^;]* cd[; ]" dig.out.ns5.test${n} >/dev/null && ret=1
grep "flags:[^;]* ad[; ]" dig.out.ns5.test${n} >/dev/null && ret=1
grep "flags:[^;]*; MBZ: " dig.out.ns5.test${n} >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that EDNS client subnet with non-zeroed bits is handled correctly (${n})"
ret=0
# 0001 (IPv4) 1f (31 significant bits) 00 (0) ffffffff (255.255.255.255)
dig_with_opts soa . @10.53.0.5 +ednsopt=8:00011f00ffffffff >dig.out.ns5.test${n} || ret=1
grep "status: FORMERR" dig.out.ns5.test${n} >/dev/null || ret=1
grep "; EDNS: version:" dig.out.ns5.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that dig +subnet zeros address bits correctly (${n})"
ret=0
dig_with_opts soa . @10.53.0.5 +subnet=255.255.255.255/23 >dig.out.ns5.test${n} || ret=1
grep "status: NOERROR" dig.out.ns5.test${n} >/dev/null || ret=1
grep "CLIENT-SUBNET: 255.255.254.0/23/0" dig.out.ns5.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check zero ttl not returned for learnt non zero ttl records (${n})"
ret=0
# use prefetch disabled server
dig_with_opts @10.53.0.7 non-zero.example.net txt >dig.out.1.${n} || ret=1
ttl1=$(awk '/"A" "short" "ttl"/ { print $2 - 2 }' dig.out.1.${n})
# sleep so we are in expire range
sleep "${ttl1:-0}"
# look for ttl = 1, allow for one miss at getting zero ttl
zerotonine="0 1 2 3 4 5 6 7 8 9"
zerotonine="$zerotonine $zerotonine $zerotonine"
for i in $zerotonine $zerotonine $zerotonine $zerotonine; do
  dig_with_opts @10.53.0.7 non-zero.example.net txt >dig.out.2.${n} || ret=1
  ttl2=$(awk '/"A" "short" "ttl"/ { print $2 }' dig.out.2.${n})
  test "${ttl2:-1}" -eq 0 && break
  test "${ttl2:-1}" -ge "${ttl1:-0}" && break
  "${PERL}" -e 'select(undef, undef, undef, 0.05);'
done
test "${ttl2:-1}" -eq 0 && ret=1
test "${ttl2:-1}" -ge "${ttl1:-0}" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check zero ttl is returned for learnt zero ttl records (${n})"
ret=0
dig_with_opts @10.53.0.7 zero.example.net txt >dig.out.1.${n} || ret=1
ttl=$(awk '/"A" "zero" "ttl"/ { print $2 }' dig.out.1.${n})
test "${ttl:-1}" -eq 0 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'ad' in not returned in truncated answer with empty answer and authority sections to request with +ad (${n})"
ret=0
dig_with_opts @10.53.0.6 dnskey ds.example.net +bufsize=512 +ad +nodnssec +ignore +norec >dig.out.$n
grep "flags: qr aa tc; QUERY: 1, ANSWER: 0, AUTHORITY: 0" dig.out.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'ad' in not returned in truncated answer with empty answer and authority sections to request with +dnssec (${n})"
ret=0
dig_with_opts @10.53.0.6 dnskey ds.example.net +bufsize=512 +noad +dnssec +ignore +norec >dig.out.$n
grep "flags: qr aa tc; QUERY: 1, ANSWER: 0, AUTHORITY: 0" dig.out.$n >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that the resolver accepts a reply with empty question section with TC=1 and retries over TCP ($n)"
ret=0
dig_with_opts @10.53.0.5 truncated.no-questions. a +tries=3 +time=4 >dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} >/dev/null || ret=1
grep "ANSWER: 1," dig.ns5.out.${n} >/dev/null || ret=1
grep "1\.2\.3\.4" dig.ns5.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that the resolver rejects a reply with empty question section with TC=0 ($n)"
ret=0
dig_with_opts @10.53.0.5 not-truncated.no-questions. a +tries=3 +time=4 >dig.ns5.out.${n} || ret=1
grep "status: NOERROR" dig.ns5.out.${n} >/dev/null && ret=1
grep "ANSWER: 1," dig.ns5.out.${n} >/dev/null && ret=1
grep "1\.2\.3\.4" dig.ns5.out.${n} >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

if ${FEATURETEST} --enable-querytrace; then
  n=$((n + 1))
  echo_i "check that SERVFAIL is returned for an empty question section via TCP ($n)"
  ret=0
  nextpart ns5/named.run >/dev/null
  # bind to local address so that addresses in log messages are consistent
  # between platforms
  dig_with_opts @10.53.0.5 -b 10.53.0.5 tcpalso.no-questions. a +tries=2 +timeout=15 >dig.ns5.out.${n} || ret=1
  grep "status: SERVFAIL" dig.ns5.out.${n} >/dev/null || ret=1
  check_namedrun() {
    nextpartpeek ns5/named.run >nextpart.out.${n}
    grep 'resolving tcpalso.no-questions/A for [^:]*: empty question section, accepting it anyway as TC=1' nextpart.out.${n} >/dev/null || return 1
    grep '(tcpalso.no-questions/A): connecting via TCP' nextpart.out.${n} >/dev/null || return 1
    grep 'resolving tcpalso.no-questions/A for [^:]*: empty question section$' nextpart.out.${n} >/dev/null || return 1
    grep '(tcpalso.no-questions/A): nextitem' nextpart.out.${n} >/dev/null || return 1
    return 0
  }
  retry_quiet 12 check_namedrun || ret=1
  if [ $ret != 0 ]; then echo_i "failed"; fi
  status=$((status + ret))
fi

n=$((n + 1))
echo_i "checking SERVFAIL is returned when all authoritative servers return FORMERR ($n)"
ret=0
dig_with_opts @10.53.0.5 ns.formerr-to-all. a >dig.ns5.out.${n} || ret=1
grep "status: SERVFAIL" dig.ns5.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking SERVFAIL is not returned if only some authoritative servers return FORMERR ($n)"
ret=0
dig_with_opts @10.53.0.5 ns.partial-formerr. a >dig.ns5.out.${n} || ret=1
grep "status: SERVFAIL" dig.ns5.out.${n} >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check logged command line ($n)"
ret=0
grep "running as: .* -m record " ns1/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking NXDOMAIN is returned when querying non existing domain in CH class ($n)"
ret=0
dig_with_opts @10.53.0.1 id.hostname txt ch >dig.ns1.out.${n} || ret=1
grep "status: NXDOMAIN" dig.ns1.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that the addition section for HTTPS is populated on initial query to a recursive server ($n)"
ret=0
dig_with_opts @10.53.0.7 www.example.net https >dig.out.ns7.${n} || ret=1
grep "status: NOERROR" dig.out.ns7.${n} >/dev/null || ret=1
grep "flags:[^;]* ra[ ;]" dig.out.ns7.${n} >/dev/null || ret=1
grep "ADDITIONAL: 2" dig.out.ns7.${n} >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns7.${n} >/dev/null || ret=1
grep "http-server\.example\.net\..*A.*10\.53\.0\.6" dig.out.ns7.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check HTTPS loop is handled properly ($n)"
ret=0
dig_with_opts @10.53.0.7 https-loop.example.net https >dig.out.ns7.${n} || ret=1
grep "status: NOERROR" dig.out.ns7.${n} >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns7.${n} >/dev/null || ret=1
grep "ADDITIONAL: 2" dig.out.ns7.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check HTTPS -> CNAME loop is handled properly ($n)"
ret=0
dig_with_opts @10.53.0.7 https-cname-loop.example.net https >dig.out.ns7.${n} || ret=1
grep "status: NOERROR" dig.out.ns7.${n} >/dev/null || ret=1
grep "ADDITIONAL: 2" dig.out.ns7.${n} >/dev/null || ret=1
grep "ANSWER: 1," dig.out.ns7.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check HTTPS cname chains are followed ($n)"
ret=0
dig_with_opts @10.53.0.7 https-cname.example.net https >dig.out.ns7.${n} || ret=1
grep "status: NOERROR" dig.out.ns7.${n} >/dev/null || ret=1
grep "ADDITIONAL: 4" dig.out.ns7.${n} >/dev/null || ret=1
grep 'http-server\.example\.net\..*A.10\.53\.0\.6' dig.out.ns7.${n} >/dev/null || ret=1
grep 'cname-server\.example\.net\..*CNAME.cname-next\.example\.net\.' dig.out.ns7.${n} >/dev/null || ret=1
grep 'cname-next\.example\.net\..*CNAME.http-server\.example\.net\.' dig.out.ns7.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check ADB find loops are detected ($n)"
ret=0
dig_with_opts +tcp +tries=1 +timeout=5 @10.53.0.1 fake.lame.example.org >dig.out.ns1.${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check handling of large referrals to unresponsive name servers ($n)"
ret=0
dig_with_opts +timeout=15 large-referral.example.net @10.53.0.1 a >dig.out.ns1.test${n} || ret=1
grep "status: SERVFAIL" dig.out.ns1.test${n} >/dev/null || ret=1
# Check the total number of findname() calls triggered by a single query
# for large-referral.example.net/A.
findname_call_count="$(grep -c "large-referral\.example\.net.*FINDNAME" ns1/named.run || true)"
if [ "${findname_call_count}" -gt 1000 ]; then
  echo_i "failed: ${findname_call_count} (> 1000) findname() calls detected for large-referral.example.net"
  ret=1
fi
# Check whether the limit of NS RRs processed for any delegation
# encountered was not exceeded.
if grep -Eq "dns_adb_createfind: started (A|AAAA) fetch for name ns21.fake.redirect.com" ns1/named.run; then
  echo_i "failed: unexpected address fetch(es) were triggered for ns21.fake.redirect.com"
  ret=1
fi
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "checking query resolution for a domain with a valid glueless delegation chain ($n)"
ret=0
rndccmd 10.53.0.1 flush || ret=1
dig_with_opts foo.bar.sub.tld1 @10.53.0.1 TXT >dig.out.ns1.test${n} || ret=1
grep "status: NOERROR" dig.out.ns1.test${n} >/dev/null || ret=1
grep "IN.*TXT.*baz" dig.out.ns1.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that correct namespace is chosen for dual-stack-servers ($n)"
ret=0
dig_with_opts @fd92:7065:b8e:ffff::9 foo.v4only.net A >dig.out.ns9.${n} || ret=1
grep "status: NOERROR" dig.out.ns9.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check expired TTLs with qtype * (${n})"
ret=0
dig_with_opts +tcp @10.53.0.5 mixedttl.tld any >dig.out.1.${n} || ret=1
ttl1=$(awk '$1 == "mixedttl.tld." && $4 == "A" { print $2 + 1 }' dig.out.1.${n})
# sleep TTL + 1 so that record has expired
sleep "${ttl1:-0}"
dig_with_opts +tcp @10.53.0.5 mixedttl.tld any >dig.out.2.${n} || ret=1
# check preconditions
grep "ANSWER: 3," dig.out.1.${n} >/dev/null || ret=1
lines=$(awk '$1 == "mixedttl.tld." && $2 > 30 { print }' dig.out.1.${n} | wc -l)
test ${lines:-1} -ne 0 && ret=1
# check behaviour (there may be 1 answer on very slow machines)
grep "ANSWER: [12]," dig.out.2.${n} >/dev/null || ret=1
lines=$(awk '$1 == "mixedttl.tld." && $2 > 30 { print }' dig.out.2.${n} | wc -l)
test ${lines:-1} -ne 0 && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check resolver behavior when FORMERR for EDNS options happens (${n})"
ret=0
msg="resolving options-formerr/A .* server sent FORMERR with echoed DNS COOKIE"
if [ $ret != 0 ]; then echo_i "failed"; fi
nextpart ns5/named.run >/dev/null
dig_with_opts +tcp @10.53.0.5 options-formerr A >dig.out.${n} || ret=1
grep "status: NOERROR" dig.out.${n} >/dev/null || ret=1
nextpart ns5/named.run | grep "$msg" >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "GL#4612 regression test: DS query against broken NODATA responses (${n})"
# servers ns2 and ns3 return authority SOA which matches QNAME rather than the zone
ret=0
dig_with_opts @10.53.0.7 a.a.gl6412 DS >dig.out.${n} || ret=1
grep "status: SERVFAIL" dig.out.${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that response codes have been logged with 'responselog yes;' ($n)"
ret=0
grep "responselog yes;" ns5/named.conf >/dev/null || ret=1
grep "response: version.bind CH TXT NOERROR" ns5/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog off' disables logging 'responselog yes;' ($n)"
ret=0
rndccmd 10.53.0.5 responselog off || ret=1
dig_with_opts @10.53.0.5 should.not.be.logged >dig.ns5.out.${n} || ret=1
grep "response: should.not.be.logged" ns5/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog on' enables logging 'responselog yes;' ($n)"
ret=0
grep "response: should.be.logged" ns5/named.run >/dev/null && ret=1
rndccmd 10.53.0.5 responselog on || ret=1
dig_with_opts @10.53.0.5 should.be.logged >dig.ns5.out.${n} || ret=1
grep "response: should.be.logged" ns5/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that response codes have not been logged with default 'responselog' ($n)"
ret=0
grep "responselog" ns1/named.conf >/dev/null && ret=1
grep "response: version.bind CH TXT NOERROR" ns1/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog on' enables logging with default 'responselog' ($n)"
ret=0
grep "response: should.be.logged" ns1/named.run >/dev/null && ret=1
rndccmd 10.53.0.1 responselog on || ret=1
dig_with_opts @10.53.0.1 should.be.logged >dig.ns1.out.${n} || ret=1
grep "response: should.be.logged" ns1/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog off' disables logging with default 'responselog' ($n)"
ret=0
rndccmd 10.53.0.1 responselog off || ret=1
dig_with_opts @10.53.0.1 should.not.be.logged >dig.ns1.out.${n} || ret=1
grep "response: should.not.be.logged" ns1/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that response codes have not been logged with 'responselog no;' ($n)"
ret=0
grep "responselog no;" ns6/named.conf >/dev/null || ret=1
grep "response: version.bind CH TXT NOERROR" ns6/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog on' enables logging with default 'responselog no;' ($n)"
ret=0
grep "response: should.be.logged" ns6/named.run >/dev/null && ret=1
rndccmd 10.53.0.6 responselog on || ret=1
dig_with_opts @10.53.0.6 should.be.logged >dig.ns6.out.${n} || ret=1
grep "response: should.be.logged" ns6/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog' toggles logging off with default 'responselog no;' ($n)"
ret=0
rndccmd 10.53.0.6 responselog || ret=1
dig_with_opts @10.53.0.6 toggled.should.not.be.logged >dig.ns6.out.${n} || ret=1
grep "response: toggled.should.not.be.logged" ns6/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog' toggles logging on with default 'responselog no;' ($n)"
ret=0
rndccmd 10.53.0.6 responselog || ret=1
dig_with_opts @10.53.0.6 toggled.should.be.logged >dig.ns6.out.${n} || ret=1
grep "response: toggled.should.be.logged" ns6/named.run >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that 'rndc responselog off' disables logging with default 'responselog no;' ($n)"
ret=0
rndccmd 10.53.0.6 responselog off || ret=1
dig_with_opts @10.53.0.6 should.not.be.logged >dig.ns6.out.${n} || ret=1
grep "response: should.not.be.logged" ns6/named.run >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "check that attach-cache with arbitrary cache name is preserved across reload ($n)"
ret=0
# send a query, wait a second, reload the configuration, and query again.
# the TTL should indicate that the cache was still populated.
dig_with_opts +noall +answer @10.53.0.1 www.example.org >/dev/null || ret=1
sleep 1
rndc_reload ns1 10.53.0.1
dig_with_opts +noall +answer @10.53.0.1 www.example.org >dig.ns1.out.${n} || ret=1
ttl=$(awk '{print $2}' dig.ns1.out.${n})
[ $ttl -lt 300 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

n=$((n + 1))
echo_i "client requests recursion but it is disabled - expect EDE 20 code with REFUSED($n)"
ret=0
dig_with_opts +recurse www.isc.org @10.53.0.11 a >dig.out.ns11.test${n} || ret=1
grep "status: REFUSED" dig.out.ns11.test${n} >/dev/null || ret=1
grep -F "EDE: 20 (Not Authoritative)" dig.out.ns11.test${n} >/dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status + ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
