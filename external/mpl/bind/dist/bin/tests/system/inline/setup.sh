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

cp ns1/root.db.in ns1/root.db
rm -f ns1/root.db.signed

touch ns2/trusted.conf
cp ns2/bits.db.in ns2/bits.db
cp ns2/bits.db.in ns2/inactiveksk.db
cp ns2/bits.db.in ns2/inactivezsk.db
cp ns2/bits.db.in ns2/nokeys.db
cp ns2/bits.db.in ns2/removedkeys-secondary.db
cp ns2/bits.db.in ns2/retransfer.db
cp ns2/bits.db.in ns2/retransfer3.db
rm -f ns2/bits.db.jnl

cp ns3/master.db.in ns3/master.db
cp ns3/master.db.in ns3/dynamic.db
cp ns3/master.db.in ns3/updated.db
cp ns3/master.db.in ns3/expired.db
cp ns3/master.db.in ns3/nsec3.db
cp ns3/master.db.in ns3/externalkey.db
cp ns3/master.db.in ns3/removedkeys-primary.db

mkdir ns3/removedkeys

touch ns4/trusted.conf
cp ns4/noixfr.db.in ns4/noixfr.db
rm -f ns4/noixfr.db.jnl

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns4/named.conf.in ns4/named.conf
copy_setports ns5/named.conf.pre ns5/named.conf
copy_setports ns6/named.conf.in ns6/named.conf
copy_setports ns7/named.conf.in ns7/named.conf

(cd ns3; $SHELL -e sign.sh)
(cd ns1; $SHELL -e sign.sh)
(cd ns7; $SHELL -e sign.sh)
