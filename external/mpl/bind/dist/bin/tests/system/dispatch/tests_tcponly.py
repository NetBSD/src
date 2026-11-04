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

import dns.message
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
    ]
)


def test_tcponly_not_resolved():
    """
    An authoritative server that only answers over TCP is unreachable
    when its zone is queried over UDP: the resolver does not transparently
    fall back to TCP after UDP timeouts. (This confirms the expected behavior
    for this commit; TCP fallback will be restored in the next.)
    """
    msg = dns.message.make_query("foo.tcp-only.", "A")
    res = isctest.query.udp(msg, "10.53.0.2", timeout=15)
    isctest.check.servfail(res)
