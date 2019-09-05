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
DIGOPTS6="+tcp +short -p ${PORT} @fd92:7065:b8e:ffff::2 -6"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

for conf in conf/good*.conf
do
	n=`expr $n + 1`
	echo_i "checking that $conf is accepted ($n)"
	ret=0
	$CHECKCONF "$conf" || ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

for conf in conf/bad*.conf
do
	n=`expr $n + 1`
	echo_i "checking that $conf is rejected ($n)"
	ret=0
	$CHECKCONF "$conf" >/dev/null && ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo_i "checking Country database by code using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking Country database by code using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 country code test"
fi

echo_i "reloading server"
copy_setports ns2/named2.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking Country database with nested ACLs using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking Country database with nested ACLs using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
      j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
      [ "$i" = "$j" ] || lret=1
      [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 country nested ACL test"
fi

echo_i "reloading server"
copy_setports ns2/named3.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking Country database by name using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking Country database by name using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 country name test"
fi

echo_i "reloading server"
copy_setports ns2/named4.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking Country database by continent code using IPv4 ($n)"
ret=0
lret=0
# deliberately skipping 4 and 6 as they have duplicate continents
for i in 1 2 3 5 7; do
    $DIG $DIGOPTS txt example -b 10.53.0.$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
done
[ $lret -eq 1 ] && ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking Country database by continent code using IPv6 ($n)"
  ret=0
  lret=0
  # deliberately skipping 4 and 6 as they have duplicate continents
  for i in 1 2 3 5 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 continent code test"
fi

echo_i "reloading server"
copy_setports ns2/named5.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking City database by region code using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking City database by region code using IPv6 ($n)"
  ret=0
  lret=0
# skipping 2 on purpose here; it has the same region code as 1
  for i in 1 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 region code test"
fi

n=`expr $n + 1`
echo_i "reloading server"
copy_setports ns2/named6.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking City database by city name using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking City database by city name using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
    j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
    [ "$i" = "$j" ] || lret=1
    [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 city test"
fi

echo_i "reloading server"
copy_setports ns2/named7.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking ISP database using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking ISP database using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
      j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
      [ "$i" = "$j" ] || lret=1
      [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 ISP test"
fi

echo_i "reloading server"
copy_setports ns2/named8.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking ASN database by org name using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking ASN database by org name using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
      j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
      [ "$i" = "$j" ] || lret=1
      [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 ASN test"
fi

echo_i "reloading server"
copy_setports ns2/named9.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP6 ASN database, ASNNNN only, using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking ASN database, ASNNNN only, using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
      j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
      [ "$i" = "$j" ] || lret=1
      [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 ASN test"
fi

echo_i "reloading server"
copy_setports ns2/named10.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking GeoIP6 ASN database, NNNN only, using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking ASN database, NNNN only, using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
      j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
      [ "$i" = "$j" ] || lret=1
      [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 ASN test"
fi

echo_i "reloading server"
copy_setports ns2/named11.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking Domain database using IPv4 ($n)"
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

if $TESTSOCK6 fd92:7065:b8e:ffff::3
then
  n=`expr $n + 1`
  echo_i "checking Domain database using IPv6 ($n)"
  ret=0
  lret=0
  for i in 1 2 3 4 5 6 7; do
    $DIG $DIGOPTS6 txt example -b fd92:7065:b8e:ffff::$i > dig.out.ns2.test$n.$i || lret=1
      j=`cat dig.out.ns2.test$n.$i | tr -d '"'`
      [ "$i" = "$j" ] || lret=1
      [ $lret -eq 1 ] && break
  done
  [ $lret -eq 1 ] && ret=1
  [ $ret -eq 0 ] || echo_i "failed"
  status=`expr $status + $ret`
else
  echo_i "IPv6 unavailable; skipping IPv6 Domain test"
fi

echo_i "reloading server"
copy_setports ns2/named12.conf.in ns2/named.conf
$CHECKCONF ns2/named.conf | cat_i
rndc_reload ns2 10.53.0.2
sleep 3

n=`expr $n + 1`
echo_i "checking geoip blackhole ACL ($n)"
ret=0
$DIG $DIGOPTS txt example -b 10.53.0.7 > dig.out.ns2.test$n || ret=1
$RNDCCMD 10.53.0.2 status 2>&1 > rndc.out.ns2.test$n || ret=1
[ $ret -eq 0 ] || echo_i "failed"
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
