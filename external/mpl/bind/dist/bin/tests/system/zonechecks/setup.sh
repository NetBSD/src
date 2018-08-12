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

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf

$SHELL ../genzone.sh 1 > ns1/master.db
$SHELL ../genzone.sh 1 > ns1/duplicate.db
cp bigserial.db ns1/
cd ns1
touch master.db.signed
echo '$INCLUDE "master.db.signed"' >> master.db
$KEYGEN -r $RANDFILE -a rsasha256 -q master.example > /dev/null 2>&1
$KEYGEN -r $RANDFILE -a rsasha256 -qfk master.example > /dev/null 2>&1
$SIGNER -SD -o master.example master.db > /dev/null \
    2> signer.err || cat signer.err
echo '$INCLUDE "soa.db"' > reload.db
echo '@ 0 NS .' >> reload.db
echo '@ 0 SOA . . 1 0 0 0 0' > soa.db
