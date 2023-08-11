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
	  nsec3-inline-to-dynamic nsec3-dynamic-update-inline
do
	setup "${zn}.kasp"
done

cp nsec3-fails-to-load.kasp.db.in nsec3-fails-to-load.kasp.db
