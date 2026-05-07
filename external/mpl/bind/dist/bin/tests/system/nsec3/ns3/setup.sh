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
  cp template.db.in "$zonefile"
}

for zn in nsec-to-nsec3 nsec3 nsec3-other nsec3-change nsec3-to-nsec \
  nsec3-to-optout nsec3-from-optout nsec3-dynamic \
  nsec3-dynamic-change nsec3-dynamic-to-inline \
  nsec3-inline-to-dynamic nsec3-dynamic-update-inline; do
  setup "${zn}.kasp"
done

if [ $RSASHA1_SUPPORTED = 1 ]; then
  longago="now-1y"
  keytimes="-P ${longago} -A ${longago} -P sync ${longago}"
  O="omnipresent"

  for zn in nsec3-to-rsasha1 nsec3-to-rsasha1-ds; do
    setup "${zn}.kasp"
    CSK=$($KEYGEN -a $DEFAULT_ALGORITHM -L 3600 -f KSK $keytimes $zone 2>keygen.out.$zone)
    $SETTIME -s -g $O -k $O $longago -r $O $longago -z $O $longago -d $O $longago "$CSK" >settime.out.$zone 2>&1
    cat $CSK.key >>$zonefile
  done

  for zn in rsasha1-to-nsec3 rsasha1-to-nsec3-wait; do
    setup "${zn}.kasp"
    CSK=$($KEYGEN -k "rsasha1" -l named.conf $keytimes $zone 2>keygen.out.$zone)
    $SETTIME -s -g $O -k $O $longago -r $O $longago -z $O $longago -d $O $longago "$CSK" >settime.out.$zone 2>&1
    cat $CSK.key >>$zonefile
  done
else
  echo_i "skip: skip rsasha1 zones - signing with RSASHA1 not supported"
fi

cp nsec3-fails-to-load.kasp.db.in nsec3-fails-to-load.kasp.db
