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

#shellcheck source=conf.sh
SYSTEMTESTTOP=..
. "$SYSTEMTESTTOP/conf.sh"

dig_with_opts() (
	"$DIG" -p "$PORT" "$@"
)

sendcmd() (
	"$PERL" ../send.pl 10.53.0.6 "$EXTRAPORT1"
)

rndccmd() {
    "$RNDC" -c ../common/rndc.conf -p "$CONTROLPORT" -s "$@"
}

root=10.53.0.1
hidden=10.53.0.2
f1=10.53.0.3
f2=10.53.0.4

status=0
n=0

n=$((n+1))
echo_i "checking that a forward zone overrides global forwarders ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example1. txt @$hidden > dig.out.$n.hidden || ret=1
dig_with_opts +noadd +noauth txt.example1. txt @$f1 > dig.out.$n.f1 || ret=1
digcomp dig.out.$n.hidden dig.out.$n.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that a forward first zone no forwarders recurses ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example2. txt @$root > dig.out.$n.root || ret=1
dig_with_opts +noadd +noauth txt.example2. txt @$f1 > dig.out.$n.f1 || ret=1
digcomp dig.out.$n.root dig.out.$n.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that a forward only zone no forwarders fails ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example2. txt @$root > dig.out.$n.root || ret=1
dig_with_opts +noadd +noauth txt.example2. txt @$f1 > dig.out.$n.f1 || ret=1
digcomp dig.out.$n.root dig.out.$n.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that global forwarders work ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example4. txt @$hidden > dig.out.$n.hidden || ret=1
dig_with_opts +noadd +noauth txt.example4. txt @$f1 > dig.out.$n.f1 || ret=1
digcomp dig.out.$n.hidden dig.out.$n.f1 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that a forward zone works ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example1. txt @$hidden > dig.out.$n.hidden || ret=1
dig_with_opts +noadd +noauth txt.example1. txt @$f2 > dig.out.$n.f2 || ret=1
digcomp dig.out.$n.hidden dig.out.$n.f2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that forwarding doesn't spontaneously happen ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example2. txt @$root > dig.out.$n.root || ret=1
dig_with_opts +noadd +noauth txt.example2. txt @$f2 > dig.out.$n.f2 || ret=1
digcomp dig.out.$n.root dig.out.$n.f2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that a forward zone with no specified policy works ($n)"
ret=0
dig_with_opts +noadd +noauth txt.example3. txt @$hidden > dig.out.$n.hidden || ret=1
dig_with_opts +noadd +noauth txt.example3. txt @$f2 > dig.out.$n.f2 || ret=1
digcomp dig.out.$n.hidden dig.out.$n.f2 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that a forward only doesn't recurse ($n)"
ret=0
dig_with_opts txt.example5. txt @$f2 > dig.out.$n.f2 || ret=1
grep "SERVFAIL" dig.out.$n.f2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking for negative caching of forwarder response ($n)"
# prime the cache, shutdown the forwarder then check that we can
# get the answer from the cache.  restart forwarder.
ret=0
dig_with_opts nonexist. txt @10.53.0.5 > dig.out.$n.f2 || ret=1
grep "status: NXDOMAIN" dig.out.$n.f2 > /dev/null || ret=1
stop_server ns4 || ret=1
dig_with_opts nonexist. txt @10.53.0.5 > dig.out.$n.f2 || ret=1
grep "status: NXDOMAIN" dig.out.$n.f2 > /dev/null || ret=1
start_server --restart --noclean --port "${PORT}" ns4 || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

check_override() (
    dig_with_opts 1.0.10.in-addr.arpa TXT @10.53.0.4 > dig.out.$n.f2 &&
    grep "status: NOERROR" dig.out.$n.f2 > /dev/null &&
    dig_with_opts 2.0.10.in-addr.arpa TXT @10.53.0.4 > dig.out.$n.f2 &&
    grep "status: NXDOMAIN" dig.out.$n.f2 > /dev/null
)

n=$((n+1))
echo_i "checking that forward only zone overrides empty zone ($n)"
ret=0
# retry loop in case the server restart above causes transient failure
retry_quiet 10 check_override || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that DS lookups for grafting forward zones are isolated ($n)"
ret=0
dig_with_opts grafted A @10.53.0.4 > dig.out.$n.q1 || ret=1
dig_with_opts grafted DS @10.53.0.4 > dig.out.$n.q2 || ret=1
dig_with_opts grafted A @10.53.0.4 > dig.out.$n.q3 || ret=1
dig_with_opts grafted AAAA @10.53.0.4 > dig.out.$n.q4 || ret=1
grep "status: NOERROR" dig.out.$n.q1 > /dev/null || ret=1
grep "status: NXDOMAIN" dig.out.$n.q2 > /dev/null || ret=1
grep "status: NOERROR" dig.out.$n.q3 > /dev/null || ret=1
grep "status: NOERROR" dig.out.$n.q4 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that rfc1918 inherited 'forward first;' zones are warned about ($n)"
ret=0
$CHECKCONF rfc1918-inherited.conf | grep "forward first;" >/dev/null || ret=1
$CHECKCONF rfc1918-notinherited.conf | grep "forward first;" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that ULA inherited 'forward first;' zones are warned about ($n)"
ret=0
$CHECKCONF ula-inherited.conf | grep "forward first;" >/dev/null || ret=1
$CHECKCONF ula-notinherited.conf | grep "forward first;" >/dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

