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
. "$SYSTEMTESTTOP/conf.sh"

zone=example
infile=example.db.in
zonefile=example.db

keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
cat "$infile" "$keyname.key" > "$zonefile"

$SIGNER -P -o $zone $zonefile > /dev/null

zone=dnamed
infile=dnamed.db.in
zonefile=dnamed.db

keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -n zone $zone)
cat "$infile" "$keyname.key" > "$zonefile"

$SIGNER -P -o $zone $zonefile > /dev/null

zone=.
infile=root.db.in
zonefile=root.db

keyname=$($KEYGEN -q -a ${DEFAULT_ALGORITHM} -b ${DEFAULT_BITS} -n zone $zone)
cat "$infile" "$keyname.key" > "$zonefile"

$SIGNER -P -g -o $zone $zonefile > /dev/null

# Configure the resolving server with a static key.
keyfile_to_static_ds "$keyname" > trusted.conf
