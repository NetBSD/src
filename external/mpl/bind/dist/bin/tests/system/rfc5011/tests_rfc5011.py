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
from isctest.mark import live_internet_test

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns1/managed-keys.bind.jnl",
    ]
)


@live_internet_test
def test_rfc5011_rootdnskeyrefresh(ns1):
    with ns1.watch_log_from_start() as watcher:
        watcher.wait_for_line(
            "managed-keys-zone: Initializing automatic trust anchor management for zone '.'; DNSKEY ID 20326 is now trusted, waiving the normal 30-day waiting period"
        )

    with ns1.watch_log_from_start() as watcher:
        watcher.wait_for_line(
            "managed-keys-zone: Initializing automatic trust anchor management for zone '.'; DNSKEY ID 38696 is now trusted, waiving the normal 30-day waiting period"
        )
