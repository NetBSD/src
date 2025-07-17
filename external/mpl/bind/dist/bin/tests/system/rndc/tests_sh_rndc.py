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
        "nsupdate.out.*",
        "rndc.out.*",
        "ns2/*.jnl",
        "ns2/named.stats",
        "ns2/named_dump.db*",
        "ns2/nil.db",
        "ns2/other.db",
        "ns2/secondkey.conf",
        "ns2/static.db",
        "ns3/all.nta",
        "ns3/named_dump.db.*",
        "ns3/none.nta",
        "ns4/example.db",
        "ns4/example.db.jnl",
        "ns4/key*.conf",
        "ns4/normal.nta",
        "ns4/view with a space.nta",
        "ns6/huge.zone.db",
        "ns7/include.db",
        "ns7/test.db",
        "ns7/test.db.jnl",
    ]
)


def test_rndc(run_tests_sh):
    run_tests_sh()
