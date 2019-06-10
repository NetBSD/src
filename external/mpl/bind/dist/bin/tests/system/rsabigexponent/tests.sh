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

rm -f dig.out.*

DIGOPTS="+tcp +noadd +nosea +nostat +nocmd +dnssec -p ${PORT}"

for f in conf/good*.conf
do
	echo_i "checking '$f'"
	ret=0
	$CHECKCONF $f > /dev/null || ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

for f in conf/bad*.conf
do
	echo_i "checking '$f'"
	ret=0
	$CHECKCONF $f > /dev/null && ret=1
	if [ $ret != 0 ]; then echo_i "failed"; fi
	status=`expr $status + $ret`
done

echo_i "checking that RSA big exponent keys can't be loaded"
ret=0
grep "out of range" ns2/signer.err > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "checking that RSA big exponent signature can't validate"
ret=0
$DIG $DIGOPTS a.example @10.53.0.2 > dig.out.ns2 || ret=1
$DIG $DIGOPTS a.example @10.53.0.3 > dig.out.ns3 || ret=1
grep "status: NOERROR" dig.out.ns2 > /dev/null || ret=1
grep "status: SERVFAIL" dig.out.ns3 > /dev/null || ret=1
if [ $ret != 0 ]; then echo_i "failed"; fi
status=`expr $status + $ret`

echo_i "exit status: $status"
[ $status -eq 0 ] || exit 1
