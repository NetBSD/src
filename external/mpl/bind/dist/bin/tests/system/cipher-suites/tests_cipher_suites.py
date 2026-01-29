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

from re import compile as Re

import pytest

pytest.importorskip("dns", minversion="2.5.0")

import dns.message

import isctest
import isctest.mark


pytestmark = pytest.mark.extra_artifacts(
    [
        "ns*/example*.db",
    ]
)


@pytest.fixture(scope="module")
def transfers_complete(servers):
    for zone in ["example", "example-aes-128", "example-aes-256", "example-chacha-20"]:
        pattern = Re(
            f"transfer of '{zone}/IN' from 10.53.0.1#[0-9]+: Transfer completed"
        )
        for ns in ["ns2", "ns3", "ns4", "ns5"]:
            with servers[ns].watch_log_from_start() as watcher:
                watcher.wait_for_line(pattern)


@pytest.mark.requires_zones_loaded("ns1", "ns2", "ns3", "ns4", "ns5")
@pytest.mark.parametrize(
    "qname,ns,rcode",
    [
        ("example.", 2, dns.rcode.NOERROR),
        ("example.", 3, dns.rcode.NOERROR),
        ("example.", 4, dns.rcode.NOERROR),
        ("example-aes-128.", 2, dns.rcode.NOERROR),
        ("example-aes-256.", 3, dns.rcode.NOERROR),
        pytest.param(
            "example-chacha-20.",
            4,
            dns.rcode.NOERROR,
            marks=isctest.mark.without_fips,
        ),
        ("example-aes-256", 2, dns.rcode.SERVFAIL),
        pytest.param(
            "example-chacha-20",
            2,
            dns.rcode.SERVFAIL,
            marks=isctest.mark.without_fips,
        ),
        ("example-aes-128", 3, dns.rcode.SERVFAIL),
        pytest.param(
            "example-chacha-20",
            3,
            dns.rcode.SERVFAIL,
            marks=isctest.mark.without_fips,
        ),
        ("example-aes-128", 4, dns.rcode.SERVFAIL),
        ("example-aes-256", 4, dns.rcode.SERVFAIL),
        # NS5 tries to download the zone over TLSv1.2
        ("example", 5, dns.rcode.SERVFAIL),
        ("example-aes-128", 5, dns.rcode.SERVFAIL),
        ("example-aes-256", 5, dns.rcode.SERVFAIL),
        pytest.param(
            "example-chacha-20",
            5,
            dns.rcode.SERVFAIL,
            marks=isctest.mark.without_fips,
        ),
    ],
)
# pylint: disable=redefined-outer-name,unused-argument
def test_cipher_suites_tls_xfer(qname, ns, rcode, transfers_complete):
    msg = isctest.query.create(qname, "AXFR")
    ans = isctest.query.tls(msg, f"10.53.0.{ns}")
    assert ans.rcode() == rcode
    if rcode == dns.rcode.NOERROR:
        assert ans.answer != []
    elif rcode == dns.rcode.SERVFAIL:
        assert ans.answer == []
