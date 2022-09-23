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

#
# Stop all hanging processes from any system tests.
#

SYSTEMTESTTOP=.
. $SYSTEMTESTTOP/conf.sh

for d in $SUBDIRS
do
   $SHELL stop.sh $d
done
