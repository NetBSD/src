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

# pylint: disable=unused-variable

import socket
import time

import pytest

# isctest.asyncserver requires dnspython >= 2.0.0
pytest.importorskip("dns", minversion="2.0.0")

import dns.message
import dns.query
import dns.tsigkeyring

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
        "ns1/named-fips.conf",
    ]
)

TIMEOUT = 10


def create_msg(qname, qtype, edns=-1):
    msg = dns.message.make_query(qname, qtype, use_edns=edns)
    return msg


def timeout():
    return time.time() + TIMEOUT


def create_socket(host, port):
    sock = socket.create_connection((host, port), timeout=10)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
    return sock


def test_tsig_badtime(named_port):
    with create_socket("10.53.0.1", named_port) as sock:
        msg = create_msg("a.example.", "A")

        keyring = dns.tsigkeyring.from_text(
            {
                "sha256": "R16NojROxtxH/xbDl//ehDsHm5DjWTQ2YXV+hGC2iBY=",
            }
        )

        msg.use_tsig(keyring, keyname="sha256", fudge=0)

        wire = msg.to_wire()
        assert len(wire) > 0

        time.sleep(3)

        dns.query.send_tcp(sock, wire, timeout())
        with pytest.raises(dns.tsig.PeerBadTime):
            dns.query.receive_tcp(sock, timeout(), keyring=keyring)
