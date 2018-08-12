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

DIGOPTS="@10.53.0.1 -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

newtest() {
	n=`expr $n + 1`
	echo_i "${1} (${n})"
	ret=0
}

test_add() {
    host="$1"
    type="$2"
    ip="$3"

    cat <<EOF > ns1/update.txt
server 10.53.0.1 ${PORT}
ttl 86400
update add $host $type $ip
send
EOF

    newtest "adding $host $type $ip"
    $NSUPDATE ns1/update.txt > /dev/null 2>&1 || {
	[ "$should_fail" ] || \
             echo_i "update failed for $host $type $ip"
	return 1
    }

    out=`$DIG $DIGOPTS +noall +answer -t $type -q $host`
    echo $out > added.a.out.$n
    lines=`echo "$out" | grep "$ip" | wc -l`
    [ $lines -eq 1 ] || {
	[ "$should_fail" ] || \
            echo_i "dig output incorrect for $host $type $cmd: $out"
	return 1
    }

    out=`$DIG $DIGOPTS +noall +answer -x $ip`
    echo $out > added.ptr.out.$n
    lines=`echo "$out" | grep "$host" | wc -l`
    [ $lines -eq 1 ] || {
	[ "$should_fail" ] || \
            echo_i "dig reverse output incorrect for $host $type $cmd: $out"
	return 1
    }

    return 0
}

test_del() {
    host="$1"
    type="$2"

    ip=`$DIG $DIGOPTS +short $host $type`

    cat <<EOF > ns1/update.txt
server 10.53.0.1 ${PORT}
update del $host $type
send
EOF

    newtest "deleting $host $type (was $ip)"
    $NSUPDATE ns1/update.txt > /dev/null 2>&1 || {
	[ "$should_fail" ] || \
             echo_i "update failed deleting $host $type"
	return 1
    }

    out=`$DIG $DIGOPTS +noall +answer -t $type -q $host`
    echo $out > deleted.a.out.$n
    lines=`echo "$out" | grep "$ip" | wc -l`
    [ $lines -eq 0 ] || {
	[ "$should_fail" ] || \
            echo_i "dig output incorrect for $host $type $cmd: $out"
	return 1
    }

    out=`$DIG $DIGOPTS +noall +answer -x $ip`
    echo $out > deleted.ptr.out.$n
    lines=`echo "$out" | grep "$host" | wc -l`
    [ $lines -eq 0 ] || {
	[ "$should_fail" ] || \
            echo_i "dig reverse output incorrect for $host $type $cmd: $out"
	return 1
    }

    return 0
}

test_add test1.ipv4.example.nil. A "10.53.0.10" || ret=1
status=`expr $status + $ret`

test_add test2.ipv4.example.nil. A "10.53.0.11" || ret=1
status=`expr $status + $ret`

test_add test3.ipv4.example.nil. A "10.53.0.12" || ret=1
status=`expr $status + $ret`

test_add test4.ipv6.example.nil. AAAA "2001:db8::1" || ret=1
status=`expr $status + $ret`

test_del test1.ipv4.example.nil. A || ret=1
status=`expr $status + $ret`

test_del test2.ipv4.example.nil. A || ret=1
status=`expr $status + $ret`

test_del test3.ipv4.example.nil. A || ret=1
status=`expr $status + $ret`

test_del test4.ipv6.example.nil. AAAA || ret=1
status=`expr $status + $ret`

newtest "checking parameter logging"
grep "loading params for dyndb 'sample' from .*named.conf:" ns1/named.run > /dev/null || ret=1
grep "loading params for dyndb 'sample2' from .*named.conf:" ns1/named.run > /dev/null || ret=1
[ $ret -eq 1 ] && echo_i "failed"
status=`expr $status + $ret`

echo_i "checking dyndb still works after reload"
$RNDCCMD 10.53.0.1 reload 2>&1 | sed 's/^/ns1 /' | cat_i

test_add test5.ipv4.example.nil. A "10.53.0.10" || ret=1
status=`expr $status + $ret`

test_add test6.ipv6.example.nil. AAAA "2001:db8::1" || ret=1
status=`expr $status + $ret`

test_del test5.ipv4.example.nil. A || ret=1
status=`expr $status + $ret`

test_del test6.ipv6.example.nil. AAAA || ret=1
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
