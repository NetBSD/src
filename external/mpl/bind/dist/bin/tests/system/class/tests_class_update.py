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

from dns import message, rdataclass, rdatatype, update

import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/*.db",
    ]
)


def encode_name(name: str) -> bytes:
    out = b""
    for label in name.rstrip(".").split("."):
        out += bytes([len(label)]) + label.encode("ascii")
    return out + b"\x00"


@pytest.mark.parametrize(
    "rdtype,rdclass,ttl,rdata",
    [
        (rdatatype.SRV, rdataclass.NONE, 0, b"\x00\x00\x00\x00\x00\x00\x01"),
        (rdatatype.SRV, rdataclass.NONE, 0, b"\x00"),
        (rdatatype.KX, rdataclass.NONE, 0, b""),
        (rdatatype.PX, rdataclass.NONE, 0, b""),
        (rdatatype.NSAP, rdataclass.NONE, 0, b""),
        (rdatatype.NSAP_PTR, rdataclass.NONE, 0, b""),
        (31, rdataclass.NONE, 0, b""),  # dnspython doesn't define type EID
    ],
)
def test_class_invalid(rdtype, rdclass, ttl, rdata, named_port):
    # these update messages are badly formatted, so we construct
    # them manually instead of using dnspython.

    # opcode=UPDATE, 1 RRset in ZONE, 1 RRset in UPDATE
    header = struct.pack("!HHHHHH", 0, 0x2800, 1, 0, 1, 0)

    # ZONE section: QNAME=<zone>, QTYPE=SOA, QCLASS=ANY
    zone_q = encode_name("1.0.0.127.in-addr.arpa") + struct.pack("!HH", 6, 255)

    # UPDATE section RR:
    update_rr = (
        encode_name("1.0.0.127.in-addr.arpa")
        + struct.pack("!HHIH", rdtype, rdclass, ttl, len(rdata))
        + rdata
    )

    m = header + zone_q + update_rr
    packet = struct.pack("!H", len(m)) + m

    with socket.create_connection(
        ("10.53.0.2", named_port), source_address=("127.0.0.1", 0), timeout=2.0
    ) as s:
        s.sendall(packet)
        try:
            rwire = s.recv(4096)
            res = message.from_wire(rwire)
            isctest.check.formerr(res)
        except Exception:  # pylint: disable=broad-except
            pass

    # check the server is answering
    msg = isctest.query.create("1.0.0.127.in-addr.arpa", "SRV")
    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.noerror(res)
    isctest.check.rr_count_eq(res.answer, 0)


@pytest.mark.parametrize(
    "rdtype,rdata",
    [
        (rdatatype.SVCB, "\\# 02 0000"),
        (rdatatype.WKS, "\\# 02 4142"),
        (rdatatype.WKS, "\\# 02 4344"),
    ],
)
def test_class_chaosupdate(rdtype, rdata):
    up = update.UpdateMessage("example.", rdclass=rdataclass.CHAOS)
    up.add("foo.example.", 300, rdtype, rdata)
    res = isctest.query.tcp(up, "10.53.0.2")
    isctest.check.notimp(res)


def test_class_undefined(ns2):
    up = update.UpdateMessage(".", rdclass=257)
    up.present(".", 0)
    up.answer[0].rdclass = rdataclass.NONE
    with ns2.watch_log_from_here() as watcher:
        res = isctest.query.tcp(up, "10.53.0.2")
        isctest.check.notimp(res)
        watcher.wait_for_line("invalid message class: CLASS257")


def test_class_zero(ns2):
    up = update.UpdateMessage(".", rdclass=0)
    up.present(".", 0)
    up.answer[0].rdclass = rdataclass.NONE
    with ns2.watch_log_from_here() as watcher:
        res = isctest.query.tcp(up, "10.53.0.2")
        isctest.check.formerr(res)
        watcher.wait_for_line("message class could not be determined")


def test_class_any(ns2):
    up = update.UpdateMessage(".", rdclass=rdataclass.ANY)
    up.present(".", 0)
    up.answer[0].rdclass = rdataclass.NONE
    with ns2.watch_log_from_here() as watcher:
        res = isctest.query.tcp(up, "10.53.0.2")
        isctest.check.formerr(res)
        watcher.wait_for_line("message parsing failed: FORMERR")


def test_class_none(ns2):
    up = update.UpdateMessage(".", rdclass=rdataclass.NONE)
    up.present(".", 0)
    up.answer[0].rdclass = rdataclass.NONE
    with ns2.watch_log_from_here() as watcher:
        res = isctest.query.tcp(up, "10.53.0.2")
        isctest.check.formerr(res)
        watcher.wait_for_line("message parsing failed: FORMERR")
