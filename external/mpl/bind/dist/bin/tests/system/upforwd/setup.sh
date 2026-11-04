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

cp -f ns1/example1.db ns1/example.db
cp -f ns1/example3.db.in ns1/example3.db
cp -f ns3/noprimary.db ns3/noprimary1.db

if $FEATURETEST --enable-dnstap; then
  cat <<'EOF' >ns3/dnstap.conf
	dnstap-identity "ns3";
	dnstap-version "xxx";
	dnstap-output file "dnstap.out";
	dnstap { all; };
EOF
else
  echo "/* DNSTAP NOT ENABLED */" >ns3/dnstap.conf
fi

#
# SIG(0) requires cryptographic support which may not be configured.
#
keyname=$($KEYGEN -q -n HOST -a ${DEFAULT_ALGORITHM} -T KEY sig0.example2 2>keyname.err)
if test -n "$keyname"; then
  cat ns1/example1.db $keyname.key >ns1/example2.db
  echo $keyname >keyname
else
  cat ns1/example1.db >ns1/example2.db
fi
cat_i <keyname.err

cat ns1/example1.db >ns1/example2-toomanykeys.db
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17; do
  keyname=$($KEYGEN -q -n HOST -a ${DEFAULT_ALGORITHM} -T KEY sig0.example2-toomanykeys 2>/dev/null)
  if test -n "$keyname"; then
    cat $keyname.key >>ns1/example2-toomanykeys.db
    echo $keyname >keyname$i
  fi
done
