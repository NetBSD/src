#!/bin/sh -e
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

. ../conf.sh

if [ -n "${SOFTHSM2_MODULE}" ] && command -v pkcs11-tool >/dev/null && command -v softhsm2-util >/dev/null; then
  exit 0
fi

echo_i "skip: softhsm2-util or pkcs11-tool not available"
exit 255
