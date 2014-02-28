#!/bin/sh
#
# Copyright (C) 2013, 2014  Internet Systems Consortium, Inc. ("ISC")
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0

rm -f dig.out.*

DIGOPTS="+tcp +short -p 5300 @10.53.0.2"

n=`expr $n + 1`
echo "I:checking GeoIP country database by code"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named2.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP country database by three-letter code"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named3.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP country database by name"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named4.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP region code, no specified database"
ret=0
lret=0
# skipping 2 on purpose here; it has the same region code as 1
for i in 1 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named5.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP region database by region name and country code"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named6.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP city database by city name"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named7.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP isp database"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named8.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP org database"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named9.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP asnum database"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named10.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP domain database"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named11.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP netspeed database"
ret=0
lret=0
for i in 1 2 3 4; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:reloading server"
cp -f ns2/named12.conf ns2/named.conf
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 reload 2>&1 | sed 's/^/I:ns2 /'
sleep 3

n=`expr $n + 1`
echo "I:checking GeoIP blackhole ACL"
ret=0
$DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n || ret=1
$RNDC -c ../common/rndc.conf -s 10.53.0.2 -p 9953 status 2>&1 > rndc.out.ns2.test$n || ret=1
[ $ret -eq 0 ] || echo "I:failed"
status=`expr $status + $ret`

echo "I:exit status: $status"
exit $status
