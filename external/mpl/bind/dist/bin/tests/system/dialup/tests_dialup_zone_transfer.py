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

import isctest

import dns.message


@pytest.mark.parametrize("ns", [2, 3])
def test_dialup_zone_transfer(named_port, servers, ns):
    msg = dns.message.make_query("example.", "SOA")
    # Drop the RD flag from the query
    msg.flags &= ~dns.flags.RD
    ns1response = isctest.query.tcp(msg, "10.53.0.1")
    with servers[f"ns{ns}"].watch_log_from_start() as watcher:
        watcher.wait_for_line(
            f"transfer of 'example/IN' from 10.53.0.{ns-1}#{named_port}: Transfer status: success",
            timeout=90,
        )
    response = isctest.query.tcp(msg, f"10.53.0.{ns}")
    if response.rcode() != dns.rcode.SERVFAIL:
        assert response.answer == ns1response.answer
        assert response.authority == ns1response.authority
