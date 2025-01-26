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
        "dig.out*",
        "stats.*",
        "ans*/ans.run",
        "ns*/*.jnl",
        "ns1/named.run.prev*",
        "ns1/myftp.db",
        "ns3/large.db",
        "ns3/mytest*.db",
        "ns3/subtest*.db",
        "ns4/mytest.db",
        "ns4/subtest.db",
        "ns5/mytest.db",
        "ns5/subtest.db",
    ]
)


def test_ixfr(run_tests_sh):
    run_tests_sh()
