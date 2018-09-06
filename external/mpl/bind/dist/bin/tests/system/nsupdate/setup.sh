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

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

$SHELL clean.sh
copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf
copy_setports ns5/named.conf.in ns5/named.conf

copy_setports verylarge.in verylarge

#
# jnl and database files MUST be removed before we start
#

rm -f ns1/*.jnl ns1/example.db ns2/*.jnl ns2/example.bk
rm -f ns2/update.bk ns2/update.alt.bk
rm -f ns3/example.db.jnl
rm -f ns3/too-big.test.db.jnl

cp -f ns1/example1.db ns1/example.db
sed 's/example.nil/other.nil/g' ns1/example1.db > ns1/other.db
sed 's/example.nil/unixtime.nil/g' ns1/example1.db > ns1/unixtime.db
sed 's/example.nil/yyyymmddvv.nil/g' ns1/example1.db > ns1/yyyymmddvv.db
sed 's/example.nil/keytests.nil/g' ns1/example1.db > ns1/keytests.db
cp -f ns3/example.db.in ns3/example.db
cp -f ns3/too-big.test.db.in ns3/too-big.test.db

# update_test.pl has its own zone file because it
# requires a specific NS record set.
cat <<\EOF >ns1/update.db
$ORIGIN .
$TTL 300        ; 5 minutes
update.nil              IN SOA  ns1.example.nil. hostmaster.example.nil. (
                                1          ; serial
                                2000       ; refresh (2000 seconds)
                                2000       ; retry (2000 seconds)
                                1814400    ; expire (3 weeks)
                                3600       ; minimum (1 hour)
                                )
update.nil.             NS      ns1.update.nil.
ns1.update.nil.         A       10.53.0.2
ns2.update.nil.		AAAA	::1
EOF

$DDNSCONFGEN -q -r $RANDFILE -z example.nil > ns1/ddns.key

$DDNSCONFGEN -q -r $RANDFILE -a hmac-md5 -k md5-key -z keytests.nil > ns1/md5.key
$DDNSCONFGEN -q -r $RANDFILE -a hmac-sha1 -k sha1-key -z keytests.nil > ns1/sha1.key
$DDNSCONFGEN -q -r $RANDFILE -a hmac-sha224 -k sha224-key -z keytests.nil > ns1/sha224.key
$DDNSCONFGEN -q -r $RANDFILE -a hmac-sha256 -k sha256-key -z keytests.nil > ns1/sha256.key
$DDNSCONFGEN -q -r $RANDFILE -a hmac-sha384 -k sha384-key -z keytests.nil > ns1/sha384.key
$DDNSCONFGEN -q -r $RANDFILE -a hmac-sha512 -k sha512-key -z keytests.nil > ns1/sha512.key

(cd ns3; $SHELL -e sign.sh)

cp -f ns1/many.test.db.in ns1/many.test.db
rm -f ns1/many.test.db.jnl

cp ns1/sample.db.in ns1/sample.db
cp ns2/sample.db.in ns2/sample.db

cp -f ns1/maxjournal.db.in ns1/maxjournal.db
rm -f ns1/maxjournal.db.jnl

cp -f ns5/local.db.in ns5/local.db
