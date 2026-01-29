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

#
# Generate a large enough zone, so the transfer takes longer than
# tcp-initial-timeout interval
#
$PYTHON -c "
print('large IN TXT', end=' ')
for a in range(128):
    print('\"%s\"' % ('A' * 240), end=' ')
print('')

for a in range(150000):
    print('%s IN NS a' % (a))
    print('%s IN NS b' % (a))" >ns1/large.db
