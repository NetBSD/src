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

zone=example.com.
infile=example.com.db.in
zonefile=example.com.db
starttime=20150729220000
endtime=20150819220000

echo_i "ns2/sign.sh"

cp $infile $zonefile

if [ -f ../ed25519-supported.file ]; then

  for i in Xexample.com.+015+03613 Xexample.com.+015+35217; do
    cp "$i.key" "$(echo $i.key | sed s/X/K/)"
    cp "$i.private" "$(echo $i.private | sed s/X/K/)"
    cat "$(echo $i.key | sed s/X/K/)" >>"$zonefile"
  done
fi

$SIGNER -P -z -s "$starttime" -e "$endtime" -o "$zone" "$zonefile" >/dev/null 2>signer.err || cat signer.err
