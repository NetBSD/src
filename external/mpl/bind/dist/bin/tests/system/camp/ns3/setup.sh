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

echo_i "ns3/setup.sh"

cp template.db.in final.tld0.db
echo "q.final.tld0. IN A 1.2.3.4" >>final.tld0.db

DEPTH=5

tld=1
while [ $tld -le $DEPTH ]; do
  nexttld=$((tld + 1))

  label=1
  while [ $label -le $DEPTH ]; do
    nextlabel=$((label + 1))

    cat >>"named.conf" <<EOF
zone "label${label}.tld${tld}" {
        type primary;
        file "label${label}.tld${tld}.db";
};
EOF

    cp template.db.in label${label}.tld${tld}.db

    if [ $label -eq $DEPTH ] && [ $tld -eq $DEPTH ]; then
      echo "q.label${label}.tld${tld}. IN CNAME q.goto1.tld1." >>label${label}.tld${tld}.db
    elif [ $tld -eq $DEPTH ]; then
      nextlabel=$((label + 1))
      echo "q.label${label}.tld${tld}. IN CNAME q.label${nextlabel}.tld1." >>label${label}.tld${tld}.db
    else
      echo "q.label${label}.tld${tld}. IN CNAME q.label${label}.tld${nexttld}." >>label${label}.tld${tld}.db
    fi

    label=$nextlabel
  done

  echo "" >>label${label}.tld${tld}.db
  tld=$nexttld
done

goto=1
tld=1
while [ $goto -le $DEPTH ]; do
  nextgoto=$((goto + 1))

  cat >>"named.conf" <<EOF
zone "goto${goto}.tld${tld}" {
        type primary;
        file "goto${goto}.tld${tld}.db";
};
EOF

  cp template.db.in goto${goto}.tld${tld}.db

  if [ $goto -eq $DEPTH ]; then
    echo "q.goto${goto}.tld${tld}. IN CNAME q.final.tld0." >>goto${goto}.tld${tld}.db
  else
    echo "q.goto${goto}.tld${tld}. IN CNAME q.goto${nextgoto}.tld${tld}." >>goto${goto}.tld${tld}.db
  fi

  echo "" >>label${label}.tld${tld}.db
  goto=$nextgoto
done
