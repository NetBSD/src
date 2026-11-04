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
Tests for the look-up tools other than dig, delv and mdig: nslookup,
host and nsupdate.
"""

from textwrap import dedent

import pytest

from digdelv.common import ARTIFACTS

import isctest

pytestmark = pytest.mark.extra_artifacts(ARTIFACTS)


def test_nslookup_update_response(named_port, ans6):
    """Check that nslookup rejects a response with the UPDATE opcode."""
    nslookup = isctest.run.EnvCmd("NSLOOKUP")
    result = nslookup(
        f"-port={named_port} -q=CNAME -timeout=1 foo.bar {ans6.ip}",
        raise_on_exception=False,
    )
    assert result.rc != 0
    assert "Opcode mismatch" in result.out


def test_host_update_response(named_port, ans6):
    """Check that host rejects a response with the UPDATE opcode."""
    host = isctest.run.EnvCmd("HOST")
    result = host(
        f"-p {named_port} -W 1 -t CNAME foo.bar {ans6.ip}", raise_on_exception=False
    )
    assert result.rc != 0
    assert "Opcode mismatch" in result.out


def test_nsupdate_update_response(named_port, ans6):
    """Check that nsupdate rejects an UPDATE response to its SOA query."""
    nsupdate = isctest.run.EnvCmd("NSUPDATE")
    commands = dedent(f"""\
        server {ans6.ip} {named_port}
        add x.example.com 300 in a 1.2.3.4
        send
    """)
    result = nsupdate("", input_text=commands.encode(), raise_on_exception=False)
    assert result.rc == 1
    assert "invalid OPCODE in response to SOA query" in result.err
