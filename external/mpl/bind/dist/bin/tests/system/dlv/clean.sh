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

rm -f ns*/named.run
rm -f ns*/named.conf
rm -f ns1/K*
rm -f ns1/dsset-*
rm -f ns1/*.signed
rm -f ns1/signer.err
rm -f ns1/root.db
rm -f ns1/trusted.conf
rm -f ns2/K*
rm -f ns2/dlvset-*
rm -f ns2/dsset-*
rm -f ns2/*.signed
rm -f ns2/*.pre
rm -f ns2/signer.err
rm -f ns2/druz.db
rm -f ns3/K*
rm -f ns3/*.db
rm -f ns3/*.signed ns3/*.signed.tmp
rm -f ns3/dlvset-*
rm -f ns3/dsset-*
rm -f ns3/keyset-*
rm -f ns3/trusted*.conf
rm -f ns3/signer.err
rm -f ns5/trusted*.conf
rm -f ns6/K*
rm -f ns6/*.db
rm -f ns6/*.signed
rm -f ns6/dsset-*
rm -f ns6/signer.err
rm -f ns7/trusted*.conf ns8/trusted*.conf
rm -f */named.memstats
rm -f dig.out.ns*.test*
rm -f ns*/named.lock
rm -f ns*/managed-keys.bind*
