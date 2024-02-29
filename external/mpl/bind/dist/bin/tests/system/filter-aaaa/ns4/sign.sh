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

. ../../conf.sh

SYSTESTDIR=filter-aaaa

zone=signed.
infile=signed.db.in
zonefile=signed.db.signed
outfile=signed.db.signed

$KEYGEN -a $DEFAULT_ALGORITHM $zone 2>&1 >/dev/null | cat_i
$KEYGEN -f KSK -a $DEFAULT_ALGORITHM $zone 2>&1 >/dev/null | cat_i

$SIGNER -S -o $zone -f $outfile $infile >/dev/null 2>signer.err || cat signer.err
echo_i "signed zone '$zone'"
