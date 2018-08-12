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

rm -f dig.out.*

DIGOPTS="+tcp +short -p ${PORT} @10.53.0.2"
DIGOPTS6="+tcp +short -p ${PORT} @fd92:7065:b8e:ffff::2"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

n=`expr $n + 1`
echo_i "checking GeoIP country database by code ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP country database by code (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking response scope using client subnet ($n)"
ret=0
$DIG +tcp -p ${PORT} @10.53.0.2 txt example -b 127.0.0.1 +subnet="10.53.0.1/32" > dig.out.ns2.test$n.1 || ret=1
grep 'CLIENT-SUBNET.*10.53.0.1/32/32' dig.out.ns2.test$n.1 > /dev/null || ret=1
$DIG +tcp -p ${PORT} @10.53.0.2 txt example -b 127.0.0.1 +subnet="192.0.2.64/32" > dig.out.ns2.test$n.2 || ret=1
grep 'CLIENT-SUBNET.*192.0.2.64/32/24' dig.out.ns2.test$n.2 > /dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named2.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP country database by three-letter code ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named3.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP country database by name ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named4.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP region code, no specified database ($n)"
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
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named5.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP region database by region name and country code ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP region database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`


echo_i "reloading server"
copy_setports ns2/named6.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking GeoIP city database by city name using IPv6 ($n)"
  ret=0
  $DIG +tcp +short -p ${PORT} @fd92:7065:b8e:ffff::1 -6 txt example -b fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n || ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping"
fi

n=`expr $n + 1`
echo_i "checking GeoIP city database by city name ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP city database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named7.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP isp database ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP isp database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named8.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP org database ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP org database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named9.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP asnum database ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP asnum database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named10.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP asnum database - ASNNNN only ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP asnum database - ASNNNN only (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named11.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP domain database ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP domain database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named12.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP netspeed database ($n)"
ret=0
lret=0
for i in 1 2 3 4; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP netspeed database (using client subnet) ($n)"
ret=0
lret=0
for i in 1 2 3 4; do
    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named13.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP blackhole ACL ($n)"
ret=0
$DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n || ret=1
$RNDCCMD 10.53.0.2 status 2>&1 > rndc.out.ns2.test$n || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named14.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP country database by code (using nested ACLs) ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "reloading server"
copy_setports ns2/named14.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3

n=`expr $n + 1`
echo_i "checking geoip-use-ecs ($n)"
ret=0
lret=0
for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break

    $DIG $DIGOPTS txt example -b 127.0.0.1 +subnet="10.53.0.$i/32" > dig.out.ns2.test$n.ecs.$i || lret=1
    j=`cat dig.out.ns2.test$n.ecs.$i | tr -d '"'`
    [ "$j" = "bogus" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "reloading server with different geoip-directory ($n)"
copy_setports ns2/named15.conf.in ns2/named.conf
$RNDCCMD 10.53.0.2 reload 2>&1 | sed 's/^/ns2 /' | cat_i
sleep 3
awk '/using "..\/data2" as GeoIP directory/ {m=1} ; { if (m>0) { print } }' ns2/named.run | grep "GeoIP City .* DB not available" > /dev/null || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking GeoIP v4/v6 when only IPv6 database is available ($n)"
ret=0
$DIG $DIGOPTS -4 txt example -b 10.53.0.2 > dig.out.ns2.test$n.1 || ret=1
j=`cat dig.out.ns2.test$n.1 | tr -d '"'`
[ "$j" = "bogus" ] || ret=1
if $TESTSOCK6 fd92:7065:b8e:ffff::2; then
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::2 > dig.out.ns2.test$n.2 || ret=1
    j=`cat dig.out.ns2.test$n.2 | tr -d '"'`
    [ "$j" = "2" ] || ret=1
fi
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking other GeoIP options are parsed correctly ($n)"
ret=0
$CHECKCONF options.conf || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
