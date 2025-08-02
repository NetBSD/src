#!/bin/sh

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

. ../conf.sh

for d in ns1 ns2 ns3; do
  conf=named.conf
  copy_setports "${d}/${conf}.in" "${d}/${conf}"
  conf=statistics-channels.conf
  if $FEATURETEST --have-libxml2 || $FEATURETEST --have-json-c; then
    copy_setports "${d}/${conf}.in" "${d}/${conf}"
  else
    echo "" >"${d}/${conf}"
  fi
done
