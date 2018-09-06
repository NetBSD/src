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

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p 5300"

status=0
ret=0

algs=""
have_rsa=`grep rsa supported`
if [ "x$have_rsa" != "x" ]; then
    algs="rsa "
fi
have_ecc=`grep ecc supported`
if [ "x$have_ecc" != "x" ]; then
    algs=$algs"ecc "
fi
have_ecx=`grep ecc supported`
if [ "x$have_ecx" != "x" ]; then
    algs=$algs"ecx "
fi

for alg in $algs; do
    zonefile=ns1/$alg.example.db 
    echo "I:testing PKCS#11 key generation ($alg)"
    count=`$PK11LIST | grep robie-$alg-ksk | wc -l`
    if [ $count != 2 ]; then echo "I:failed"; status=1; fi

    echo "I:testing offline signing with PKCS#11 keys ($alg)"

    count=`grep RRSIG $zonefile.signed | wc -l`
    if [ $count != 12 ]; then echo "I:failed"; status=1; fi

    echo "I:testing inline signing with PKCS#11 keys ($alg)"

    $DIG $DIGOPTS ns.$alg.example. @10.53.0.1 a > dig.out.$alg.0 || ret=1
    if [ $ret != 0 ]; then echo "I:failed"; fi
    status=`expr $status + $ret`
    count0=`grep RRSIG dig.out.$alg.0 | wc -l`

    $NSUPDATE -v > upd.log.$alg <<END || status=1
server 10.53.0.1 5300
ttl 300
zone $alg.example.
update add `grep -v ';' ns1/${alg}.key`
send
END

    echo "I:waiting 20 seconds for key changes to take effect"
    sleep 20

    $DIG $DIGOPTS ns.$alg.example. @10.53.0.1 a > dig.out.$alg || ret=1
    if [ $ret != 0 ]; then echo "I:failed"; fi
    status=`expr $status + $ret`
    count=`grep RRSIG dig.out.$alg | wc -l`
    if [ $count -le $count0 ]; then echo "I:failed"; status=1; fi

    echo "I:testing PKCS#11 key destroy ($alg)"
    ret=0
    $PK11DEL -l robie-$alg-ksk -w0 > /dev/null 2>&1 || ret=1
    $PK11DEL -l robie-$alg-zsk1 -w0 > /dev/null 2>&1 || ret=1
    case $alg in
        rsa) id=02 ;;
        ecc) id=04 ;;
	ecx) id=06 ;;
    esac
    $PK11DEL -i $id -w0 > /dev/null 2>&1 || ret=1
    if [ $ret != 0 ]; then echo "I:failed"; fi
    status=`expr $status + $ret`
    count=`$PK11LIST | grep robie-$alg | wc -l`
    if [ $count != 0 ]; then echo "I:failed"; fi
    status=`expr $status + $count`
done

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
