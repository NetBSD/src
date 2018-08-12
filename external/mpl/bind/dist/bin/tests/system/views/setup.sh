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

$SHELL clean.sh

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

cp -f ns2/example1.db ns2/example.db
rm -f ns2/external/K*
rm -f ns2/external/inline.db.signed
rm -f ns2/external/inline.db.signed.jnl
rm -f ns2/internal/K*
rm -f ns2/internal/inline.db.signed
rm -f ns2/internal/inline.db.signed.jnl

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named1.conf.in ns2/named.conf
copy_setports ns3/named1.conf.in ns3/named.conf
copy_setports ns5/named.conf.in ns5/named.conf

#
# We remove k1 and k2 as KEYGEN is deterministic when given the
# same source of "random" data and we want different keys for
# internal and external instances of inline.
#
$KEYGEN -K ns2/internal -r $RANDFILE -a rsasha256 -q inline > /dev/null 2>&1
$KEYGEN -K ns2/internal -r $RANDFILE -a rsasha256 -qfk inline > /dev/null 2>&1
k1=`$KEYGEN -K ns2/external -r $RANDFILE -a rsasha256 -q inline 2> /dev/null`
k2=`$KEYGEN -K ns2/external -r $RANDFILE -a rsasha256 -qfk inline 2> /dev/null`
$KEYGEN -K ns2/external -r $RANDFILE -a rsasha256 -q inline > /dev/null 2>&1
$KEYGEN -K ns2/external -r $RANDFILE -a rsasha256 -qfk inline > /dev/null 2>&1
test -n "$k1" && rm -f ns2/external/$k1.*
test -n "$k2" && rm -f ns2/external/$k2.*
