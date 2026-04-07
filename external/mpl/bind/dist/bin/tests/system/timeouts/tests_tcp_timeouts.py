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

import dns.edns
import dns.message
import dns.name
import dns.query
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest.mark

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns1/large.db",
    ]
)

TIMEOUT = 10


def create_msg(qname, qtype):
    msg = dns.message.make_query(
        qname, qtype, want_dnssec=True, use_edns=0, payload=4096
    )
    return msg


def timeout():
    return time.time() + TIMEOUT


def test_initial_timeout(named_port):
    #
    # The initial timeout is 2.5 seconds, so this should timeout
    #
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        time.sleep(3)

        msg = create_msg("example.", "A")

        with pytest.raises(EOFError):
            try:
                dns.query.send_tcp(sock, msg, timeout())
                dns.query.receive_tcp(sock, timeout())
            except ConnectionError as e:
                raise EOFError from e


@pytest.mark.flaky(max_runs=2, rerun_filter=isctest.mark.is_host_freebsd_13)
def test_idle_timeout(named_port):
    #
    # The idle timeout is 5 seconds, so the third message should fail
    #
    msg = create_msg("example.", "A")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        time.sleep(1)

        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        time.sleep(2)

        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        time.sleep(6)

        with pytest.raises(EOFError):
            try:
                dns.query.send_tcp(sock, msg, timeout())
                dns.query.receive_tcp(sock, timeout())
            except ConnectionError as e:
                raise EOFError from e


def test_keepalive_timeout(named_port):
    #
    # Keepalive is 7 seconds, so the third message should succeed.
    #
    msg = create_msg("example.", "A")
    kopt = dns.edns.GenericOption(11, b"\x00")
    msg.use_edns(edns=True, payload=4096, options=[kopt])

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        time.sleep(1)

        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        time.sleep(2)

        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        time.sleep(6)

        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())


def test_pipelining_timeout(named_port):
    #
    # The pipelining should only timeout after the last message is received
    #
    msg = create_msg("example.", "A")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        time.sleep(1)

        # Send and receive 25 DNS queries
        for _ in range(25):
            dns.query.send_tcp(sock, msg, timeout())
        for _ in range(25):
            dns.query.receive_tcp(sock, timeout())

        time.sleep(3)

        # Send and receive 25 DNS queries
        for _ in range(25):
            dns.query.send_tcp(sock, msg, timeout())
        for _ in range(25):
            dns.query.receive_tcp(sock, timeout())

        time.sleep(6)

        with pytest.raises(EOFError):
            try:
                dns.query.send_tcp(sock, msg, timeout())
                dns.query.receive_tcp(sock, timeout())
            except ConnectionError as e:
                raise EOFError from e


def test_long_axfr(named_port):
    #
    # The timers should not fire during AXFR, thus the connection should not
    # close abruptly
    #
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        name = dns.name.from_text("example.")
        msg = create_msg("example.", "AXFR")
        dns.query.send_tcp(sock, msg, timeout())

        # Receive the initial DNS message with SOA
        response, _ = dns.query.receive_tcp(sock, timeout(), one_rr_per_rrset=True)
        soa = response.get_rrset(
            dns.message.ANSWER, name, dns.rdataclass.IN, dns.rdatatype.SOA
        )
        assert soa is not None

        # Pull DNS message from wire until the second SOA is received
        while True:
            response, _ = dns.query.receive_tcp(sock, timeout(), one_rr_per_rrset=True)
            soa = response.get_rrset(
                dns.message.ANSWER, name, dns.rdataclass.IN, dns.rdatatype.SOA
            )
            if soa is not None:
                break
        assert soa is not None


@pytest.mark.flaky(max_runs=3)
def test_send_timeout(named_port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        # Send and receive single large RDATA over TCP
        msg = create_msg("large.example.", "TXT")
        dns.query.send_tcp(sock, msg, timeout())
        dns.query.receive_tcp(sock, timeout())

        # Send and receive 28 large (~32k) DNS queries that should
        # fill the default maximum 208k TCP send buffer
        for _ in range(28):
            dns.query.send_tcp(sock, msg, timeout())

        # configure idle interval is 5 seconds, sleep 6 to make sure we are
        # above the interval
        time.sleep(6)

        with pytest.raises(EOFError):
            try:
                dns.query.send_tcp(sock, msg, timeout())
                dns.query.receive_tcp(sock, timeout())
            except ConnectionError as e:
                raise EOFError from e


@isctest.mark.long_test
def test_max_transfer_idle_out(named_port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        name = dns.name.from_text("example.")
        msg = create_msg("example.", "AXFR")
        dns.query.send_tcp(sock, msg, timeout())

        # Receive the initial DNS message with SOA
        response, _ = dns.query.receive_tcp(sock, timeout(), one_rr_per_rrset=True)
        soa = response.get_rrset(
            dns.message.ANSWER, name, dns.rdataclass.IN, dns.rdatatype.SOA
        )
        assert soa is not None

        time.sleep(61)  # max-transfer-idle-out is 1 minute

        with pytest.raises(ConnectionResetError):
            # Process queued TCP messages
            while True:
                response, _ = dns.query.receive_tcp(
                    sock, timeout(), one_rr_per_rrset=True
                )
                soa = response.get_rrset(
                    dns.message.ANSWER, name, dns.rdataclass.IN, dns.rdatatype.SOA
                )
                if soa is not None:
                    break
            assert soa is None


@isctest.mark.long_test
def test_max_transfer_time_out(named_port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect(("10.53.0.1", named_port))

        name = dns.name.from_text("example.")
        msg = create_msg("example.", "AXFR")
        dns.query.send_tcp(sock, msg, timeout())

        # Receive the initial DNS message with SOA
        response, _ = dns.query.receive_tcp(sock, timeout(), one_rr_per_rrset=True)
        soa = response.get_rrset(
            dns.message.ANSWER, name, dns.rdataclass.IN, dns.rdatatype.SOA
        )
        assert soa is not None

        # The loop should timeout at the 5 minutes (max-transfer-time-out)
        with pytest.raises(EOFError):
            while True:
                time.sleep(1)
                response, _ = dns.query.receive_tcp(
                    sock, timeout(), one_rr_per_rrset=True
                )
                soa = response.get_rrset(
                    dns.message.ANSWER, name, dns.rdataclass.IN, dns.rdatatype.SOA
                )
                if soa is not None:
                    break
        assert soa is None
