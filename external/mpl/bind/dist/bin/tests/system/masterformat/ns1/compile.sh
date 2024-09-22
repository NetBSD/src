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

$CHECKZONE -D -F raw -o example.db.raw example \
  example.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o ../ns3/example.db.raw example \
  example.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o ../ns3/dynamic.db.raw dynamic \
  example.db >/dev/null 2>&1
$CHECKZONE -D -F raw=1 -o example.db.raw1 example-explicit \
  example.db >/dev/null 2>&1
$CHECKZONE -D -F raw=0 -o example.db.compat example-compat \
  example.db >/dev/null 2>&1
$CHECKZONE -D -F raw -L 3333 -o example.db.serial.raw example \
  example.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o under-limit.db.raw under-limit under-limit.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o under-limit-kasp.db.raw under-limit-kasp under-limit-kasp.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o on-limit.db.raw on-limit on-limit.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o on-limit-kasp.db.raw on-limit-kasp on-limit-kasp.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o over-limit.db.raw over-limit over-limit.db >/dev/null 2>&1
$CHECKZONE -D -F raw -o 255types.db.raw 255types 255types.db >/dev/null 2>&1

$KEYGEN -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK signed >/dev/null 2>&1
$KEYGEN -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" signed >/dev/null 2>&1
$SIGNER -S -f signed.db.signed -o signed signed.db >/dev/null
$CHECKZONE -D -F raw -o signed.db.raw signed signed.db.signed >/dev/null 2>&1
