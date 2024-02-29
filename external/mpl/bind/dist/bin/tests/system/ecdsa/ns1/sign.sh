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

set -e

. ../../conf.sh

zone=.
infile=root.db.in
zonefile=root.db

echo_i "ns1/sign.sh"

cp $infile $zonefile

if [ -f ../ecdsa256-supported.file ]; then
  zsk256=$($KEYGEN -q -a ECDSA256 -n zone "$zone")
  ksk256=$($KEYGEN -q -a ECDSA256 -n zone -f KSK "$zone")
  cat "$ksk256.key" "$zsk256.key" >>"$zonefile"
  $DSFROMKEY -a sha-256 "$ksk256.key" >>dsset-256
fi

if [ -f ../ecdsa384-supported.file ]; then
  zsk384=$($KEYGEN -q -a ECDSA384 -n zone "$zone")
  ksk384=$($KEYGEN -q -a ECDSA384 -n zone -f KSK "$zone")
  cat "$ksk384.key" "$zsk384.key" >>"$zonefile"
  $DSFROMKEY -a sha-256 "$ksk384.key" >>dsset-256
fi

# Configure the resolving server with a static key.
if [ -f ../ecdsa256-supported.file ]; then
  keyfile_to_static_ds $ksk256 >trusted.conf
  cp trusted.conf ../ns2/trusted.conf
else
  keyfile_to_static_ds $ksk384 >trusted.conf
  cp trusted.conf ../ns2/trusted.conf
fi

if [ -f ../ecdsa384-supported.file ]; then
  keyfile_to_static_ds $ksk384 >trusted.conf
  cp trusted.conf ../ns3/trusted.conf
else
  keyfile_to_static_ds $ksk256 >trusted.conf
  cp trusted.conf ../ns3/trusted.conf
fi

$SIGNER -P -g -o "$zone" "$zonefile" >/dev/null 2>signer.err || cat signer.err
