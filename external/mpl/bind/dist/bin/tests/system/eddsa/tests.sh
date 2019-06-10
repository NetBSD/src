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

DIGOPTS="+tcp +noau +noadd +nosea +nostat +nocmd +dnssec -p 5300"

# Check the example. domain

echo "I:checking that positive validation works ($n)"
ret=0
$DIG $DIGOPTS . @10.53.0.1 soa > dig.out.ns1.test$n || ret=1
$DIG $DIGOPTS . @10.53.0.2 soa > dig.out.ns2.test$n || ret=1
$PERL ../digcomp.pl dig.out.ns1.test$n dig.out.ns2.test$n || ret=1
grep "flags:.*ad.*QUERY" dig.out.ns2.test$n > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

# Check test vectors (RFC 8080 + errata)

echo "I:checking that test vectors match ($n)"
ret=0
grep 'oL9krJun7xfBOIWcGHi7mag5/hdZrKWw15jP' ns2/example.com.db.signed > /dev/null || ret=1
grep 'VrbpMngwcrqNAg==' ns2/example.com.db.signed > /dev/null || ret=1
grep 'zXQ0bkYgQTEFyfLyi9QoiY6D8ZdYo4wyUhVi' ns2/example.com.db.signed > /dev/null || ret=1
grep 'R0O7KuI5k2pcBg==' ns2/example.com.db.signed > /dev/null || ret=1
n=`expr $n + 1`
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
