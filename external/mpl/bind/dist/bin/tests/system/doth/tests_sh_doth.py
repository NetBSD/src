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

import pytest

import isctest.mark

EXTRA_ARTIFACTS = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "headers.*",
        "ns*/example*.db",
    ]
)

pytestmark = [
    isctest.mark.with_libnghttp2,
    isctest.mark.with_fips_dh,
    EXTRA_ARTIFACTS,
]


def test_doth(run_tests_sh):
    run_tests_sh()
