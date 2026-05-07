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

import selectors
import struct
import subprocess
import time

import dns.exception
import dns.message
import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "gnutls-cli.*",
        "ns*/example*.db",
    ]
)


def test_gnutls_cli_query(gnutls_cli_executable, named_tlsport):
    # Prepare the example/SOA query which will be sent over TLS.
    query = isctest.query.create("example.", dns.rdatatype.SOA)
    query_wire = query.to_wire()
    query_with_length = struct.pack(">H", len(query_wire)) + query_wire

    # Run gnutls-cli.
    gnutls_cli_args = [
        gnutls_cli_executable,
        "--no-ca-verification",
        "-V",
        "--no-ocsp",
        "--alpn=dot",
        "--logfile=gnutls-cli.log",
        f"--port={named_tlsport}",
        "10.53.0.1",
    ]
    with open("gnutls-cli.err", "wb") as gnutls_cli_stderr, subprocess.Popen(
        gnutls_cli_args,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=gnutls_cli_stderr,
        bufsize=0,
    ) as gnutls_cli:
        # Send the example/SOA query to the standard input of gnutls-cli.  Do
        # not close standard input yet because that causes gnutls-cli to close
        # the TLS connection immediately, preventing the response from being
        # read.
        gnutls_cli.stdin.write(query_with_length)
        gnutls_cli.stdin.flush()

        # Keep reading data from the standard output of gnutls-cli until a full
        # DNS message is received or a timeout is exceeded or gnutls-cli exits.
        # Popen.communicate() cannot be used here because: a) it closes
        # standard input after sending data to the process (see above why this
        # is a problem), b) gnutls-cli is not DNS-aware, so it does not exit
        # upon receiving a DNS response.
        selector = selectors.DefaultSelector()
        selector.register(gnutls_cli.stdout, selectors.EVENT_READ)
        deadline = time.monotonic() + 10
        gnutls_cli_output = b""
        response = b""
        while not response and not gnutls_cli.poll():
            if not selector.select(timeout=deadline - time.monotonic()):
                break
            gnutls_cli_output += gnutls_cli.stdout.read(512)
            try:
                # Ignore TCP length, just try to parse a DNS message from
                # the rest of the data received.
                response = dns.message.from_wire(gnutls_cli_output[2:])
            except dns.exception.FormError:
                continue

        # At this point either a DNS response was received or a timeout fired
        # or gnutls-cli exited prematurely.  Close the standard input of
        # gnutls-cli.  Terminate it if that does not cause it to shut down
        # gracefully.
        gnutls_cli.stdin.close()
        try:
            gnutls_cli.wait(5)
        except subprocess.TimeoutExpired:
            gnutls_cli.kill()

    # Store the response received for diagnostic purposes.
    with open("gnutls-cli.out.bin", "wb") as response_bin:
        response_bin.write(gnutls_cli_output)
    if response:
        with open("gnutls-cli.out.txt", "w", encoding="utf-8") as response_txt:
            response_txt.write(response.to_text())

    # Check whether a response was received and whether it is sane.
    assert response
    assert query.id == response.id
    assert len(response.answer) == 1
    assert response.answer[0].match(
        dns.name.from_text("example."),
        dns.rdataclass.IN,
        dns.rdatatype.SOA,
        dns.rdatatype.NONE,
    )
