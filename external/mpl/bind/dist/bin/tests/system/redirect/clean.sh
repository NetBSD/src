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

rm -f */named.conf
rm -f */named.memstats
rm -f */named.run
rm -f */named.stats
rm -f dig.out.*
rm -f ns*/named.lock
rm -f ns1/K*
rm -f ns1/dsset-nsec3.
rm -f ns1/dsset-signed.
rm -f ns1/nsec3.db*
rm -f ns1/signed.db*
rm -f ns2/*.db
rm -f ns3/K*
rm -f ns3/dsset-nsec3.
rm -f ns3/dsset-signed.
rm -f ns3/nsec3.db*
rm -f ns3/signed.db*
rm -f ns4/*.db
rm -f ns5/dsset-*
rm -f ns5/K* ns5/sign.ns5.*
rm -f ns5/root.db ns5/root.db.signed
rm -f ns5/signed.db ns5/signed.db.signed
rm -f ns6/signed.db.signed
rm -f rndc.out
rm -f ns*/managed-keys.bind*
