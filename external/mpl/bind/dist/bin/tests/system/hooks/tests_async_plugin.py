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

pytest.importorskip("dns")
import dns.message
import dns.query
import dns.rcode


def test_async_hook(named_port):
    msg = dns.message.make_query(
        "example.com.",
        "A",
    )
    ans = dns.query.udp(msg, "10.53.0.1", timeout=10, port=named_port)
    # the test-async plugin changes the status of any positive answer to NOTIMP
    assert ans.rcode() == dns.rcode.NOTIMP
