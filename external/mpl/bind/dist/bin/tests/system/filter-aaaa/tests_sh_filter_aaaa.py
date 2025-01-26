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
        "ns*/trusted.conf",
        "ns1/K*",
        "ns1/dsset-*",
        "ns1/*.signed",
        "ns1/signer.err",
        "ns4/K*",
        "ns4/dsset-*",
        "ns4/*.signed",
        "ns4/signer.err",
    ]
)


def test_filter_aaaa(run_tests_sh):
    run_tests_sh()
