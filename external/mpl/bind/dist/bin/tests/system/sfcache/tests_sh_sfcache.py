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

pytestmark = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "rndc.out.*",
        "sfcache.*",
        "ns*/*.db",
        "ns*/*.signed",
        "ns*/dsset-*",
        "ns*/K*.key",
        "ns*/K*.private",
        "ns*/managed.conf",
        "ns*/trusted.conf",
        "ns5/named.run.*",
        "ns5/named_dump*",
    ]
)


def test_sfcache(run_tests_sh):
    run_tests_sh()
