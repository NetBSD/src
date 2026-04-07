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

import socket
import time

import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns2/nil.db",
        "ns2/other.db",
        "ns2/secondkey.conf",
        "ns2/static.db",
        "ns4/example.db",
        "ns4/key*.conf",
        "ns6/huge.zone.db",
        "ns7/include.db",
        "ns7/test.db",
    ]
)


def test_cve_2023_3341(control_port):
    depth = 4500
    # Should not be more than isccc_ccmsg_setmaxsize(&conn->ccmsg, 32768)
    total_len = 10 + (depth * 7) - 6

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        data = b"".join(
            [
                total_len.to_bytes(4, "big"),  # <total lenght>
                b"\x00\x00\x00\x01",  # <version>
                b"\x01\x41",  # <size><name>
            ]
        )

        for i in range(depth, 0, -1):
            l = (i - 1) * 7
            t = b"".join(
                [
                    b"\x02",  # ISCCC_CCMSGTYPE_TABLE
                    l.to_bytes(4, "big"),  # <size>
                    b"\x01\x41",  # <size><name>
                ]
            )
            data = b"".join([data, t])

        s.connect(("10.53.0.2", control_port))
        s.sendall(data)

    # Wait for named to (possibly) crash
    time.sleep(10)

    msg = isctest.query.create("version.bind", "TXT", "CH")
    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.noerror(res)
