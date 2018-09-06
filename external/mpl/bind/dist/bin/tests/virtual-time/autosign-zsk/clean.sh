# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

#
# Clean up after virtual time tests.
#
rm -f */K* */dsset-* */*.signed */*.jnl */tmp*
rm -f dig.out.*
rm -f random.data*
rm -f */named.memstats
rm -f */*vtwrapper.*
rm -f ns1/example.db
rm -f ns1/keyname
rm -f ns*/named.lock
