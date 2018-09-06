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

status=0
n=0

DIGOPTS="@10.53.0.1 -p ${PORT} +nocookie"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

newtest() {
	n=`expr $n + 1`
	echo_i "${1} (${n})"
	ret=0
}

test_update() {
    host="$1"
    type="$2"
    cmd="$3"
    digout="$4"
    should_fail="$5"

    cat <<EOF > ns1/update.txt
server 10.53.0.1 ${PORT}
update add $host $cmd
send
EOF

    newtest "testing update for $host $type $cmd${comment:+ }$comment"
    $NSUPDATE -k ns1/ddns.key ns1/update.txt > /dev/null 2>&1 || {
	[ "$should_fail" ] || \
             echo_i "update failed for $host $type $cmd"
	return 1
    }

    out=`$DIG $DIGOPTS -t $type -q $host | egrep "^$host"`
    lines=`echo "$out" | grep "$digout" | wc -l`
    [ $lines -eq 1 ] || {
	[ "$should_fail" ] || \
            echo_i "dig output incorrect for $host $type $cmd: $out"
	return 1
    }
    return 0
}

test_update testdc1.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || ret=1
status=`expr $status + $ret`

test_update testdc2.example.nil. A "86400 A 10.53.0.11" "10.53.0.11" || ret=1
status=`expr $status + $ret`

test_update testdc3.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || ret=1
status=`expr $status + $ret`

test_update deny.example.nil. TXT "86400 TXT helloworld" "helloworld" should_fail && ret=1
status=`expr $status + $ret`

newtest "testing nxrrset"
$DIG $DIGOPTS testdc1.example.nil AAAA > dig.out.$n
grep "status: NOERROR" dig.out.$n > /dev/null || ret=1
grep "ANSWER: 0" dig.out.$n > /dev/null || ret=1
status=`expr $status + $ret`

newtest "testing prerequisites are checked correctly"
cat > ns1/update.txt << EOF
server 10.53.0.1 ${PORT}
prereq nxdomain testdc3.example.nil
update add testdc3.example.nil 86500 in a 10.53.0.12
send
EOF
$NSUPDATE -k ns1/ddns.key ns1/update.txt > /dev/null 2>&1 && ret=1
out=`$DIG $DIGOPTS +short a testdc3.example.nil`
[ "$out" = "10.53.0.12" ] && ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing passing client info into DLZ driver"
out=`$DIG $DIGOPTS +short -t txt -q source-addr.example.nil | grep -v '^;'`
addr=`eval echo "$out" | cut -f1 -d'#'`
[ "$addr" = "10.53.0.1" ] || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing DLZ driver is cleaned up on reload"
$RNDCCMD 10.53.0.1 reload 2>&1 | sed 's/^/ns1 /' | cat_i
for i in 0 1 2 3 4 5 6 7 8 9; do
    ret=0
    grep 'dlz_example: shutting down zone example.nil' ns1/named.run > /dev/null 2>&1 || ret=1
    [ "$ret" -eq 0 ] && break
    sleep 1
done
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing multiple DLZ drivers"
test_update testdc1.alternate.nil. A "86400 A 10.53.0.10" "10.53.0.10" || ret=1
status=`expr $status + $ret`

newtest "testing AXFR from DLZ drivers"
$DIG $DIGOPTS +noall +answer axfr example.nil > dig.out.ns1.test$n
lines=`cat dig.out.ns1.test$n | wc -l`
[ ${lines:-0} -eq 4 ] || ret=1
$DIG $DIGOPTS +noall +answer axfr alternate.nil > dig.out.ns1.test$n
lines=`cat dig.out.ns1.test$n | wc -l`
[ ${lines:-0} -eq 5 ] || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing unsearched/unregistered DLZ zone is not found"
$DIG $DIGOPTS +noall +answer ns other.nil > dig.out.ns1.test$n
grep "3600.IN.NS.other.nil." dig.out.ns1.test$n > /dev/null && ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing unsearched/registered DLZ zone is found"
$DIG $DIGOPTS +noall +answer ns zone.nil > dig.out.ns1.test$n
grep "3600.IN.NS.zone.nil." dig.out.ns1.test$n > /dev/null || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing unsearched/registered DLZ zone is found"
$DIG $DIGOPTS +noall +answer ns zone.nil > dig.out.ns1.test$n
grep "3600.IN.NS.zone.nil." dig.out.ns1.test$n > /dev/null || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing correct behavior with findzone returning ISC_R_NOMORE"
$DIG $DIGOPTS +noall a test.example.com > /dev/null 2>&1 || ret=1
# we should only find one logged lookup per searched DLZ database
lines=`grep "dlz_findzonedb.*test\.example\.com.*example.nil" ns1/named.run | wc -l`
[ $lines -eq 1 ] || ret=1
lines=`grep "dlz_findzonedb.*test\.example\.com.*alternate.nil" ns1/named.run | wc -l`
[ $lines -eq 1 ] || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing findzone can return different results per client"
$DIG $DIGOPTS -b 10.53.0.1 +noall a test.example.net > /dev/null 2>&1 || ret=1
# we should only find one logged lookup per searched DLZ database
lines=`grep "dlz_findzonedb.*example\.net.*example.nil" ns1/named.run | wc -l`
[ $lines -eq 1 ] || ret=1
lines=`grep "dlz_findzonedb.*example\.net.*alternate.nil" ns1/named.run | wc -l`
[ $lines -eq 1 ] || ret=1
$DIG $DIGOPTS -b 10.53.0.2 +noall a test.example.net > /dev/null 2>&1 || ret=1
# we should find several logged lookups this time
lines=`grep "dlz_findzonedb.*example\.net.*example.nil" ns1/named.run | wc -l`
[ $lines -gt 2 ] || ret=1
lines=`grep "dlz_findzonedb.*example\.net.*alternate.nil" ns1/named.run | wc -l`
[ $lines -gt 2 ] || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing zone returning oversized data"
$DIG $DIGOPTS txt too-long.example.nil > dig.out.ns1.test$n 2>&1 || ret=1
grep "status: SERVFAIL" dig.out.ns1.test$n > /dev/null || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "testing zone returning oversized data at zone origin"
$DIG $DIGOPTS txt bigcname.domain > dig.out.ns1.test$n 2>&1 || ret=1
grep "status: SERVFAIL" dig.out.ns1.test$n > /dev/null || ret=1
[ "$ret" -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

newtest "checking redirected lookup for nonexistent name"
$DIG $DIGOPTS @10.53.0.1 unexists a > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "^unexists.*A.*100.100.100.2" dig.out.ns1.test$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "checking no redirected lookup for nonexistent type"
$DIG $DIGOPTS @10.53.0.1 exists aaaa > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "ANSWER: 0" dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

newtest "checking redirected lookup for a long nonexistent name"
$DIG $DIGOPTS @10.53.0.1 long.name.is.not.there a > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "^long.name.*A.*100.100.100.3" dig.out.ns1.test$n > /dev/null || ret=1
grep "flags:[^;]* aa[ ;]" dig.out.ns1.test$n > /dev/null || ret=1
lookups=`grep "lookup #.*\.not\.there" ns1/named.run | wc -l`
[ "$lookups" -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
