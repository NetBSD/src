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
import struct
import time

import dns.flags
import dns.message
import dns.name
import dns.query
import dns.rrset
import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
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


def test_tcp_garbage(named_port):
    with create_socket("10.53.0.7", named_port) as sock:
        msg = create_msg("a.example.", "A")
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        wire = msg.to_wire()
        assert len(wire) > 0

        # Send DNS message shorter than DNS message header (12),
        # this should cause the connection to be terminated
        sock.send(struct.pack("!H", 11))
        sock.send(struct.pack("!s", b"0123456789a"))

        with pytest.raises(EOFError):
            try:
                dns.query.send_tcp(sock, msg, timeout())
                dns.query.receive_tcp(sock, timeout())
            except ConnectionError as e:
                raise EOFError from e


def test_tcp_garbage_response(named_port):
    with create_socket("10.53.0.7", named_port) as sock:
        msg = create_msg("a.example.", "A")
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        wire = msg.to_wire()
        assert len(wire) > 0

        # Send DNS response instead of DNS query, this should cause
        # the connection to be terminated

        rmsg = dns.message.make_response(msg)
        dns.query.send_tcp(sock, rmsg, timeout())

        with pytest.raises(EOFError):
            try:
                dns.query.receive_tcp(sock, timeout())
            except ConnectionError as e:
                raise EOFError from e


# Regression test for CVE-2022-0396
def test_close_wait(named_port):
    with create_socket("10.53.0.7", named_port) as sock:
        msg = create_msg("a.example.", "A")
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        msg = dns.message.make_query("a.example.", "A", use_edns=0, payload=1232)
        dns.query.send_tcp(sock, msg, timeout())

        # Shutdown the socket, but ignore the other side closing the socket
        # first because we sent DNS message with EDNS0
        try:
            sock.shutdown(socket.SHUT_RDWR)
        except ConnectionError:
            pass
        except OSError:
            pass

    # BIND allows one TCP client, the part above sends DNS messaage with EDNS0
    # after the first query. BIND should react adequately because of
    # ns7/named.dropedns and close the socket, making room for the next
    # request. If it gets stuck in CLOSE_WAIT state, there is no connection
    # available for the query below and it will time out.
    with create_socket("10.53.0.7", named_port) as sock:
        msg = create_msg("a.example.", "A")
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())


# GL #4273
def test_tcp_big(named_port):
    with create_socket("10.53.0.7", named_port) as sock:
        msg = dns.message.Message(id=0)
        msg.flags = dns.flags.RD
        msg.question.append(dns.rrset.from_text(dns.name.root, 0, 1, "URI"))
        msg.additional.append(
            dns.rrset.from_text(dns.name.root, 0, 1, "URI", "0 0 " + "b" * 65503)
        )
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        # Now check that the server is alive and well
        msg = create_msg("a.example.", "A")
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())
