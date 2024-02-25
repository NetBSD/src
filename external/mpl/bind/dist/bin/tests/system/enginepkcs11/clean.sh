#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. ../conf.sh

set -e

rm -f dig.out.*
rm -f dsset-*
rm -f pin
rm -f keyfromlabel.err.* keyfromlabel.out.*
rm -f pkcs11-tool.err.* pkcs11-tool.out.*
rm -f signer.out.*
rm -f ns1/*.example.db ns1/*.example.db.signed
rm -f ns1/*.kskid1 ns1/*.kskid2 ns1/*.zskid1 ns1/*.zskid2
rm -f ns1/dig.out.*
rm -f ns1/K*
rm -f ns1/named.conf ns1/named.run ns1/named.memstats
rm -f ns1/update.cmd.*
rm -f ns1/update.log.*
rm -f ns1/verify.out.*
rm -f ns1/zone.*.signed.jnl ns1/zone.*.signed.jbk

softhsm2-util --delete-token --token "softhsm2-enginepkcs11" >/dev/null 2>&1 || echo_i "softhsm2-enginepkcs11 token not found for cleaning"
