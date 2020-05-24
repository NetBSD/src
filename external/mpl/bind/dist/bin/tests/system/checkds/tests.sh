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

if [ "$CYGWIN" ]; then
    DIG=".\dig.bat"
    WINDSFROMKEY=`cygpath -w $DSFROMKEY`
    CHECKDS="$CHECKDS -a sha1 -a sha256 -d $DIG -D $WINDSFROMKEY"
else
    DIG="./dig.sh"
    CHECKDS="$CHECKDS -a sha1 -a sha256 -d $DIG -D $DSFROMKEY"
fi
chmod +x $DIG

status=0
n=1

echo_i "checking for correct DS, looking up key via 'dig' ($n)"
ret=0
$CHECKDS ok.example > checkds.out.$n 2>&1 || ret=1
grep 'SHA-1' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for correct DS, obtaining key from file ($n)"
ret=0
$CHECKDS -f ok.example.dnskey.db ok.example > checkds.out.$n 2>&1 || ret=1
grep 'SHA-1' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for incorrect DS, looking up key via 'dig' ($n)"
ret=0
$CHECKDS wrong.example > checkds.out.$n 2>&1 || ret=1
grep 'SHA-1' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for incorrect DS, obtaining key from file ($n)"
ret=0
$CHECKDS -f wrong.example.dnskey.db wrong.example > checkds.out.$n 2>&1 || ret=1
grep 'SHA-1' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for partially missing DS, looking up key via 'dig' ($n)"
ret=0
$CHECKDS missing.example > checkds.out.$n 2>&1 && ret=1
grep 'SHA-1.*found' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256.*found' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-1.*missing' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256.*missing' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for partially missing DS, obtaining key from file ($n)"
ret=0
$CHECKDS -f missing.example.dnskey.db missing.example > checkds.out.$n 2>&1 && ret=1
grep 'SHA-1.*found' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256.*found' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-1.*missing' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256.*missing' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for entirely missing DS, looking up key via 'dig' ($n)"
ret=0
$CHECKDS none.example > checkds.out.$n 2>&1 && ret=1
grep 'SHA-1.*found' checkds.out.$n > /dev/null 2>&1 && ret=1
grep 'SHA-256.*found' checkds.out.$n > /dev/null 2>&1 && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking for entirely missing DS, obtaining key from file ($n)"
ret=0
$CHECKDS -f none.example.dnskey.db none.example > checkds.out.$n 2>&1 && ret=1
grep 'SHA-1.*found' checkds.out.$n > /dev/null 2>&1 && ret=1
grep 'SHA-256.*found' checkds.out.$n > /dev/null 2>&1 && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking with prepared dsset file ($n)"
ret=0
$CHECKDS -f prep.example.db -s prep.example.ds.db prep.example > checkds.out.$n 2>&1 || ret=1
grep 'SHA-1.*found' checkds.out.$n > /dev/null 2>&1 || ret=1
grep 'SHA-256.*found' checkds.out.$n > /dev/null 2>&1 || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

if [ $status = 0 ]; then $SHELL clean.sh; fi
echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
