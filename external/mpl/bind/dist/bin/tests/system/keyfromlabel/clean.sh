#!/bin/sh
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

# shellcheck source=conf.sh
. ../conf.sh

set -e

rm -f K*
rm -f pin
rm -f dsset-*
rm -f *.example.db *.example.db.signed
rm -f keyfromlabel.out.*
rm -f pkcs11-tool.out.*
rm -f signer.out.*

softhsm2-util --delete-token --token "softhsm2-keyfromlabel" >/dev/null 2>&1 || echo_i "softhsm2-keyfromlabel token not found for cleaning"
