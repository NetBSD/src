#!/usr/bin/python3
############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

# pylint: disable=unused-variable

import socket
import time

import pytest

TIMEOUT = 10


def create_msg(qname, qtype):
    import dns.message
    msg = dns.message.make_query(qname, qtype, want_dnssec=True,
                                 use_edns=0, payload=4096)

    return msg


def timeout():
    return time.time() + TIMEOUT


@pytest.mark.dnspython
@pytest.mark.dnspython2
def test_initial_timeout(port):
    #
    # The initial timeout is 2.5 seconds, so this should timeout
    #
    import dns.query

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", port))

        time.sleep(3)

        msg = create_msg("example.", "A")

        with pytest.raises(EOFError):
            try:
                (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
                (response, rtime) = dns.query.receive_tcp(sock, timeout())
            except ConnectionResetError as e:
                raise EOFError from e


@pytest.mark.dnspython
@pytest.mark.dnspython2
def test_idle_timeout(port):
    #
    # The idle timeout is 5 second, so sending the second message must fail
    #
    import dns.rcode

    msg = create_msg("example.", "A")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", port))

        time.sleep(1)

        (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
        (response, rtime) = dns.query.receive_tcp(sock, timeout())

        time.sleep(3)

        (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
        (response, rtime) = dns.query.receive_tcp(sock, timeout())

        time.sleep(6)

        with pytest.raises(EOFError):
            try:
                (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
                (response, rtime) = dns.query.receive_tcp(sock, timeout())
            except ConnectionResetError as e:
                raise EOFError from e


@pytest.mark.dnspython
@pytest.mark.dnspython2
def test_pipelining_timeout(port):
    #
    # The pipelining should only timeout after the last message is received
    #
    import dns.query

    msg = create_msg("example.", "A")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", port))

        time.sleep(1)

        # Send and receive 25 DNS queries
        for n in range(25):
            (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
        for n in range(25):
            (response, rtime) = dns.query.receive_tcp(sock, timeout())

        time.sleep(3)

        # Send and receive 25 DNS queries
        for n in range(25):
            (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
        for n in range(25):
            (response, rtime) = dns.query.receive_tcp(sock, timeout())

        time.sleep(6)

        with pytest.raises(EOFError):
            try:
                (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())
                (response, rtime) = dns.query.receive_tcp(sock, timeout())
            except ConnectionResetError as e:
                raise EOFError from e


@pytest.mark.dnspython
@pytest.mark.dnspython2
def test_long_axfr(port):
    #
    # The timers should not fire during AXFR, thus the connection should not
    # close abruptly
    #
    import dns.query
    import dns.rdataclass
    import dns.rdatatype

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", port))

        name = dns.name.from_text("example.")
        msg = create_msg("example.", "AXFR")
        (sbytes, stime) = dns.query.send_tcp(sock, msg, timeout())

        # Receive the initial DNS message with SOA
        (response, rtime) = dns.query.receive_tcp(sock, timeout(),
                                                  one_rr_per_rrset=True)
        soa = response.get_rrset(dns.message.ANSWER, name,
                                 dns.rdataclass.IN, dns.rdatatype.SOA)
        assert soa is not None

        # Pull DNS message from wire until the second SOA is received
        while True:
            (response, rtime) = dns.query.receive_tcp(sock, timeout(),
                                                      one_rr_per_rrset=True)
            soa = response.get_rrset(dns.message.ANSWER, name,
                                     dns.rdataclass.IN, dns.rdatatype.SOA)
            if soa is not None:
                break
        assert soa is not None
