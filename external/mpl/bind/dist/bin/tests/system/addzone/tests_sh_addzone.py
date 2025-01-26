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
        "rndc.out*",
        "showzone.out.*",
        "zonestatus.out.*",
        "ns*/*.nzd*",
        "ns*/*.nzf*",
        "ns1/redirect.db",
        "nzd2nzf.out.*",
        "ns2/*.nzf~",
        "ns2/K*.key",
        "ns2/K*.private",
        "ns2/K*.state",
        "ns2/external.nzd",
        "ns2/extra.nzd",
        "ns2/inline.db.jbk",
        "ns2/inline.db.signed",
        "ns2/inline.db.signed.jnl",
        "ns2/inlinesec.bk.jbk",
        "ns2/new-zones",
        "ns2/redirect.bk",
        "ns2/redirect.db",
        "ns3/redirect.db",
    ]
)


def test_addzone(run_tests_sh):
    run_tests_sh()
