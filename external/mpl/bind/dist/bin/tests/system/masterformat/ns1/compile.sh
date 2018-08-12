# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=../..
. $SYSTEMTESTTOP/conf.sh

../named-compilezone -D -F raw -o example.db.raw example \
        example.db > /dev/null 2>&1
../named-compilezone -D -F map -o ../ns3/example.db.map example \
        example.db > /dev/null 2>&1
../named-compilezone -D -F map -o ../ns3/dynamic.db.map dynamic \
        example.db > /dev/null 2>&1
../named-compilezone -D -F raw=1 -o example.db.raw1 example-explicit \
        example.db > /dev/null 2>&1
../named-compilezone -D -F raw=0 -o example.db.compat example-compat \
        example.db > /dev/null 2>&1
../named-compilezone -D -F raw -L 3333 -o example.db.serial.raw example \
        example.db > /dev/null 2>&1
../named-compilezone -D -F raw -o large.db.raw large large.db > /dev/null 2>&1
../named-compilezone -D -F map -o example.db.map example-map \
        example.db > /dev/null 2>&1

$KEYGEN -q -a rsasha256 -r $RANDFILE signed > /dev/null 2>&1
$KEYGEN -q -a rsasha256 -r $RANDFILE -fk signed > /dev/null 2>&1
$SIGNER -S -f signed.db.signed -o signed signed.db > /dev/null 2>&1
../named-compilezone -D -F map -o signed.db.map signed signed.db.signed > /dev/null 2>&1
