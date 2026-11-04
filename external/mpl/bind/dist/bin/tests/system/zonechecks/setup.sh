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

$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 1 >ns1/primary.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 1 >ns1/duplicate.db
cp bigserial.db ns1/
cd ns1
touch primary.db.signed
echo '$INCLUDE "primary.db.signed"' >>primary.db
$KEYGEN -a ${DEFAULT_ALGORITHM} -q primary.example >/dev/null 2>&1
$KEYGEN -a ${DEFAULT_ALGORITHM} -qfk primary.example >/dev/null 2>&1
$SIGNER -SD -o primary.example primary.db >/dev/null \
  2>signer.err || cat signer.err
echo '$INCLUDE "soa.db"' >reload.db
echo '@ 0 NS .' >>reload.db
echo '@ 0 SOA . . 1 0 0 0 0' >soa.db
