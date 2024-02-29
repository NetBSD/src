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

set -e

zone=.
infile=root.db.in
zonefile=root.db

(cd ../ns2 && $SHELL sign.sh)

cp "../ns2/dsset-example." .

keyname=$($KEYGEN -q -a "${DEFAULT_ALGORITHM}" -b "${DEFAULT_BITS}" -n zone $zone)

cat "$infile" "$keyname.key" >"$zonefile"

$SIGNER -P -g -o $zone $zonefile >/dev/null

# Configure the resolving server with a static key.
keyfile_to_static_ds "$keyname" >trusted.conf
cp trusted.conf ../ns2/trusted.conf

# ...or with an initializing key.
keyfile_to_initial_ds "$keyname" >managed.conf
