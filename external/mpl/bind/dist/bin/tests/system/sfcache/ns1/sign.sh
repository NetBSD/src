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

set -e

zone=.
infile=root.db.in
zonefile=root.db

(cd ../ns2 && $SHELL sign.sh )

cp "../ns2/dsset-example$TP" .

keyname=$($KEYGEN -q -a "${DEFAULT_ALGORITHM}" -b "${DEFAULT_BITS}" -n zone $zone)

cat "$infile" "$keyname.key" > "$zonefile"

$SIGNER -P -g -o $zone $zonefile > /dev/null

# Configure the resolving server with a trusted key.
keyfile_to_trusted_keys "$keyname" > trusted.conf
cp trusted.conf ../ns2/trusted.conf

# ...or with a managed key.
keyfile_to_managed_keys "$keyname" > managed.conf
