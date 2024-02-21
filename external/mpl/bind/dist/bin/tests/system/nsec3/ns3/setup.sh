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

setup() {
  zone="$1"
  echo_i "setting up zone: $zone"
  zonefile="${zone}.db"
  infile="${zone}.db.infile"
  cp template.db.in "$zonefile"
}

for zn in nsec-to-nsec3 nsec3 nsec3-other nsec3-change nsec3-to-nsec \
  nsec3-to-optout nsec3-from-optout nsec3-dynamic \
  nsec3-dynamic-change nsec3-dynamic-to-inline \
  nsec3-inline-to-dynamic nsec3-dynamic-update-inline; do
  setup "${zn}.kasp"
done

if (
  cd ..
  $SHELL ../testcrypto.sh -q RSASHA1
); then
  for zn in rsasha1-to-nsec3 rsasha1-to-nsec3-wait nsec3-to-rsasha1 \
    nsec3-to-rsasha1-ds; do
    setup "${zn}.kasp"
  done

  longago="now-1y"
  keytimes="-P ${longago} -A ${longago}"
  O="omnipresent"

  zone="rsasha1-to-nsec3-wait.kasp"
  CSK=$($KEYGEN -k "rsasha1" -l named.conf $keytimes $zone 2>keygen.out.$zone)
  echo_i "Created key file $CSK"
  $SETTIME -s -g $O -k $O $longago -r $O $longago -z $O $longago -d $O $longago "$CSK" >settime.out.$zone 2>&1

  zone="nsec3-to-rsasha1-ds.kasp"
  CSK=$($KEYGEN -k "default" -l named.conf $keytimes $zone 2>keygen.out.$zone)
  echo_i "Created key file $CSK"
  $SETTIME -s -g $O -k $O $longago -r $O $longago -z $O $longago -d $O $longago "$CSK" >settime.out.$zone 2>&1
else
  echo_i "skip: skip rsasha1 zones - signing with RSASHA1 not supported"
fi

cp nsec3-fails-to-load.kasp.db.in nsec3-fails-to-load.kasp.db
