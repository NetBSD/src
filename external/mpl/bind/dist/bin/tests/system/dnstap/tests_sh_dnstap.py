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

import isctest.mark

pytestmark = [
    isctest.mark.with_dnstap,
    pytest.mark.extra_artifacts(
        [
            "dig.out*",
            "dnstap.hex*",
            "dnstap.out*",
            "fstrm_capture.out.*",
            "nsupdate.out*",
            "ydump.out*",
            "ns*/dnstap.out*",
            "ns2/example.db",
            "ns2/example.db.jnl",
        ]
    ),
]


@pytest.mark.flaky(max_runs=2, rerun_filter=isctest.mark.is_host_freebsd_13)
def test_dnstap(run_tests_sh):
    run_tests_sh()
