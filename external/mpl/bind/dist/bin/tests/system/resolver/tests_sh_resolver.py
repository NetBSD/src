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
        ".digrc",
        "dig*.out*",
        "dig.*.foo.*",
        "dig.*.bar.*",
        "dig.*.prime.*",
        "nextpart.out.*",
        "ans*/ans.run",
        "ans*/query.log",
        "ns1/named.stats*",
        "ns4/tld.db",
        "ns5/trusted.conf",
        "ns6/K*",
        "ns6/ds.example.net.db",
        "ns6/ds.example.net.db.signed",
        "ns6/dsset-ds.example.net.",
        "ns6/dsset-example.net.",
        "ns6/example.net.db",
        "ns6/example.net.db.signed",
        "ns6/example.net.db.signed.jnl",
        "ns6/to-be-removed.tld.db",
        "ns6/to-be-removed.tld.db.jnl",
        "ns7/server.db",
        "ns7/server.db.jnl",
    ]
)


def test_resolver(run_tests_sh):
    run_tests_sh()
