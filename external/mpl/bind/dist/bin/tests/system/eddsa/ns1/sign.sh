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

if [ -f ../ed25519-supported.file ]; then
  zsk25519=$($KEYGEN -q -a ED25519 -n zone "$zone")
  ksk25519=$($KEYGEN -q -a ED25519 -n zone -f KSK "$zone")
  cat "$ksk25519.key" "$zsk25519.key" >>"$zonefile"
  $DSFROMKEY -a sha-256 "$ksk25519.key" >>dsset-256
fi

if [ -f ../ed448-supported.file ]; then
  zsk448=$($KEYGEN -q -a ED448 -n zone "$zone")
  ksk448=$($KEYGEN -q -a ED448 -n zone -f KSK "$zone")
  cat "$ksk448.key" "$zsk448.key" >>"$zonefile"
  $DSFROMKEY -a sha-256 "$ksk448.key" >>dsset-256
fi

# Configure the resolving server with a static key.
if [ -f ../ed25519-supported.file ]; then
  keyfile_to_static_ds $ksk25519 >trusted.conf
  cp trusted.conf ../ns2/trusted.conf
else
  keyfile_to_static_ds $ksk448 >trusted.conf
  cp trusted.conf ../ns2/trusted.conf
fi

if [ -f ../ed448-supported.file ]; then
  keyfile_to_static_ds $ksk448 >trusted.conf
  cp trusted.conf ../ns3/trusted.conf
else
  keyfile_to_static_ds $ksk25519 >trusted.conf
  cp trusted.conf ../ns3/trusted.conf
fi

$SIGNER -P -g -o "$zone" "$zonefile" >/dev/null 2>signer.err || cat signer.err
