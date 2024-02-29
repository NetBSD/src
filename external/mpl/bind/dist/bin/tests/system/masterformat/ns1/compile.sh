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
$CHECKZONE -D -F raw -o large.db.raw large large.db >/dev/null 2>&1

$KEYGEN -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" -f KSK signed >/dev/null 2>&1
$KEYGEN -q -a "$DEFAULT_ALGORITHM" -b "$DEFAULT_BITS" signed >/dev/null 2>&1
$SIGNER -S -f signed.db.signed -o signed signed.db >/dev/null
$CHECKZONE -D -F raw -o signed.db.raw signed signed.db.signed >/dev/null 2>&1
