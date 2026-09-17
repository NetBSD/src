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

"""
Tests for the mdig tool.
"""

import os

import pytest

from digdelv.common import ARTIFACTS, parse_yaml

import isctest

pytestmark = [
    pytest.mark.extra_artifacts(ARTIFACTS),
]


@pytest.fixture(name="mdig")
def mdig_fixture(named_port):
    return isctest.run.EnvCmd("MDIG", f"-p {named_port}")


def test_tcp_with_source_address_and_port(mdig, ns3):
    """Check that mdig +tcp works with a source address and port.  When
    running more than once in quick succession with a source
    address#port, the query can fail with "response failed with address
    not available" because the address#port is still busy; that error is
    not interesting, only the unexpected error case is.  See GL #4969
    for more information."""
    extraport8 = os.environ["EXTRAPORT8"]
    result = mdig(
        f"-b {ns3.ip}#{extraport8} +tcp @{ns3.ip} example", raise_on_exception=False
    )
    assert "unexpected error" not in result.out
    assert "unexpected error" not in result.err


def test_ednsopt_malformed(mdig, ns3):
    """Check that mdig handles the malformed option '+ednsopt=:'
    gracefully."""
    result = mdig(f"@{ns3.ip} +ednsopt=: a.example", raise_on_exception=False)
    assert result.rc != 0
    assert "ednsopt no code point specified" in result.err


def test_ednsopt_ednsopts(mdig, ns3):
    """Check that mdig handles multiple queries with ednsopts."""
    result = mdig(f"@{ns3.ip} +ednsopt=100:00 a.example b.example")
    assert result.rc == 0


def test_dnskey_norrcomments(mdig, ns3, zsk):
    """Check that +multi +norrcomments suppresses the DNSKEY comment
    (the default is rrcomments)."""
    result = mdig(f"+tcp @{ns3.ip} +multi +norrcomments -t DNSKEY example")
    assert zsk.rrcomment not in result.out


def test_soa_norrcomments(mdig, ns3):
    """Check that +multi +norrcomments suppresses the SOA field
    comments."""
    result = mdig(f"+tcp @{ns3.ip} +multi +norrcomments -t SOA example")
    assert "; serial" not in result.out


def test_yaml_output(mdig, ns3):
    """Check the structure of mdig +yaml output."""
    result = mdig(f"+yaml @{ns3.ip} -t any ns2.example")
    response = parse_yaml(result.out)[0]["message"]["response_message_data"]
    assert response["status"] == "NOERROR"
    assert response["QUESTION_SECTION"][0] == "ns2.example. IN ANY"
