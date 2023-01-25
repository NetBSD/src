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

rm -f dig.out*
rm -f ns1/named.conf
rm -f ns1/named.lock
rm -f ns1/named.memstats
rm -f ns1/named.run
rm -f ns1/signing.test.db
rm -f ns1/signing.test.db.jbk
rm -f ns1/signing.test.db.signed
rm -f ns1/signing.test.db.signed.jnl
rm -f ns1/keys/signing.test/K*
rm -f ns1/managed-keys.bind*
