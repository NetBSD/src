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

echo_i "ns5/setup.sh"

for zn in \
  ns2 ns2-4 ns2-4-5 ns2-4-6 ns2-5-7 \
  ns5 ns5-6-7 ns5-7 ns6; do
  zone="${zn}"
  infile="${zn}.db.infile"
  zonefile="${zn}.db"

  CSK=$($KEYGEN -k default $zone 2>keygen.out.$zone)
  cat "${zn}.db.in" "${CSK}.key" >"$infile"
  private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >>"$infile"
  $SIGNER -S -g -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile >signer.out.$zone 2>&1

  # Copy key to ns2, the other primary.
  echo "${CSK}" >"../ns2/${zn}.keyname"
  cp "${CSK}.key" ../ns2/
  cp "${CSK}.private" ../ns2/
done
