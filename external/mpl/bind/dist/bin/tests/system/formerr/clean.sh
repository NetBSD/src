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

rm -f badnsec3owner.out
rm -f badrecordname.out
rm -f dupans.out
rm -f dupquestion.out
rm -f keyclass.out
rm -f malformeddeltype.out
rm -f malformedrrsig.out
rm -f nametoolong.out
rm -f noquestions.out
rm -f ns*/managed-keys.bind*
rm -f ns*/named.conf
rm -f ns*/named.lock
rm -f ns*/named.memstats
rm -f ns*/named.run
rm -f optwrongname.out
rm -f qtypeasanswer.out
rm -f questionclass.out
rm -f shortquestion.out
rm -f shortrecord.out
rm -f tsignotlast.out
rm -f tsigwrongclass.out
rm -f twoquestionnames.out
rm -f twoquestiontypes.out
rm -f wrongclass.out
