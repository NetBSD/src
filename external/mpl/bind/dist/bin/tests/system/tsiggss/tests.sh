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

# tests for TSIG-GSS updates

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=1

DIGOPTS="@10.53.0.1 -p ${PORT}"

test_update () {
    num="$1"
    host="$2"
    type="$3"
    cmd="$4"
    digout="$5"

    cat <<EOF > ns1/update.txt
server 10.53.0.1 ${PORT}
update add $host $cmd
send
answer
EOF
    echo_i "testing update for $host $type $cmd"
    $NSUPDATE -g -d ns1/update.txt > nsupdate.out${num} 2>&1 || {
	echo_i "update failed for $host $type $cmd"
	sed "s/^/I:/" nsupdate.out${num}
	return 1
    }

    # Verify that TKEY response is signed.
    tkeyout=`awk '/recvmsg reply from GSS-TSIG query/,/Sending update to/' nsupdate.out${num}`
    pattern="recvmsg reply from GSS-TSIG query .* opcode: QUERY, status: NOERROR, id: .* flags: qr; QUESTION: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1 ;; QUESTION SECTION: ;.* ANY TKEY ;; ANSWER SECTION: .* 0 ANY TKEY gss-tsig\. .* ;; TSIG PSEUDOSECTION: .* 0 ANY TSIG gss-tsig\. .* NOERROR 0"
    echo $tkeyout | grep "$pattern" > /dev/null || {
	echo_i "bad tkey response (not tsig signed)"
	return 1
    }

    # Weak verification that TKEY response is signed.
    grep -q "flags: qr; QUESTION: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1" nsupdate.out${num} || {
	echo_i "bad tkey response (not tsig signed)"
	return 1
    }

    out=`$DIG $DIGOPTS -t $type -q $host | grep -E "^${host}"`
    lines=`echo "$out" | grep "$digout" | wc -l`
    [ $lines -eq 1 ] || {
	echo_i "dig output incorrect for $host $type $cmd: $out"
	return 1
    }
    return 0
}


# Testing updates with good credentials.
KRB5CCNAME="FILE:"`pwd`/ns1/administrator.ccache
export KRB5CCNAME

echo_i "testing updates to testdc1 as administrator ($n)"
ret=0
test_update $n testdc1.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing updates to testdc2 as administrator ($n)"
ret=0
test_update $n testdc2.example.nil. A "86400 A 10.53.0.11" "10.53.0.11" || ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing updates to denied as administrator ($n)"
ret=0
test_update $n denied.example.nil. TXT "86400 TXT helloworld" "helloworld" > /dev/null && ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))


# Testing denied updates.
KRB5CCNAME="FILE:"`pwd`/ns1/testdenied.ccache
export KRB5CCNAME

echo_i "testing updates to denied (A) as a user ($n)"
ret=0
test_update $n testdenied.example.nil. A "86400 A 10.53.0.12" "10.53.0.12" > /dev/null && ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing updates to denied (TXT) as a user ($n)"
ret=0
test_update $n testdenied.example.nil. TXT "86400 TXT helloworld" "helloworld" || ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing external update policy (CNAME) ($n)"
ret=0
test_update $n testcname.example.nil. CNAME "86400 CNAME testdenied.example.nil" "testdenied" > /dev/null && ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing external update policy (CNAME) with auth sock ($n)"
ret=0
$PERL ./authsock.pl --type=CNAME --path=ns1/auth.sock --pidfile=authsock.pid --timeout=120 > /dev/null 2>&1 &
sleep 1
test_update $n testcname.example.nil. CNAME "86400 CNAME testdenied.example.nil" "testdenied" || ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing external update policy (A) ($n)"
ret=0
test_update $n testcname.example.nil. A "86400 A 10.53.0.13" "10.53.0.13" > /dev/null && ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "testing external policy with SIG(0) key ($n)"
ret=0
$NSUPDATE -k ns1/Kkey.example.nil.*.private <<END > /dev/null 2>&1 || ret=1
server 10.53.0.1 ${PORT}
zone example.nil
update add fred.example.nil 120 cname foo.bar.
send
END
output=`$DIG $DIGOPTS +short cname fred.example.nil.`
[ -n "$output" ] || ret=1
[ $ret -eq 0 ] || echo_i "failed"
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "ensure too long realm name is fatal in non-interactive mode ($n)"
ret=0
$NSUPDATE <<END > nsupdate.out${n} 2>&1 && ret=1
    realm namenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename
END
grep "realm is too long" nsupdate.out${n} > /dev/null || ret=1
grep "syntax error" nsupdate.out${n} > /dev/null || ret=1
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

echo_i "ensure too long realm name is not fatal in interactive mode ($n)"
ret=0
$NSUPDATE -i <<END > nsupdate.out${n} 2>&1 || ret=1
    realm namenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename
END
grep "realm is too long" nsupdate.out${n} > /dev/null || ret=1
[ $ret = 0 ] || { echo_i "failed"; status=1; }
n=$((n+1))
if [ "$ret" -ne 0 ]; then echo_i "failed"; fi
status=$((status+ret))

[ $status -eq 0 ] && echo_i "tsiggss tests all OK"

kill `cat authsock.pid`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
