#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

. ../conf.sh

cp -f ns1/example1.db ns1/example.db
sed 's/example.nil/other.nil/g' ns1/example1.db >ns1/other.db
sed 's/example.nil/unixtime.nil/g' ns1/example1.db >ns1/unixtime.db
sed 's/example.nil/yyyymmddvv.nil/g' ns1/example1.db >ns1/yyyymmddvv.db
sed 's/example.nil/keytests.nil/g' ns1/example1.db >ns1/keytests.db
cp -f ns3/example.db.in ns3/example.db
cp -f ns3/relaxed.db.in ns3/relaxed.db
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

$TSIGKEYGEN ddns-key.example.nil >ns1/ddns.key

if $FEATURETEST --md5; then
  $TSIGKEYGEN -a hmac-md5 md5-key >ns1/md5.key
else
  echo "/* MD5 NOT SUPPORTED */" >ns1/md5.key
fi
$TSIGKEYGEN -a hmac-sha1 sha1-key >ns1/sha1.key
$TSIGKEYGEN -a hmac-sha224 sha224-key >ns1/sha224.key
$TSIGKEYGEN -a hmac-sha256 sha256-key >ns1/sha256.key
$TSIGKEYGEN -a hmac-sha384 sha384-key >ns1/sha384.key
$TSIGKEYGEN -a hmac-sha512 sha512-key >ns1/sha512.key

if $FEATURETEST --md5; then
  echo 'key "legacy-157" { algorithm "hmac-md5"; secret "mGcDSCx/fF121GOVJlITLg=="; };' >ns1/legacy157.key
else
  echo "/* MD5 NOT SUPPORTED */" >ns1/legacy157.key
fi
echo 'key "legacy-161" { algorithm "hmac-sha1"; secret "N80fGvcr8JifzRUJ62R4rQ=="; };' >ns1/legacy161.key
echo 'key "legacy-162" { algorithm "hmac-sha224"; secret "nSIKzFAGS7/tvBs8JteI+Q=="; };' >ns1/legacy162.key
echo 'key "legacy-163" { algorithm "hmac-sha256"; secret "CvaupxnDeES3HnlYhTq53w=="; };' >ns1/legacy163.key
echo 'key "legacy-164" { algorithm "hmac-sha384"; secret "wDldBJwJrYfPoL1Pj4ucOQ=="; };' >ns1/legacy164.key
echo 'key "legacy-165" { algorithm "hmac-sha512"; secret "OgZrTcEa8P76hVY+xyN7Wg=="; };' >ns1/legacy165.key

(
  cd ns3
  $SHELL -e sign.sh
)

cp -f ns1/many.test.db.in ns1/many.test.db

cp ns1/sample.db.in ns1/sample.db
cp ns2/sample.db.in ns2/sample.db

cp -f ns1/maxjournal.db.in ns1/maxjournal.db

cp -f ns5/local.db.in ns5/local.db
cp -f ns6/2.0.0.2.ip6.addr.db.in ns6/2.0.0.2.ip6.addr.db
cp -f ns6/in-addr.db.in ns6/in-addr.db
cp -f ns7/in-addr.db.in ns7/in-addr.db
cp -f ns7/example.com.db.in ns7/example.com.db
cp -f ns8/in-addr.db.in ns8/in-addr.db
cp -f ns8/example.com.db.in ns8/example.com.db
cp -f ns9/in-addr.db.in ns9/in-addr.db
cp -f ns9/example.com.db.in ns9/example.com.db
cp -f ns9/example.com.db.in ns9/denyname.example.db
cp -f ns10/in-addr.db.in ns10/in-addr.db
cp -f ns10/example.com.db.in ns10/example.com.db
