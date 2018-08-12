# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

test -r $RANDFILE || $GENRANDOM 800 $RANDFILE

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf

rm -f named-compilezone
ln -s $CHECKZONE named-compilezone

rm -f ns1/example.db.raw
cp ns1/example.db ns2/
cp ns2/formerly-text.db.in ns2/formerly-text.db
cp ns1/large.db.in ns1/large.db
awk 'END {
	 for (i = 0; i < 512; i++ ) { print "a TXT", i; }
	 for (i = 0; i < 1024; i++ ) { print "b TXT", i; }
	 for (i = 0; i < 2000; i++ ) { print "c TXT", i; }
}' < /dev/null >> ns1/large.db
cd ns1 && $SHELL compile.sh
