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

# Set up db files for zone "test" - this is a series of four
# versions of the zone, the second and third having small changes
# and the fourth having a large one.

testdb() {
  cat <<EOF
\$ORIGIN $1
\$TTL    15
@  15  IN        SOA ns1.test. hostmaster.test. (
                              $2 ; serial
                              3H ; refresh
                              15 ; retry
                              1w ; expire
                              3h ; minimum
                             )
       IN  NS     ns1.test.
       IN  NS     ns2.test.
       IN  NS     ns5.test.
ns1    IN  A      10.53.0.3
ns2    IN  A      10.53.0.4
ns5    IN  A      10.53.0.5
EOF

  i=0
  while [ $i -lt $3 ]; do
    echo "host$i IN A 192.0.2.$i"
    i=$((i + 1))
  done
}

testdb test. 1 60 >ns3/mytest.db
testdb test. 2 61 >ns3/mytest1.db
testdb test. 3 62 >ns3/mytest2.db
testdb test. 4 0 >ns3/mytest3.db

# Set up similar db files for sub.test, which will have IXFR disabled
testdb sub.test. 1 60 >ns3/subtest.db
testdb sub.test. 3 61 >ns3/subtest1.db

# Set up a large zone
i=0
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 3 >ns3/large.db
while [ $i -lt 10000 ]; do
  echo "record$i 10 IN TXT this is record %i" >>ns3/large.db
  i=$((i + 1))
done
