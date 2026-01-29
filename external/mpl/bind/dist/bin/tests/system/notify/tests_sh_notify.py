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
        "awk.out.*",
        "dig.out.*",
        "ns2/example.db",
        "ns2/named-tls.conf",
        "ns2/x21.db*",
        "ns3/example.bk",
        "ns3/named-tls.conf",
        "ns4/named.port",
        "ns4/x21.bk",
        "ns4/x21.bk.jnl",
        "ns5/x21.bk-b",
        "ns5/x21.bk-b.jnl",
        "ns5/x21.bk-c",
        "ns5/x21.bk-c.jnl",
        "ns5/x21.db.jnl",
    ]
)


def test_notify(run_tests_sh):
    run_tests_sh()
