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

# This ensures we're using a suitable hypothesis version. A newer version is
# required for FIPS-enabled platforms.

import hashlib

import pytest

MIN_HYPOTHESIS_VERSION = None

if "md5" not in hashlib.algorithms_available:
    # FIPS mode is enabled, use hypothesis 4.41.2 which doesn't use md5
    MIN_HYPOTHESIS_VERSION = "4.41.2"

pytest.importorskip("hypothesis", minversion=MIN_HYPOTHESIS_VERSION)

from . import settings
from . import strategies
