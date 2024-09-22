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

# shellcheck source=conf.sh
. ../../conf.sh

for zone in kasp-max-records-per-type \
  kasp-max-records-per-type-dnskey \
  kasp-max-types-per-name; do
  $CHECKZONE -D -F raw -o $zone.db.raw $zone template.db >/dev/null 2>&1
done
