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

set -e

rm -f ./*/named.memstats
rm -f ./*/named.conf
rm -f ./*/named.run
rm -f ./*/named.run.prev
rm -f ./*/named.stats
rm -f ./dig.out.*
rm -f ./ns1/K*+*+*.key
rm -f ./ns1/K*+*+*.private
rm -f ./ns1/dsset-*
rm -f ./ns1/example.db
rm -f ./ns1/example.db.signed
rm -f ./ns1/insecure.example.db
rm -f ./ns1/insecure.example.db.signed
rm -f ./ns1/dnamed.db
rm -f ./ns1/dnamed.db.signed
rm -f ./ns1/minimal.db
rm -f ./ns1/minimal.db.signed
rm -f ./ns1/root.db
rm -f ./ns1/root.db.signed
rm -f ./ns1/soa-without-dnskey.db
rm -f ./ns1/soa-without-dnskey.db.signed
rm -f ./ns1/trusted.conf
rm -f ./ns2/named_dump.db
rm -f ./ns*/managed-keys.bind*
rm -f ./nodata.out ./insecure.nodata.out
rm -f ./nxdomain.out ./insecure.nxdomain.out
rm -f ./wild.out ./insecure.wild.out
rm -f ./wildcname.out ./insecure.wildcname.out
rm -f ./wildnodata1nsec.out ./insecure.wildnodata1nsec.out
rm -f ./wildnodata2nsec.out ./insecure.wildnodata2nsec.out
rm -f ./wildnodata2nsecafterdata.out ./insecure.wildnodata2nsecafterdata.out
rm -f ./minimal.nxdomain.out
rm -f ./black.out
rm -f ./xml.out*
rm -f ./json.out*
