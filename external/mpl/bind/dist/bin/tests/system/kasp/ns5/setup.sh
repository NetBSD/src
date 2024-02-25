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

echo_i "ns5/setup.sh"

#
# Set up zones that potentially will be initially signed.
#
for zn in inherit.inherit override.inherit none.inherit \
  inherit.override override.override none.override \
  inherit.none override.none none.none; do
  zone="$zn.unsigned"
  echo_i "setting up zone: $zone"
  zonefile="${zone}.db"
  cp template.db.in $zonefile
done
