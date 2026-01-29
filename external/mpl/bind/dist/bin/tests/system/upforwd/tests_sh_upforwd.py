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

# isctest.asyncserver requires dnspython >= 2.0.0
pytest.importorskip("dns", minversion="2.0.0")

pytestmark = pytest.mark.extra_artifacts(
    [
        "Ksig0.example2*",
        "dig.out.*",
        "dnstap.out.*",
        "dnstapread.out*",
        "keyname*",
        "nsupdate.out.*",
        "ans*/ans.run",
        "ns*/*.bk",
        "ns*/*.jnl",
        "ns1/example.db",
        "ns1/example2-toomanykeys.db",
        "ns1/example2.db",
        "ns1/example3.db",
        "ns3/dnstap.conf",
        "ns3/dnstap.out",
        "ns3/noprimary1.db",
    ]
)


def test_upforwd(run_tests_sh):
    run_tests_sh()
