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

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existance proof is returned auth ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec +norec @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existance proof is returned non-validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns2.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existance proof is returned validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existance proof is returned validating + CD ($n)"
ret=0
$DIG $DIGOPTS +cd a b.wild.nsec @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns5.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns5.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`
n=`expr $n + 1`

echo_i "checking that returned NSEC wildcard non-existance proof validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'a\.wild\.nsec\..*NSEC.*nsec\..*NSEC'  dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC wildcard non-existance proof is returned private, validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'a\.wild\.private\.nsec\..*NSEC.*private\.nsec\..*NSEC'  dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that returned NSEC wildcard non-existance proof for private zone validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'a\.wild\.private\.nsec\..*NSEC.*private\.nsec\..*NSEC'  dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existance proof is returned auth ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 +norec @10.53.0.1 > dig.out.ns1.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns1.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existance proof is returned non-validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 @10.53.0.2 > dig.out.ns2.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns2.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns2.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existance proof is returned validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existance proof is returned validating + CD ($n)"
ret=0
$DIG $DIGOPTS +cd a b.wild.nsec3 @10.53.0.5 > dig.out.ns5.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns5.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns5.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that returned NSEC3 wildcard non-existance proof validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.nsec3 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'O3TJ8D9AJ54CBTFCQCJ3QK49CH7SF6H9\.nsec3\..*V5DLFB6UJNHR94LQ61FO607KGK12H88A'  dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that NSEC3 wildcard non-existance proof is returned private, validating ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec3 @10.53.0.3 > dig.out.ns3.test$n || ret=1
grep -i 'UDBSP4R8OUOT6HSO39VD8B5LMOSHRD5N\.private\.nsec3\..*NSEC3.*ASDRUIB7GO00OR92S5OUGI404LT27RNU' dig.out.ns3.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns3.test$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo_i "checking that returned NSEC3 wildcard non-existance proof for private zone validates ($n)"
ret=0
$DIG $DIGOPTS a b.wild.private.nsec3 @10.53.0.4 > dig.out.ns4.test$n || ret=1
grep -i 'UDBSP4R8OUOT6HSO39VD8B5LMOSHRD5N\.private\.nsec3\..*NSEC3.*ASDRUIB7GO00OR92S5OUGI404LT27RNU' dig.out.ns4.test$n > /dev/null || ret=1
grep -i 'flags:.* ad[ ;]'  dig.out.ns4.test$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
