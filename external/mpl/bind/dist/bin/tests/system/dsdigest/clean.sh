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

rm -f supported
rm -f */K* */dsset-* */*.signed */trusted.conf
rm -f ns1/root.db
rm -f ns1/signer.err
rm -f ns2/good.db ns2/bad.db
rm -f dig.out*
rm -f */named.conf
rm -f */named.run
rm -f */named.memstats
rm -f ns*/named.lock
