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

echo_i "ns9/setup.sh"

setup() {
  zone="$1"
  echo_i "setting up zone: $zone"
  zonefile="${zone}.db"
  infile="${zone}.db.infile"
  echo "$zone" >>zones
}

sign_dspublish() {
  cp template.db.in "$zonefile"
  keytimes="-P $T -P sync $T -A $T"
  CSK=$($KEYGEN -k default $keytimes $zone 2>keygen.out.$zone)
  $SETTIME -s -g $O -k $O $T -r $O $T -z $O $T -d $R $T "$CSK" >settime.out.$zone 2>&1
  cat "$zonefile" "${CSK}.key" >"$infile"
  private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >>"$infile"
  cp $infile $zonefile
  $SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1
  cp "dsset-${zone}." ../ns2/
}

sign_dsremoved() {
  cp template.db.in "$zonefile"
  keytimes="-P $Y -P sync $Y -A $Y"
  CSK=$($KEYGEN -k default $keytimes $zone 2>keygen.out.$zone)
  $SETTIME -s -g $H -k $O $T -r $O $T -z $O $T -d $U $T "$CSK" >settime.out.$zone 2>&1
  cat "$zonefile" "${CSK}.key" >"$infile"
  private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >>"$infile"
  cp $infile $zonefile
  $SIGNER -S -z -x -s now-1h -e now+30d -o $zone -O raw -f "${zonefile}.signed" $infile >signer.out.$zone.1 2>&1
  cp "dsset-${zone}." ../ns2/
}

# Short environment variable names for key states and times.
H="HIDDEN"
R="RUMOURED"
O="OMNIPRESENT"
U="UNRETENTIVE"
T="now-30d"
Y="now-1y"

# DS Publication.
for checkds in explicit yes no; do
  for zn in \
    good.${checkds}.dspublish.ns2 \
    reference.${checkds}.dspublish.ns2 \
    resolver.${checkds}.dspublish.ns2 \
    not-yet.${checkds}.dspublish.ns5 \
    bad.${checkds}.dspublish.ns6 \
    good.${checkds}.dspublish.ns2-4 \
    incomplete.${checkds}.dspublish.ns2-4-5 \
    bad.${checkds}.dspublish.ns2-4-6; do
    setup "${zn}"
    sign_dspublish
  done
done

# DS Withdrawal.
for checkds in explicit yes no; do
  for zn in \
    good.${checkds}.dsremoved.ns5 \
    resolver.${checkds}.dsremoved.ns5 \
    still-there.${checkds}.dsremoved.ns2 \
    bad.${checkds}.dsremoved.ns6 \
    good.${checkds}.dsremoved.ns5-7 \
    incomplete.${checkds}.dsremoved.ns2-5-7 \
    bad.${checkds}.dsremoved.ns5-6-7; do
    setup "${zn}"
    sign_dsremoved
  done
done

setup "no-ent.ns2"
sign_dspublish

setup "no-ent.ns5"
sign_dsremoved
