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

set -e

. ../conf.sh

echo_i "tools must not crash if stdio is closed"
"$NSUPDATE" 0>&-
"$NSUPDATE" 0>&- 1>&-
"$NSUPDATE" 0>&- 1>&- 2>&-
"$NSUPDATE" 0>&- 2>&-
"$NSUPDATE" 1>&-
"$NSUPDATE" 1>&- 2>&-
"$NSUPDATE" 2>&-
