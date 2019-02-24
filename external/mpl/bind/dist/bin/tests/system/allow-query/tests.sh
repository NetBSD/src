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

# Test of allow-query statement.
# allow-query takes an address match list and can be included in either the
# options statement or in the zone statement.  This test assumes that the
# acl tests cover the details of the address match list and uses a limited
# number of address match test cases to ensure that allow-query finds the
# expected match.
# Test list:
# In options:
# default (any), any, none, [localhost, localnets],
# allowed address, not allowed address, denied address,
# allowed key, not allowed key, denied key
# allowed acl, not allowed acl, denied acl (acls pointing to addresses)
#
# Each of these tests requires changing to a new configuration
# file and using rndc to update the server
#
# In view, with nothing in options (default to any)
# default (any), any, none, [localhost, localnets],
# allowed address, not allowed address, denied address,
# allowed key, not allowed key, denied key
# allowed acl, not allowed acl, denied acl (acls pointing to addresses)
#
# In view, with options set to none, view set to any
# In view, with options set to any, view set to none
#
# In zone, with nothing in options (default to any)
# any, none, [localhost, localnets],
# allowed address, denied address,
# allowed key, not allowed key, denied key
# allowed acl, not allowed acl, denied acl (acls pointing to addresses),
#
# In zone, with options set to none, zone set to any
# In zone, with options set to any, zone set to none
# In zone, with view set to none, zone set to any
# In zone, with view set to any, zone set to none
#
# zone types of master, slave and stub can be tested in parallel by using
# multiple instances (ns2 as master, ns3 as slave, ns4 as stub) and querying
# as necessary.
#

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

DIGOPTS="+tcp +nosea +nostat +nocmd +norec +noques +noauth +noadd +nostats +dnssec -p ${PORT}"

status=0
n=0

nextpart ns2/named.run > /dev/null

