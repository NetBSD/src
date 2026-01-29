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
        "dig.out.*",
        "*/ans.run",
        "ns2/K*",
        "ns2/dsset-*",
        "ns2/*.db.signed",
    ]
)


def test_chain(run_tests_sh):
    run_tests_sh()
