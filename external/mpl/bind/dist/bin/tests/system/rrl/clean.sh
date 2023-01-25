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

# Clean up after rrl tests.

rm -f dig.out* *mdig.out*
rm -f  */named.memstats */named.run */named.stats */log-* */session.key
rm -f ns3/bl*.db */*.jnl */*.core */*.pid
rm -f ns*/named.lock
rm -f ns*/named.conf
rm -f broken.conf
rm -f broken.out
rm -f ns*/managed-keys.bind*
