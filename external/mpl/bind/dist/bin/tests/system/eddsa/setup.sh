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

. ../conf.sh

if $SHELL ../testcrypto.sh ed25519; then
  echo "yes" >ed25519-supported.file
fi

if $SHELL ../testcrypto.sh ed448; then
  echo "yes" >ed448-supported.file
fi

copy_setports ns1/named.conf.in ns1/named.conf
copy_setports ns2/named.conf.in ns2/named.conf
copy_setports ns3/named.conf.in ns3/named.conf

(
  cd ns1
  $SHELL sign.sh
)
(
  cd ns2
  $SHELL sign.sh
)
(
  cd ns3
  $SHELL sign.sh
)
