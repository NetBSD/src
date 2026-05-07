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

$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 >ns2/nil.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 >ns2/other.db
$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 >ns2/static.db

$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 >ns4/example.db

$SHELL ${TOP_SRCDIR}/bin/tests/system/genzone.sh 2 >ns6/huge.zone.db

cp ns7/test.db.in ns7/test.db
cp ns7/include.db.in ns7/include.db

# we make the huge zone less huge if we're running under
# TSAN, to give the test a fighting chance not to time out.
size=1000000
if $FEATURETEST --tsan; then
  size=250000
fi
awk 'END { for (i = 1; i <= '${size}'; i++)
     printf "host%d IN A 10.53.0.6\n", i; }' </dev/null >>ns6/huge.zone.db

keyset=
make_key() {
  $RNDCCONFGEN -k key$1 -A $3 -s 10.53.0.4 -p $2 \
    >ns4/key${1}.conf 2>/dev/null
  grep -E -v '(^# Start|^# End|^# Use|^[^#])' ns4/key$1.conf | cut -c3- \
    | sed 's/allow { 10.53.0.4/allow { any/' >>ns4/named.conf
  key='"'key$1'";'
  keyset="$keyset $key"
}

$FEATURETEST --md5 && make_key 1 ${EXTRAPORT1} hmac-md5
make_key 2 ${EXTRAPORT2} hmac-sha1
make_key 3 ${EXTRAPORT3} hmac-sha224
make_key 4 ${EXTRAPORT4} hmac-sha256
make_key 5 ${EXTRAPORT5} hmac-sha384
make_key 6 ${EXTRAPORT6} hmac-sha512

cat >>ns4/named.conf <<-EOF

controls {
	inet 10.53.0.4 port ${EXTRAPORT7}
		allow { any; } keys { $keyset };
};
EOF
