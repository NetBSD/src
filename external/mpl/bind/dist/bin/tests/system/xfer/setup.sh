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

$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 1 6 7 >ns1/sec.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 1 6 7 >ns1/edns-expire.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 3 >ns2/example.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 3 >ns2/tsigzone.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 6 3 >ns6/primary.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 7 >ns7/primary2.db

cp -f ns4/root.db.in ns4/root.db
$PERL -e 'for ($i=0;$i<10000;$i++){ printf("x%u 0 in a 10.53.0.1\n", $i);}' >>ns4/root.db

cp ns1/dot-fallback.db.in ns1/dot-fallback.db

cp ns2/sec.db.in ns2/sec.db
touch -t 200101010000 ns2/sec.db

cp ns2/mapped.db.in ns2/mapped.db

$PERL -e 'for ($i=0;$i<4096;$i++){ printf("name%u 259200 A 1.2.3.4\nname%u 259200 TXT \"Hello World %u\"\n", $i, $i, $i);}' >ns8/small.db
$PERL -e 'printf("large IN TYPE45234 \\# 48000 "); for ($i=0;$i<16*3000;$i++) { printf("%02x", $i % 256); } printf("\n");' >ns8/large.db

cp -f ns1/ixfr-too-big.db.in ns1/ixfr-too-big.db
