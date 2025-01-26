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
        "*.created",
        "dig.out.*",
        "rndc.reload.*",
        "rndc.signing.*",
        "update.out.*",
        "verify.out.*",
        "ns*/dsset-**",
        "ns*/K*",
        "ns*/settime.out.*",
        "ns*/*.db",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/*.signed",
        "ns*/keygen.out.*",
        "ns3/named-fips.conf",
    ]
)


def test_nsec3(run_tests_sh):
    run_tests_sh()
