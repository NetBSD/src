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

alg=rsa
zonefile=ns1/rsa.example.db 
echo "I:testing PKCS#11 key generation (rsa)"
count=`$PK11LIST | grep robie-rsa-ksk | wc -l`
if [ $count != 2 ]; then echo "I:failed"; status=1; fi

echo "I:testing offline signing with PKCS#11 keys (rsa)"

count=`grep RRSIG $zonefile.signed | wc -l`
if [ $count != 12 ]; then echo "I:failed"; status=1; fi

echo "I:testing inline signing with PKCS#11 keys (rsa)"

$NSUPDATE > /dev/null <<END || status=1
server 10.53.0.1 5300
ttl 300
zone rsa.example.
update add `grep -v ';' ns1/${alg}.key`
send
END

echo "I:waiting 20 seconds for key changes to take effect"
sleep 20

$DIG $DIGOPTS ns.rsa.example. @10.53.0.1 a > dig.out || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`
count=`grep RRSIG dig.out | wc -l`
if [ $count != 4 ]; then echo "I:failed"; status=1; fi

echo "I:testing PKCS#11 key destroy (rsa)"
ret=0
$PK11DEL -l robie-rsa-ksk -w0 > /dev/null 2>&1 || ret=1
$PK11DEL -l robie-rsa-zsk1 -w0 > /dev/null 2>&1 || ret=1
$PK11DEL -i $id -w0 > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`
count=`$PK11LIST | grep robie-rsa | wc -l`
if [ $count != 0 ]; then echo "I:failed"; fi
status=`expr $status + $count`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
