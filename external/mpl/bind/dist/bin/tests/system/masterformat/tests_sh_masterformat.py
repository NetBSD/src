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
        "baseline.txt",
        "dig.out.*",
        "raw.*",
        "rndc.out*",
        "text.*",
        "ns*/*.raw*",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/*.signed",
        "ns*/dsset-*",
        "ns*/K*",
        "ns1/255types.db",
        "ns1/example.db.compat",
        "ns1/below-limit-kasp.db",
        "ns1/below-limit.db",
        "ns1/above-limit.db",
        "ns1/under-limit-kasp.db",
        "ns1/under-limit.db",
        "ns2/db-*",
        "ns2/example.db",
        "ns2/formerly-text.db",
        "ns2/transfer.db.full",
        "ns2/transfer.db.txt",
        "ns2/under-limit-kasp.bk",
        "ns2/under-limit.bk",
    ]
)


def test_masterformat(run_tests_sh):
    run_tests_sh()