# Test 1 - default, query allowed
n=`expr $n + 1`
echo_i "test $n: default - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 2 - explicit any, query allowed
n=`expr $n + 1`
copy_setports ns2/named02.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: explicit any - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 3 - none, query refused
n=`expr $n + 1`
copy_setports ns2/named03.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: none - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 4 - address allowed, query allowed
n=`expr $n + 1`
copy_setports ns2/named04.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: address allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 5 - address not allowed, query refused
n=`expr $n + 1`
copy_setports ns2/named05.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: address not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 6 - address disallowed, query refused
n=`expr $n + 1`
copy_setports ns2/named06.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: address disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 7 - acl allowed, query allowed
n=`expr $n + 1`
copy_setports ns2/named07.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: acl allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 8 - acl not allowed, query refused
n=`expr $n + 1`
copy_setports ns2/named08.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: acl not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`


# Test 9 - acl disallowed, query refused
n=`expr $n + 1`
copy_setports ns2/named09.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: acl disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 10 - key allowed, query allowed
n=`expr $n + 1`
copy_setports ns2/named10.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: key allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y one:1234abcd8765 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 11 - key not allowed, query refused
n=`expr $n + 1`
copy_setports ns2/named11.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: key not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y two:1234efgh8765 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 12 - key disallowed, query refused
n=`expr $n + 1`
copy_setports ns2/named12.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: key disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y one:1234abcd8765 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# The next set of tests check if allow-query works in a view

n=20
# Test 21 - views default, query allowed
n=`expr $n + 1`
copy_setports ns2/named21.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views default - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 22 - views explicit any, query allowed
n=`expr $n + 1`
copy_setports ns2/named22.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views explicit any - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 23 - views none, query refused
n=`expr $n + 1`
copy_setports ns2/named23.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views none - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 24 - views address allowed, query allowed
n=`expr $n + 1`
copy_setports ns2/named24.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views address allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 25 - views address not allowed, query refused
n=`expr $n + 1`
copy_setports ns2/named25.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views address not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 26 - views address disallowed, query refused
n=`expr $n + 1`
copy_setports ns2/named26.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views address disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 27 - views acl allowed, query allowed
n=`expr $n + 1`
copy_setports ns2/named27.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views acl allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 28 - views acl not allowed, query refused
n=`expr $n + 1`
copy_setports ns2/named28.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views acl not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 29 - views acl disallowed, query refused
n=`expr $n + 1`
copy_setports ns2/named29.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views acl disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 30 - views key allowed, query allowed
n=`expr $n + 1`
copy_setports ns2/named30.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views key allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y one:1234abcd8765 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 31 - views key not allowed, query refused
n=`expr $n + 1`
copy_setports ns2/named31.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views key not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y two:1234efgh8765 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 32 - views key disallowed, query refused
n=`expr $n + 1`
copy_setports ns2/named32.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views key disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y one:1234abcd8765 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 33 - views over options, views allow, query allowed
n=`expr $n + 1`
copy_setports ns2/named33.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views over options, views allow - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 34 - views over options, views disallow, query refused
n=`expr $n + 1`
copy_setports ns2/named34.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views over options, views disallow - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Tests for allow-query in the zone statements

n=40

# Test 41 - zone default, query allowed
n=`expr $n + 1`
copy_setports ns2/named40.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: zone default - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 42 - zone explicit any, query allowed
n=`expr $n + 1`
echo_i "test $n: zone explicit any - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.any.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.any.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 43 - zone none, query refused
n=`expr $n + 1`
echo_i "test $n: zone none - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.none.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.none.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 44 - zone address allowed, query allowed
n=`expr $n + 1`
echo_i "test $n: zone address allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.addrallow.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.addrallow.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 45 - zone address not allowed, query refused
n=`expr $n + 1`
echo_i "test $n: zone address not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.addrnotallow.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.addrnotallow.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 46 - zone address disallowed, query refused
n=`expr $n + 1`
echo_i "test $n: zone address disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.addrdisallow.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.addrdisallow.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 47 - zone acl allowed, query allowed
n=`expr $n + 1`
echo_i "test $n: zone acl allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.aclallow.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.aclallow.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 48 - zone acl not allowed, query refused
n=`expr $n + 1`
echo_i "test $n: zone acl not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.aclnotallow.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.aclnotallow.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 49 - zone acl disallowed, query refused
n=`expr $n + 1`
echo_i "test $n: zone acl disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.acldisallow.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.acldisallow.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 50 - zone key allowed, query allowed
n=`expr $n + 1`
echo_i "test $n: zone key allowed - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y one:1234abcd8765 a.keyallow.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.keyallow.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 51 - zone key not allowed, query refused
n=`expr $n + 1`
echo_i "test $n: zone key not allowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y two:1234efgh8765 a.keyallow.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.keyallow.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 52 - zone key disallowed, query refused
n=`expr $n + 1`
echo_i "test $n: zone key disallowed - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 -y one:1234abcd8765 a.keydisallow.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.keydisallow.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 53 - zones over options, zones allow, query allowed
n=`expr $n + 1`
copy_setports ns2/named53.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views over options, views allow - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 54 - zones over options, zones disallow, query refused
n=`expr $n + 1`
copy_setports ns2/named54.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: views over options, views disallow - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 55 - zones over views, zones allow, query allowed
n=`expr $n + 1`
copy_setports ns2/named55.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: zones over views, views allow - query allowed"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 56 - zones over views, zones disallow, query refused
n=`expr $n + 1`
copy_setports ns2/named56.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: zones over views, views disallow - query refused"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 57 - zones over views, zones disallow, query refused (allow-query-on)
n=`expr $n + 1`
copy_setports ns2/named57.conf.in ns2/named.conf
rndc_reload ns2 10.53.0.2

echo_i "test $n: zones over views, allow-query-on"
ret=0
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.normal.example a > dig.out.ns2.1.$n || ret=1
grep 'status: NOERROR' dig.out.ns2.1.$n > /dev/null || ret=1
grep '^a.normal.example' dig.out.ns2.1.$n > /dev/null || ret=1
$DIG $DIGOPTS @10.53.0.2 -b 10.53.0.2 a.aclnotallow.example a > dig.out.ns2.2.$n || ret=1
grep 'status: REFUSED' dig.out.ns2.2.$n > /dev/null || ret=1
grep '^a.aclnotallow.example' dig.out.ns2.2.$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 58 - allow-recursion default
n=`expr $n + 1`
echo_i "test $n: default allow-recursion configuration"
ret=0
$DIG -p ${PORT} @10.53.0.3 -b 127.0.0.1 a.normal.example a > dig.out.ns3.1.$n
grep 'status: NOERROR' dig.out.ns3.1.$n > /dev/null || ret=1
$DIG -p ${PORT} @10.53.0.3 -b 10.53.0.1 a.normal.example a > dig.out.ns3.2.$n
grep 'status: REFUSED' dig.out.ns3.2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 59 - allow-query-cache default
n=`expr $n + 1`
echo_i "test $n: default allow-query-cache configuration"
ret=0
$DIG -p ${PORT} @10.53.0.3 -b 127.0.0.1 ns . > dig.out.ns3.1.$n
grep 'status: NOERROR' dig.out.ns3.1.$n > /dev/null || ret=1
$DIG -p ${PORT} @10.53.0.3 -b 10.53.0.1 ns . > dig.out.ns3.2.$n
grep 'status: REFUSED' dig.out.ns3.2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 60 - block recursion-on, allow query-cache-on
n=`expr $n + 1`
copy_setports ns3/named2.conf.in ns3/named.conf
rndc_reload ns3 10.53.0.3

echo_i "test $n: block recursion-on, allow query-cache-on"
ret=0
# this should query the cache, and an answer should already be there
$DIG -p ${PORT} @10.53.0.3 a.normal.example a > dig.out.ns3.1.$n
grep 'recursion requested but not available' dig.out.ns3.1.$n > /dev/null || ret=1
grep 'ANSWER: 1' dig.out.ns3.1.$n > /dev/null || ret=1
# this should require recursion and therefore can't get an answer
$DIG -p ${PORT} @10.53.0.3 b.normal.example a > dig.out.ns3.2.$n
grep 'recursion requested but not available' dig.out.ns3.2.$n > /dev/null || ret=1
grep 'ANSWER: 0' dig.out.ns3.2.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 61 - inheritance of allow-query-cache-on from allow-recursion-on
n=`expr $n + 1`
copy_setports ns3/named3.conf.in ns3/named.conf
rndc_reload ns3 10.53.0.3

echo_i "test $n: inheritance of allow-query-cache-on"
ret=0
# this should query the cache, an answer should already be there
$DIG -p ${PORT} @10.53.0.3 a.normal.example a > dig.out.ns3.1.$n
grep 'ANSWER: 1' dig.out.ns3.1.$n > /dev/null || ret=1
# this should be refused due to allow-recursion-on/allow-query-cache-on
$DIG -p ${PORT} @10.53.1.2 a.normal.example a > dig.out.ns3.2.$n
grep 'recursion requested but not available' dig.out.ns3.2.$n > /dev/null || ret=1
grep 'status: REFUSED' dig.out.ns3.2.$n > /dev/null || ret=1
# this should require recursion and should be allowed
$DIG -p ${PORT} @10.53.0.3 c.normal.example a > dig.out.ns3.3.$n
grep 'ANSWER: 1' dig.out.ns3.3.$n > /dev/null || ret=1
# this should require recursion and be refused
$DIG -p ${PORT} @10.53.1.2 d.normal.example a > dig.out.ns3.4.$n
grep 'recursion requested but not available' dig.out.ns3.4.$n > /dev/null || ret=1
grep 'status: REFUSED' dig.out.ns3.4.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

# Test 62 - inheritance of allow-recursion-on from allow-query-cache-on
n=`expr $n + 1`
copy_setports ns3/named4.conf.in ns3/named.conf
rndc_reload ns3 10.53.0.3

echo_i "test $n: inheritance of allow-recursion-on"
ret=0
# this should query the cache, an answer should already be there
$DIG -p ${PORT} @10.53.0.3 a.normal.example a > dig.out.ns3.1.$n
grep 'ANSWER: 1' dig.out.ns3.1.$n > /dev/null || ret=1
# this should be refused due to allow-recursion-on/allow-query-cache-on
$DIG -p ${PORT} @10.53.1.2 a.normal.example a > dig.out.ns3.2.$n
grep 'recursion requested but not available' dig.out.ns3.2.$n > /dev/null || ret=1
grep 'status: REFUSED' dig.out.ns3.2.$n > /dev/null || ret=1
# this should require recursion and should be allowed
$DIG -p ${PORT} @10.53.0.3 e.normal.example a > dig.out.ns3.3.$n
grep 'ANSWER: 1' dig.out.ns3.3.$n > /dev/null || ret=1
# this should require recursion and be refused
$DIG -p ${PORT} @10.53.1.2 f.normal.example a > dig.out.ns3.4.$n
grep 'recursion requested but not available' dig.out.ns3.4.$n > /dev/null || ret=1
grep 'status: REFUSED' dig.out.ns3.4.$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
