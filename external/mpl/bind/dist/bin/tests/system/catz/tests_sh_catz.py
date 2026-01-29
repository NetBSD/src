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
        "wait_for_message.*",
        "ns*/*.jnl",
        "ns*/*.nzf*",
        "ns*/*.nzd*",
        "ns*/catalog*.example.db",
        "ns*/*dom*.example.db",
        "ns1/longlong.longlong.long.long.name.example.db",
        "ns1/tls1.example.db",
        "ns2/__catz__*.db",
        "ns2/named.conf.tmp",
        "ns2/zonedir",
        "ns4/__catz__*.db",
    ]
)


def test_catz(run_tests_sh):
    run_tests_sh()
