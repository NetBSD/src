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

# tests for TSIG-GSS updates

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0

DIGOPTS="@10.53.0.1 -p ${PORT}"

test_update() {
    host="$1"
    type="$2"
    cmd="$3"
    digout="$4"

    cat <<EOF > ns1/update.txt
server 10.53.0.1 ${PORT}
update add $host $cmd
send
EOF
    echo "I:testing update for $host $type $cmd"
    $NSUPDATE -g -d ns1/update.txt > nsupdate.out 2>&1 || {
	echo "I:update failed for $host $type $cmd"
	sed "s/^/I:/" nsupdate.out
	return 1
    }

    out=`$DIG $DIGOPTS -t $type -q $host | egrep "^${host}"`
    lines=`echo "$out" | grep "$digout" | wc -l`
    [ $lines -eq 1 ] || {
	echo "I:dig output incorrect for $host $type $cmd: $out"
	return 1
    }
    return 0
}

echo "I:testing updates as administrator"
KRB5CCNAME="FILE:"`pwd`/ns1/administrator.ccache
export KRB5CCNAME

test_update testdc1.example.nil. A "86400 A 10.53.0.10" "10.53.0.10" || status=1
test_update testdc2.example.nil. A "86400 A 10.53.0.11" "10.53.0.11" || status=1
test_update denied.example.nil. TXT "86400 TXT helloworld" "helloworld" > /dev/null && status=1

echo "I:testing updates as a user"
KRB5CCNAME="FILE:"`pwd`/ns1/testdenied.ccache
export KRB5CCNAME

test_update testdenied.example.nil. A "86400 A 10.53.0.12" "10.53.0.12" > /dev/null && status=1
test_update testdenied.example.nil. TXT "86400 TXT helloworld" "helloworld" || status=1

echo "I:testing external update policy"
test_update testcname.example.nil. TXT "86400 CNAME testdenied.example.nil" "testdenied" > /dev/null && status=1
$PERL ./authsock.pl --type=CNAME --path=ns1/auth.sock --pidfile=authsock.pid --timeout=120 > /dev/null 2>&1 &
sleep 1
test_update testcname.example.nil. TXT "86400 CNAME testdenied.example.nil" "testdenied" || status=1
test_update testcname.example.nil. TXT "86400 A 10.53.0.13" "10.53.0.13" > /dev/null && status=1

echo "I:testing external policy with SIG(0) key"
ret=0
$NSUPDATE -R $RANDFILE -k ns1/Kkey.example.nil.*.private <<END > /dev/null 2>&1 || ret=1
server 10.53.0.1 ${PORT}
zone example.nil
update add fred.example.nil 120 cname foo.bar.
send
END
output=`$DIG $DIGOPTS +short cname fred.example.nil.`
[ -n "$output" ] || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:ensure too long realm name is fatal in non-interactive mode"
ret=0
$NSUPDATE <<END > nsupdate.out 2>&1 && ret=1
    realm namenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename
END
grep "realm is too long" nsupdate.out > /dev/null || ret=1
grep "syntax error" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo I:failed; status=1; }

echo "I:ensure too long realm name is not fatal in interactive mode"
ret=0
$NSUPDATE -i <<END > nsupdate.out 2>&1 || ret=1
    realm namenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename
END
grep "realm is too long" nsupdate.out > /dev/null || ret=1
[ $ret = 0 ] || { echo I:failed; status=1; }

[ $status -eq 0 ] && echo "I:tsiggss tests all OK"

kill `cat authsock.pid`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
