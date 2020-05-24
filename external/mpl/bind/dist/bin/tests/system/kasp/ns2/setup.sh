#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. "$SYSTEMTESTTOP/conf.sh"

echo_i "ns2/setup.sh"

zone="secondary.kasp"
echo_i "setting up zone: $zone"
zonefile="${zone}.db"
infile="${zonefile}.in"
cp $infile $zonefile

zone="signed.tld"
echo_i "setting up zone: $zone"
zonefile="${zone}.db"
infile="template.tld.db.in"
cp $infile $zonefile

zone="unsigned.tld"
echo_i "setting up zone: $zone"
zonefile="${zone}.db"
infile="template.tld.db.in"
cp $infile $zonefile
