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

cp ns1/generic.db.in ns1/changed.db
cp ns1/changed.ver1.jnl.saved ns1/changed.db.jnl

cp ns1/generic.db.in ns1/unchanged.db
cp ns1/unchanged.ver1.jnl.saved ns1/unchanged.db.jnl

cp ns1/generic.db.in ns1/changed2.db
cp ns1/changed.ver2.jnl.saved ns1/changed2.db.jnl

cp ns1/generic.db.in ns1/unchanged2.db
cp ns1/unchanged.ver2.jnl.saved ns1/unchanged2.db.jnl

cp ns1/ixfr.db.in ns1/ixfr.db
cp ns1/ixfr.ver1.jnl.saved ns1/ixfr.db.jnl

cp ns1/generic.db.in ns1/d1212.db
cp ns1/d1212.jnl.saved ns1/d1212.db.jnl

cp ns1/generic.db.in ns1/d2121.db
cp ns1/d2121.jnl.saved ns1/d2121.db.jnl

cp ns1/generic.db.in ns1/maxjournal.db
cp ns1/maxjournal.jnl.saved ns1/maxjournal.db.jnl

cp ns1/generic.db.in ns1/maxjournal2.db
cp ns1/maxjournal2.jnl.saved ns1/maxjournal2.db.jnl

cp ns1/managed-keys.bind.in ns1/managed-keys.bind
$PERL ../fromhex.pl <ns1/managed-keys.bind.jnl.in >ns1/managed-keys.bind.jnl

cp ns2/managed-keys.bind.in ns2/managed-keys.bind
cp ns2/managed-keys.bind.jnl.in ns2/managed-keys.bind.jnl
