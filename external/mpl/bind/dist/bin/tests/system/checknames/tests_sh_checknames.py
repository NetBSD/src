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
        "dig.out.ns*.test*",
        "nsupdate.out.*",
        "ns1/*.example.db",
        "ns1/*.update.db",
        "ns1/*.update.db.jnl",
        "ns4/*.update.db",
        "ns4/*.update.db.jnl",
        "ns5/*.update.db",
        "ns5/*.update.db.jnl",
    ]
)


def test_checknames(run_tests_sh):
    run_tests_sh()
