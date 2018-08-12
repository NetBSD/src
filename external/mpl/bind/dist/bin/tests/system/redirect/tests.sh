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
n=1

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd -p ${PORT}"
RNDCCMD="$RNDC -c $SYSTEMTESTTOP/common/rndc.conf -p ${CONTROLPORT} -s"

for conf in conf/good*.conf
do
        echo_i "checking that $conf is accepted ($n)"
        ret=0
        $CHECKCONF "$conf" || ret=1
        n=`expr $n + 1`
        if [ $ret != 0 ]; then echo_i "failed"; fi
        status=`expr $status + $ret`
done

for conf in conf/bad*.conf
do
        echo_i "checking that $conf is rejected ($n)"
        ret=0
        $CHECKCONF "$conf" >/dev/null && ret=1
	n=`expr $n + 1`
        if [ $ret != 0 ]; then echo_i "failed"; fi
        status=`expr $status + $ret`
done

echo_i "checking A zone redirect works for nonexist ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect updates statistics ($n)"
ret=0
rm ns2/named.stats 2>/dev/null
$RNDCCMD 10.53.0.2 stats || ret=1
PRE=`sed -n -e "s/[    ]*\([0-9]*\).queries resulted in NXDOMAIN that were redirected$/\1/p"  ns2/named.stats`
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.2 a > dig.out.ns2.test$n || ret=1
rm ns2/named.stats 2>/dev/null
$RNDCCMD 10.53.0.2 stats || ret=1
POST=`sed -n -e "s/[    ]*\([0-9]*\).queries resulted in NXDOMAIN that were redirected$/\1/p"  ns2/named.stats`
if [ `expr $POST - $PRE` != 1 ]; then ret=1; fi
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect works for nonexist ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.2 aaaa > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect works for nonexist ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.2 any > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect doesn't work for acl miss ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.4 a > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect doesn't work for acl miss ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.4 aaaa > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect doesn't work for acl miss ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.4 any > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect works for signed nonexist, DO=0 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.2 -b 10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect works for signed nonexist, DO=0 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.2 -b 10.53.0.2 aaaa > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect works for signed nonexist, DO=0 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.2 -b 10.53.0.2 any > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect fails for signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.2 -b 10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect fails for signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.2 -b 10.53.0.2 aaaa > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect fails for signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.2 -b 10.53.0.2 any > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect fails for nsec3 signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.2 -b 10.53.0.2 a > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect fails for nsec3 signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.2 -b 10.53.0.2 aaaa > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect fails for nsec3 signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.2 -b 10.53.0.2 any > dig.out.ns2.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns2.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns2.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns2.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect works for nonexist authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.1 -b 10.53.0.1 a > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect works for nonexist authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.1 -b 10.53.0.1 aaaa > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect works for nonexist authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.1 -b 10.53.0.1 any > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect doesn't work for acl miss authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.1 -b 10.53.0.4 a > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect doesn't work for acl miss authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.1 -b 10.53.0.4 aaaa > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect doesn't work for acl miss authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.1 -b 10.53.0.4 any > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect works for signed nonexist, DO=0 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.1 -b 10.53.0.1 a > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect works for signed nonexist, DO=0 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.1 -b 10.53.0.1 aaaa > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect works for signed nonexist, DO=0 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.1 -b 10.53.0.1 any > dig.out.ns1.test$n || ret=1
grep "status: NOERROR" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect fails for signed nonexist, DO=1 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.1 -b 10.53.0.1 a > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect fails for signed nonexist, DO=1 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.1 -b 10.53.0.1 aaaa > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect fails for signed nonexist, DO=1 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.1 -b 10.53.0.1 any > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A zone redirect fails for nsec3 signed nonexist, DO=1 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.1 -b 10.53.0.1 a > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA zone redirect fails for nsec3 signed nonexist, DO=1 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.1 -b 10.53.0.1 aaaa > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY zone redirect fails for nsec3 signed nonexist, DO=1 authoritative ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.1 -b 10.53.0.1 any > dig.out.ns1.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns1.test$n > /dev/null || ret=1
grep "100.100.100.2" dig.out.ns1.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6402" dig.out.ns1.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns1.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking zone redirect works (with noerror) when qtype is not found ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.2 txt > dig.out.ns2.test$n || ret=1
grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that redirect zones reload correctly"
ret=0
sleep 1 # ensure file mtime will have changed
sed -e 's/0 0 0 0 0/1 0 0 0 0/' < ns2/example.db.in > ns2/example.db
sed -e 's/0 0 0 0 0/1 0 0 0 0/' -e 's/\.1$/.2/' < ns2/redirect.db.in > ns2/redirect.db
$RNDCCMD 10.53.0.2 reload > rndc.out || ret=1
sed 's/^/ns2 /' rndc.out | cat_i
for i in 1 2 3 4 5 6 7 8 9; do
    tmp=0
    $DIG $DIGOPTS +short @10.53.0.2 soa example.nil > dig.out.ns1.test$n || tmp=1
    set -- `cat dig.out.ns1.test$n`
    [ $3 = 1 ] || tmp=1
    $DIG $DIGOPTS nonexist. @10.53.0.2 -b 10.53.0.2 a > dig.out.ns2.test$n || tmp=1
    grep "status: NOERROR" dig.out.ns2.test$n > /dev/null || tmp=1
    grep "100.100.100.2" dig.out.ns2.test$n > /dev/null || tmp=1
    [ $tmp -eq 0 ] && break
    sleep 1
