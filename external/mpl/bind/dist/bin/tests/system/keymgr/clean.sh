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

rm -f 18-nonstd-prepub/policy.conf
rm -f 19-old-keys/policy.conf
rm -f K*.key */K*.key
rm -f K*.private */K*.private
rm -f coverage.* keymgr.* settime.*
rm -f ns*/managed-keys.bind*
rm -f policy.conf
rm -f policy.out
