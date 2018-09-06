#!/bin/sh -e
#
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

zone=.
rm -f K.+*+*.key
rm -f K.+*+*.private
keyname=`$KEYGEN -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone $zone`
keyname=`$KEYGEN -q -r $RANDFILE -a RSASHA1 -b 1024 -n zone -f KSK $zone`
$SIGNER -S -x -T 1200 -o ${zone} root.db > signer.out 2>&1
[ $? = 0 ] || cat signer.out

keyfile_to_trusted_keys $keyname > trusted.conf
cp trusted.conf ../ns6/trusted.conf
