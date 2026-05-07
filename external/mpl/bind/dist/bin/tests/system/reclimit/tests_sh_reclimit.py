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
        "dsset-signed.",
        "ans*/ans.limit",
        "ans*/ans.run",
        "ns1/K*",
        "ns1/signed.db",
        "ns1/signed.db.signed",
    ]
)


# The reclimit is known to be quite unstable. GL #1587
@pytest.mark.flaky(max_runs=2)
def test_reclimit(run_tests_sh):
    run_tests_sh()
