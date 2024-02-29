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

#
# Clean up after zone transfer tests.
#

rm -f */*.jnl
rm -f */named.conf
rm -f */named.memstats
rm -f */named.run */ans.run
rm -f */named.run.prev
rm -f Kxxx.*
rm -f check.out.*
rm -f dig.out.*
rm -f jp.out.ns3.*
rm -f nextpart.out.*
rm -f ns*/managed-keys.bind* ns*/*.mkeys*
rm -f ns*/named.lock
rm -f ns1/example.db ns1/unixtime.db ns1/yyyymmddvv.db ns1/update.db ns1/other.db ns1/keytests.db
rm -f ns1/many.test.db
rm -f ns1/maxjournal.db
rm -f ns1/md5.key ns1/sha1.key ns1/sha224.key ns1/sha256.key ns1/sha384.key
rm -f ns1/legacy157.key ns1/legacy161.key ns1/legacy162.key ns1/legacy163.key ns1/legacy164.key ns1/legacy165.key
rm -f ns1/sample.db
rm -f ns1/sha512.key ns1/ddns.key
rm -f ns10/_default.tsigkeys
rm -f ns10/example.com.db
rm -f ns10/in-addr.db
rm -f ns2/example.bk
rm -f ns2/sample.db
rm -f ns2/update.bk ns2/update.alt.bk
rm -f ns3/*.signed
rm -f ns3/K*
rm -f ns3/delegation.test.db
rm -f ns3/dnskey.test.db
rm -f ns3/dsset-*
rm -f ns3/example.db
rm -f ns3/multisigner.test.db
rm -f ns3/many.test.bk
rm -f ns3/nsec3param.test.db
rm -f ns3/too-big.test.db
rm -f ns5/local.db
rm -f ns6/in-addr.db
rm -f ns7/_default.tsigkeys
rm -f ns7/example.com.db
rm -f ns7/in-addr.db
rm -f ns8/_default.tsigkeys
rm -f ns8/example.com.db
rm -f ns8/in-addr.db
rm -f ns9/_default.tsigkeys
rm -f ns9/denyname.example.db
rm -f ns9/example.com.db
rm -f ns9/in-addr.db
rm -f perl.update_test.out
rm -f nsupdate.alg-*
rm -f nsupdate.out*
rm -f typelist.out.*
rm -f update.out.*
rm -f update.in.*
rm -f verylarge
