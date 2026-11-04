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

import os

import dns.flags
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns*/trusted.conf",
        "ns1/K*",
        "ns1/dsset-*",
        "ns1/root.db",
        "ns1/root.db.signed",
        "ns1/signer.err",
    ]
)


def check_server_soa(resolver):
    msg = isctest.query.create(".", "SOA")
    res1 = isctest.query.tcp(msg, "10.53.0.1")
    res2 = isctest.query.tcp(msg, resolver)
    isctest.check.rrsets_equal(res1.answer, res2.answer)
    assert res2.flags & dns.flags.AD


@pytest.mark.skipif(
    not os.getenv("ECDSAP256SHA256_SUPPORTED"),
    reason="algorithm ECDSA256 not supported",
)
def test_ecdsa256():
    check_server_soa("10.53.0.2")


@pytest.mark.skipif(
    not os.getenv("ECDSAP384SHA384_SUPPORTED"),
    reason="algorithm ECDSA384 not supported",
)
def test_ecdsa384():
    check_server_soa("10.53.0.3")