count_sent() (
	logfile="$1"
	start_pattern="$2"
	pattern="$3"
	nextpartpeek "$logfile" | tr -d '\r' | sed -n "/$start_pattern/,/^\$/p" | grep -c "$pattern"
)

check_sent() (
	expected="$1"
	shift
	count=$(count_sent "$@")
	[ "$expected" = "$count" ]
)

wait_for_log() (
	nextpartpeek "$1" | grep "$2" >/dev/null

)

n=$((n+1))
echo_i "checking that a forwarder timeout prevents it from being reused in the same fetch context ($n)"
ret=0
# Make ans6 receive queries without responding to them.
echo "//" | sendcmd
# Query for a record in a zone which is forwarded to a non-responding forwarder
# and is delegated from the root to check whether the forwarder will be retried
# when a delegation is encountered after falling back to full recursive
# resolution.
nextpart ns3/named.run >/dev/null
dig_with_opts txt.example7. txt @$f1 > dig.out.$n.f1 || ret=1
# The forwarder for the "example7" zone should only be queried once.
start_pattern="sending packet to 10\.53\.0\.6"
retry_quiet 5 wait_for_log ns3/named.run "$start_pattern"
check_sent 1 ns3/named.run "$start_pattern" ";txt\.example7\.[[:space:]]*IN[[:space:]]*TXT$" || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that priming queries are not forwarded ($n)"
ret=0
nextpart ns7/named.run >/dev/null
dig_with_opts +noadd +noauth txt.example1. txt @10.53.0.7 > dig.out.$n.f7 || ret=1
received_pattern="received packet from 10\.53\.0\.1"
start_pattern="sending packet to 10\.53\.0\.1"
retry_quiet 5 wait_for_log ns7/named.run "$received_pattern" || ret=1
check_sent 1 ns7/named.run "$start_pattern" ";\.[[:space:]]*IN[[:space:]]*NS$" || ret=1
sent=$(grep -c "10.53.0.7#.* (.): query '\./NS/IN' approved" ns4/named.run)
[ "$sent" -eq 0 ] || ret=1
sent=$(grep -c "10.53.0.7#.* (.): query '\./NS/IN' approved" ns1/named.run)
[ "$sent" -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking recovery from forwarding to a non-recursive server ($n)"
ret=0
dig_with_opts xxx.sld.tld txt @10.53.0.8  > dig.out.$n.f8 || ret=1
grep "status: NOERROR" dig.out.$n.f8 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking that rebinding protection works in forward only mode ($n)"
ret=0
# 10.53.0.5 will forward target.malicious. query to 10.53.0.4
# which in turn will return a CNAME for subdomain.rebind.
# to honor the option deny-answer-aliases { "rebind"; };
# ns5 should return a SERVFAIL to avoid potential rebinding attacks
dig_with_opts +noadd +noauth @10.53.0.5 target.malicious. > dig.out.$n || ret=1
grep "status: SERVFAIL" dig.out.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking switch from forwarding to normal resolution while chasing DS ($n)"
ret=0
copy_setports ns3/named2.conf.in ns3/named.conf
rndccmd 10.53.0.3 reconfig 2>&1 | sed 's/^/ns3 /' | cat_i
sleep 1
sendcmd << EOF
/ns1.sld.tld/A/
300 A 10.53.0.2
/sld.tld/NS/
300 NS ns1.sld.tld.
/sld.tld/
EOF
nextpart ns3/named.run >/dev/null
dig_with_opts @$f1 xxx.yyy.sld.tld ds > dig.out.$n.f1 || ret=1
grep "status: SERVFAIL" dig.out.$n.f1 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# Check various spoofed response scenarios. The same tests will be
# run twice, with "forward first" and "forward only" configurations.
#
run_spooftests () {
    n=$((n+1))
    echo_i "checking spoofed response scenario 1 - out of bailiwick NS ($n)"
    ret=0
    # prime
    dig_with_opts @10.53.0.9 attackSecureDomain.net > dig.out.$n.prime || ret=1
    # check 'net' is not poisoned.
    dig_with_opts @10.53.0.9 diditwork.net. TXT > dig.out.$n.net || ret=1
    grep '^diditwork\.net\..*TXT.*"recursed"' dig.out.$n.net > /dev/null || ret=1
    # check 'sub.local.net' is not poisoned.
    dig_with_opts @10.53.0.9 sub.local.net TXT > dig.out.$n.sub || ret=1
    grep '^sub\.local\.net\..*TXT.*"recursed"' dig.out.$n.sub > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking spoofed response scenario 2 - inject DNAME/net2. ($n)"
    ret=0
    # prime
    dig_with_opts @10.53.0.9 attackSecureDomain.net2 > dig.out.$n.prime || ret=1
    # check that net2/DNAME is not cached
    dig_with_opts @10.53.0.9 net2. DNAME > dig.out.$n.net2 || ret=1
    grep "ANSWER: 0," dig.out.$n.net2 > /dev/null || ret=1
    grep "status: NXDOMAIN" dig.out.$n.net2 > /dev/null || ret=1
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))

    n=$((n+1))
    echo_i "checking spoofed response scenario 3 - extra answer ($n)"
    ret=0
    # prime
    dig_with_opts @10.53.0.9 attackSecureDomain.net3 > dig.out.$n.prime || ret=1
    # check extra net3 records are not cached
    rndccmd 10.53.0.9 dumpdb -cache 2>&1 | sed 's/^/ns9 /' | cat_i
    for try in 1 2 3 4 5; do
        lines=$(grep "net3" ns9/named_dump.db | wc -l)
        if [ ${lines} -eq 0 ]; then
                sleep 1
                continue
        fi
        [ ${lines} -eq 1 ] || ret=1
        grep -q '^attackSecureDomain.net3' ns9/named_dump.db || ret=1
        grep -q '^local.net3' ns9/named_dump.db && ret=1
    done
    if [ $ret != 0 ]; then echo_i "failed"; fi
    status=$((status+ret))
}

