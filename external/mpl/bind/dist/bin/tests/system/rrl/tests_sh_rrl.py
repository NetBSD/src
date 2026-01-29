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
        "*mdig.out*",
        "dig.out.*",
        "ns*/log-*",
        "ns2/named.stats",
    ]
)


# The rrl is known to be quite unstable. GL #172
@pytest.mark.flaky(max_runs=2)
def test_rrl(run_tests_sh):
    run_tests_sh()
