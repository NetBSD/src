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

# shellcheck source=conf.sh
. ../conf.sh

cp ns1/example.db ns2/
cp ns2/formerly-text.db.in ns2/formerly-text.db
cp ns1/empty.db.in ns1/under-limit.db

# counts are set with respect to these limits in named.conf:
#	max-records-per-type 2050;
#	max-types-per-name 500;
awk 'END {
	 for (i = 0; i < 500; i++ ) { print "500-txt TXT", i; }
	 for (i = 0; i < 1000; i++ ) { print "1000-txt TXT", i; }
	 for (i = 0; i < 2000; i++ ) { print "2000-txt TXT", i; }
}' </dev/null >>ns1/under-limit.db
cp ns1/under-limit.db ns1/under-limit-kasp.db

cp ns1/empty.db.in ns1/below-limit.db
awk 'END {
	 for (i = 0; i < 500; i++ ) { print "500-txt TXT", i; }
	 for (i = 0; i < 1000; i++ ) { print "1000-txt TXT", i; }
	 for (i = 0; i < 2000; i++ ) { print "2000-txt TXT", i; }
	 for (i = 0; i < 2050; i++ ) { print "2050-txt TXT", i; }
}' </dev/null >>ns1/below-limit.db
cp ns1/below-limit.db ns1/below-limit-kasp.db

cp ns1/empty.db.in ns1/above-limit.db
awk 'END {
	 for (i = 0; i < 500; i++ ) { print "500-txt TXT", i; }
	 for (i = 0; i < 1000; i++ ) { print "1000-txt TXT", i; }
	 for (i = 0; i < 2000; i++ ) { print "2000-txt TXT", i; }
	 for (i = 0; i < 2050; i++ ) { print "2050-txt TXT", i; }
	 for (i = 0; i < 2100; i++ ) { print "2100-txt TXT", i; }
}' </dev/null >>ns1/above-limit.db

cp ns1/empty.db.in ns1/255types.db
for ntype in $(seq 65280 65534); do
  echo "m TYPE${ntype} \# 0"
done >>ns1/255types.db
echo "m TXT bunny" >>ns1/255types.db
(cd ns1 && $SHELL compile.sh)
(cd ns4 && $SHELL compile.sh)