echo_i "checking spoofed response scenarios with forward first zones"
run_spooftests

copy_setports ns9/named2.conf.in ns9/named.conf
rndccmd 10.53.0.9 reconfig 2>&1 | sed 's/^/ns3 /' | cat_i
rndccmd 10.53.0.9 flush 2>&1 | sed 's/^/ns3 /' | cat_i
sleep 1

echo_i "rechecking spoofed response scenarios with forward only zones"
run_spooftests

#
# This scenario expects the spoofed response to succeed. The tests are
# similar to the ones above, but not identical.
#
echo_i "rechecking spoofed response scenarios with 'forward only' set globally"
copy_setports ns9/named3.conf.in ns9/named.conf
rndccmd 10.53.0.9 reconfig 2>&1 | sed 's/^/ns3 /' | cat_i
rndccmd 10.53.0.9 flush 2>&1 | sed 's/^/ns3 /' | cat_i
sleep 1

n=$((n+1))
echo_i "checking spoofed response scenario 1 - out of bailiwick NS ($n)"
ret=0
# prime
dig_with_opts @10.53.0.9 attackSecureDomain.net > dig.out.$n.prime || ret=1
# check 'net' is poisoned.
dig_with_opts @10.53.0.9 diditwork.net. TXT > dig.out.$n.net || ret=1
grep '^didItWork\.net\..*TXT.*"if you can see this record the attack worked"' dig.out.$n.net > /dev/null || ret=1
# check 'sub.local.net' is poisoned.
dig_with_opts @10.53.0.9 sub.local.net TXT > dig.out.$n.sub || ret=1
grep '^sub\.local\.net\..*TXT.*"if you see this attacker overrode local delegation"' dig.out.$n.sub > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

n=$((n+1))
echo_i "checking spoofed response scenario 2 - inject DNAME/net2. ($n)"
ret=0
# prime
dig_with_opts @10.53.0.9 attackSecureDomain.net2 > dig.out.$n.prime || ret=1
# check that net2/DNAME is cached
dig_with_opts @10.53.0.9 net2. DNAME > dig.out.$n.net2 || ret=1
grep "ANSWER: 1," dig.out.$n.net2 > /dev/null || ret=1
grep "net2\..*IN.DNAME.net\.example\.lll\." dig.out.$n.net2 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

#
# This test doesn't use any forwarder clauses but is here because it
# is similar to forwarders, as the set of servers that can populate
# the namespace is defined by the zone content.
#
echo_i "rechecking spoofed response scenarios glue below local zone"
copy_setports ns9/named4.conf.in ns9/named.conf
rndccmd 10.53.0.9 reconfig 2>&1 | sed 's/^/ns3 /' | cat_i
rndccmd 10.53.0.9 flush 2>&1 | sed 's/^/ns3 /' | cat_i
sleep 1

n=$((n+1))
echo_i "checking sibling glue below zone ($n)"
ret=0
# prime
dig_with_opts @10.53.0.9 sibling.tld > dig.out.$n.prime || ret=1
# check for glue A record for sub.local.tld is not used
dig_with_opts @10.53.0.9 sub.local.tld TXT > dig.out.$n.sub || ret=1
grep "ANSWER: 1," dig.out.$n.sub > /dev/null || ret=1
grep 'sub\.local\.tld\..*IN.TXT."good"$' dig.out.$n.sub > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
