#!/bin/sh
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

#
# Run a single system test using the pytest runner. This is a simple wrapper
# around pytest for convenience.
#

if [ -z "$1" ] || [ ! -d "$1" ]; then
  echo "Usage: $0 system_test_dir [pytest_args]"
  exit 2
fi

system_test_dir="$1"
shift

(
  cd "$system_test_dir" || exit 2
  /usr/bin/env python3 -m pytest "$@"
)
