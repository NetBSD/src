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

zone="checkds"
infile="checkds.db.infile"
zonefile="checkds.db"

CSK=$($KEYGEN -k default $zone 2>keygen.out.$zone)
cat template.db.in "${CSK}.key" >"$infile"
private_type_record $zone $DEFAULT_ALGORITHM_NUMBER "$CSK" >>"$infile"
$SIGNER -S -g -z -x -s now-1h -e now+30d -o $zone -O full -f $zonefile $infile >signer.out.$zone 2>&1
