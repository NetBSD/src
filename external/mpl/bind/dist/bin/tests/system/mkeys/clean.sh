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

rm -f */K* */*.signed */trusted.conf */*.jnl */*.bk
rm -f */managed*.conf ns1/managed.key ns1/managed.key.id
rm -f */managed-keys.bind* */named.secroots
rm -f */named.conf
rm -f ns3/broken.conf
rm -f */named.memstats */named.run */named.run.prev
rm -f dig.out* delv.out* rndc.out* signer.out*
rm -f dsset-. ns1/dsset-.
rm -f ns1/zone.key
rm -f ns*/managed-keys.bind*
rm -f ns*/named.lock
rm -f ns1/named.secroots ns1/root.db.signed* ns1/root.db.tmp
rm -f ns5/named.args
rm -f ns7/view1.mkeys ns7/view2.mkeys
rm -rf ns4/nope