done
[ $tmp -eq 1 ] && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A nxdomain-redirect works for nonexist ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.4 -b 10.53.0.2 a > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "nonexist.	.*100.100.100.1" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA nxdomain-redirect works for nonexist ($n)"
ret=0
rm ns4/named.stats 2>/dev/null
$RNDCCMD 10.53.0.4 stats || ret=1
PRE_RED=`sed -n -e "s/[    ]*\([0-9]*\).queries resulted in NXDOMAIN that were redirected$/\1/p"  ns4/named.stats`
PRE_SUC=`sed -n -e "s/[    ]*\([0-9]*\).queries resulted in NXDOMAIN that were redirected and resulted in a successful remote lookup$/\1/p"  ns4/named.stats`
$DIG $DIGOPTS nonexist. @10.53.0.4 -b 10.53.0.2 aaaa > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "nonexist.	.*2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA nxdomain-redirect updates statistics ($n)"
ret=0
rm ns4/named.stats 2>/dev/null
$RNDCCMD 10.53.0.4 stats || ret=1
POST_RED=`sed -n -e "s/[    ]*\([0-9]*\).queries resulted in NXDOMAIN that were redirected$/\1/p"  ns4/named.stats`
POST_SUC=`sed -n -e "s/[    ]*\([0-9]*\).queries resulted in NXDOMAIN that were redirected and resulted in a successful remote lookup$/\1/p"  ns4/named.stats`
if [ `expr $POST_RED - $PRE_RED` != 1 ]; then ret=1; fi
if [ `expr $POST_SUC - $PRE_SUC` != 1 ]; then ret=1; fi
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY nxdomain-redirect works for nonexist ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.4 -b 10.53.0.2 any > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A nxdomain-redirect works for signed nonexist, DO=0 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.4 -b 10.53.0.2 a > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA nxdomain-redirect works for signed nonexist, DO=0 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.4 -b 10.53.0.2 aaaa > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY nxdomain-redirect works for signed nonexist, DO=0 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. @10.53.0.4 -b 10.53.0.2 any > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A nxdomain-redirect fails for signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.4 -b 10.53.0.2 a > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA nxdomain-redirect fails for signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.4 -b 10.53.0.2 aaaa > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY nxdomain-redirect fails for signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.signed. +dnssec @10.53.0.4 -b 10.53.0.2 any > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null && ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking A nxdomain-redirect fails for nsec3 signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.4 -b 10.53.0.2 a > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking AAAA nxdomain-redirect fails for nsec3 signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.4 -b 10.53.0.2 aaaa > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking ANY nxdomain-redirect fails for nsec3 signed nonexist, DO=1 ($n)"
ret=0
$DIG $DIGOPTS nonexist.nsec3. +dnssec @10.53.0.4 -b 10.53.0.2 any > dig.out.ns4.test$n || ret=1
grep "status: NXDOMAIN" dig.out.ns4.test$n > /dev/null || ret=1
grep "100.100.100.1" dig.out.ns4.test$n > /dev/null && ret=1
grep "2001:ffff:ffff::6464:6401" dig.out.ns4.test$n > /dev/null && ret=1
grep "IN.NSEC3" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking nxdomain-redirect works (with noerror) when qtype is not found ($n)"
ret=0
$DIG $DIGOPTS nonexist. @10.53.0.4 -b 10.53.0.2 txt > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking nxdomain-redirect against authoritative zone ($n)"
ret=0
$DIG $DIGOPTS nonexist.example @10.53.0.4 -b 10.53.0.2 a > dig.out.ns4.test$n || ret=1
grep "status: NOERROR" dig.out.ns4.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
