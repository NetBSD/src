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

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

zone=.
zonefile=root.db

keyname=$($KEYGEN -a ${DEFAULT_ALGORITHM} -qfk $zone)
zskkeyname=$($KEYGEN -a ${DEFAULT_ALGORITHM} -q $zone)

$SIGNER -Sg -o $zone $zonefile > /dev/null 2>/dev/null

# Configure the resolving server with an initializing key.
keyfile_to_initial_ds $keyname > managed.conf
cp managed.conf ../ns2/managed.conf
cp managed.conf ../ns4/managed.conf
cp managed.conf ../ns5/managed.conf

# Configure broken trust anchor for ns3
# Rotate each nibble in the digest by -1
$DSFROMKEY $keyname.key |
awk '!/^; /{
            printf "trust-anchors {\n"
            printf "\t\""$1"\" initial-ds "
            printf $4 " " $5 " " $6 " \""
            for (i=7; i<=NF; i++) {
		# rotate digest
		digest=$i
		gsub("0", ":", digest)
		gsub("1", "0", digest)
		gsub("2", "1", digest)
		gsub("3", "2", digest)
		gsub("4", "3", digest)
		gsub("5", "4", digest)
		gsub("6", "5", digest)
		gsub("7", "6", digest)
		gsub("8", "7", digest)
		gsub("9", "8", digest)
		gsub("A", "9", digest)
		gsub("B", "A", digest)
		gsub("C", "B", digest)
		gsub("D", "C", digest)
		gsub("E", "D", digest)
		gsub("F", "E", digest)
		gsub(":", "F", digest)
		printf digest
	    }
	    printf "\";\n"
	    printf "};\n"
	}' > ../ns3/broken.conf

# Configure a static key to be used by delv.
keyfile_to_static_ds $keyname > trusted.conf

# Prepare an unsupported algorithm key.
unsupportedkey=Kunknown.+255+00000
cp unsupported.key "${unsupportedkey}.key"

#
#  Save keyname and keyid for managed key id test.
#
echo "$keyname" > managed.key
echo "$zskkeyname" > zone.key
keyfile_to_key_id $keyname > managed.key.id
