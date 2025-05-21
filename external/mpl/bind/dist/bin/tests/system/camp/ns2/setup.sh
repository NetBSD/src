#!/bin/sh -e

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
. ../../conf.sh

echo_i "ns2/setup.sh"

cp template.db.in tld0.db
echo "final.tld0. IN NS ns.final.tld0." >>tld0.db
echo "ns.final.tld0. IN A 10.53.0.3" >>tld0.db

DEPTH=5

tld=1
while [ $tld -le $DEPTH ]; do

  cat >>"named.conf" <<EOF
zone "tld${tld}" {
        type primary;
        file "tld${tld}.db";
};
EOF

  cp template.db.in tld${tld}.db

  label=0
  while [ $label -le $DEPTH ]; do
    echo "label${label}.tld${tld}. IN NS ns.label${label}.tld${tld}." >>tld${tld}.db
    echo "ns.label${label}.tld${tld}. IN A 10.53.0.3" >>tld${tld}.db
    echo "" >>tld${tld}.db

    label=$((label + 1))
  done

  tld=$((tld + 1))
done

goto=1
tld=1
while [ $goto -le $DEPTH ]; do
  echo "goto${goto}.tld${tld}. IN NS ns.goto${goto}.tld${tld}." >>tld${tld}.db
  echo "ns.goto${goto}.tld${tld}. IN A 10.53.0.3" >>tld${tld}.db
  echo "" >>tld${tld}.db

  goto=$((goto + 1))
done
