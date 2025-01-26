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
        "rndc.out.*",
        "ns1/named.args",
        "ns1/named_deflog",
        "ns1/named_inc*",
        "ns1/named_iso8601",
        "ns1/named_iso8601_utc",
        "ns1/named_log",
        "ns1/named_pipe",
        "ns1/named_sym",
        "ns1/named_ts*",
        "ns1/named_unlimited*",
        "ns1/named_vers*",
        "ns1/query_log*",
    ]
)


def test_logfileconfig(run_tests_sh):
    run_tests_sh()
