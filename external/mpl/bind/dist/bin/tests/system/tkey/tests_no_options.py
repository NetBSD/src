#!/usr/bin/python3

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
import subprocess

import dns.exception
import dns.rcode
import dns.rdatatype
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(["nooptions/named.run"])

SERVER_IP = "10.53.0.1"


def terminate_named(named_proc):
    if named_proc.poll() is None:
        named_proc.terminate()
    try:
        named_proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        named_proc.kill()
        named_proc.wait(timeout=10)


def wait_until_ready(named_proc, port):
    # isctest.check.named_alive() cannot be used because it always uses PORT.
    query = isctest.query.create("a.example.nil", "A", dnssec=False, use_edns=False)

    def check_server():
        assert named_proc.poll() is None, "named exited during startup"
        isctest.query.udp(
            query,
            SERVER_IP,
            port=port,
            timeout=1,
            attempts=1,
            expected_rcode=dns.rcode.NOERROR,
        )
        return True

    isctest.run.retry_with_timeout(check_server, timeout=10)


def test_tkey_query_without_options():
    """A TKEY query must not crash named when global options are omitted."""
    port = int(os.environ["EXTRAPORT1"])
    named_cmdline = isctest.run.get_named_cmdline("nooptions")
    named_cmdline.extend(["-p", str(port), "-T", "maxcachesize=2097152"])

    # Do not add an options block: on affected branches, even an empty one
    # creates the TKEY context and masks the bug.  This makes named listen on
    # all local addresses and use its compiled-in runtime-file paths.  The
    # extra port avoids clashes; runtime files may be created in root-run test
    # environments, while failures to create them as an unprivileged user are
    # expected.

    query = isctest.query.create(
        ".",
        dns.rdatatype.TKEY,
        dnssec=False,
        use_edns=False,
        rd=False,
        ad=False,
        message_id=0x1234,
    )
    assert query.to_wire() == bytes.fromhex("1234000000010000000000000000f90001")

    with open("nooptions/named.run", "wb") as named_log:
        named_proc = subprocess.Popen(  # pylint: disable=consider-using-with
            named_cmdline,
            cwd="nooptions",
            stdout=named_log,
            stderr=subprocess.STDOUT,
        )
        try:
            wait_until_ready(named_proc, port)
            try:
                isctest.query.udp(
                    query,
                    SERVER_IP,
                    port=port,
                    timeout=1,
                    attempts=1,
                    expected_rcode=dns.rcode.FORMERR,
                )
            except dns.exception.Timeout:
                returncode = named_proc.poll()
                if returncode is not None:
                    pytest.fail(f"TKEY query crashed named (exit code {returncode})")
                raise
            assert named_proc.poll() is None, "TKEY query crashed named"
        finally:
            terminate_named(named_proc)

    assert named_proc.returncode == 0, "named did not shut down cleanly"
