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

pytest.importorskip("dns", minversion="2.0.0")
import dns.message
import dns.query

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


# Regression test for CVE-2022-0396
def test_close_wait(named_port):
    with create_socket("10.53.0.7", named_port) as sock:

        msg = create_msg("a.example.", "A")
        (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
        (response, rtime) = dns.query.receive_tcp(sock, timeout())

        msg = dns.message.make_query("a.example.", "A", use_edns=0, payload=1232)
        (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())

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
        (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
        (response, rtime) = dns.query.receive_tcp(sock, timeout())
