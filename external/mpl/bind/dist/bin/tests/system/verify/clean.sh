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

rm -f ns*/named.lock
rm -f verify.out*
rm -f zones/*.bad
rm -f zones/*.good
rm -f zones/*.out*
rm -f zones/*.tmp
rm -f zones/K*
rm -f zones/dsset-*
