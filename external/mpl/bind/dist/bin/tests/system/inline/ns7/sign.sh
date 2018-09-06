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

# NOTE: The number of signing keys generated below is not coincidental.  More
# details can be found in the comment inside ns7/named.conf.

zone=nsec3-loop
rm -f K${zone}.+*+*.key
rm -f K${zone}.+*+*.private
keyname=`$KEYGEN -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone`
keyname=`$KEYGEN -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone $zone`
keyname=`$KEYGEN -q -r $RANDFILE -a NSEC3RSASHA1 -b 1024 -n zone -f KSK $zone`
