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
(cd ../ns6 && $SHELL sign.sh )
(cd ../ns7 && $SHELL sign.sh )

echo_i "ns1/sign.sh"

cp "../ns2/dsset-example$TP" .
cp "../ns2/dsset-in-addr.arpa$TP" .

grep "$DEFAULT_ALGORITHM_NUMBER [12] " "../ns2/dsset-algroll$TP" > "dsset-algroll$TP"
cp "../ns6/dsset-optout-tld$TP" .

ksk=$("$KEYGEN" -q -fk -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")
zsk=$("$KEYGEN" -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -n zone "$zone")

cat "$infile" "$ksk.key" "$zsk.key" > "$zonefile"

"$SIGNER" -P -g -o "$zone" "$zonefile" > /dev/null 2>&1

# Configure the resolving server with a staitc key.
keyfile_to_static_ds "$ksk" > trusted.conf
cp trusted.conf ../ns2/trusted.conf
cp trusted.conf ../ns3/trusted.conf
cp trusted.conf ../ns4/trusted.conf
cp trusted.conf ../ns6/trusted.conf
cp trusted.conf ../ns7/trusted.conf
cp trusted.conf ../ns9/trusted.conf

keyfile_to_trusted_keys "$ksk" > trusted.keys

# ...or with an initializing key.
keyfile_to_initial_ds "$ksk" > managed.conf
cp managed.conf ../ns4/managed.conf

#
#  Save keyid for managed key id test.
#

keyfile_to_key_id "$ksk" > managed.key.id
