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

from typing import SupportsInt

import socket

import dns.flags
import dns.name
import dns.opcode
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.NSEC3
import dns.rdtypes.ANY.OPT
import dns.rdtypes.ANY.RRSIG
import dns.rdtypes.ANY.SOA
import dns.rdtypes.ANY.TSIG
import dns.rdtypes.IN.A
import pytest


def wire(*parts: bytes) -> bytes:
    return b"".join(parts)


def u16(value: SupportsInt) -> bytes:
    return int(value).to_bytes(2, byteorder="big")


def u32(value: SupportsInt) -> bytes:
    return int(value).to_bytes(4, byteorder="big")


def name(text: str) -> bytes:
    return dns.name.from_text(text).to_wire()


def name_pointer(offset: int) -> bytes:
    return u16(0xC000 | offset)


def root() -> bytes:
    return dns.name.root.to_wire()


def header(
    message_id: int = 0,
    flags: int = 0,
    opcode: int = dns.opcode.QUERY,
    rcode: dns.rcode.Rcode = dns.rcode.NOERROR,
    qdcount: int = 0,
    ancount: int = 0,
    nscount: int = 0,
    arcount: int = 0,
) -> bytes:
    return wire(
        u16(message_id),
        u16(flags | dns.opcode.to_flags(opcode) | int(rcode)),
        u16(qdcount),
        u16(ancount),
        u16(nscount),
        u16(arcount),
    )


def formerr_response_header(
    message_id: int = 0,
    opcode: int = dns.opcode.QUERY,
    rcode: dns.rcode.Rcode = dns.rcode.FORMERR,
    qdcount: int = 0,
    ancount: int = 0,
    nscount: int = 0,
    arcount: int = 0,
) -> bytes:
    return header(
        message_id=message_id,
        flags=dns.flags.QR,
        opcode=opcode,
        rcode=rcode,
        qdcount=qdcount,
        ancount=ancount,
        nscount=nscount,
        arcount=arcount,
    )


def question(
    qname: bytes,
    qtype: dns.rdatatype.RdataType = dns.rdatatype.RdataType.A,
    qclass: dns.rdataclass.RdataClass = dns.rdataclass.RdataClass.IN,
) -> bytes:
    return wire(
        qname,
        u16(qtype),
        u16(qclass),
    )


def rr(
    owner: bytes,
    rrtype: SupportsInt,
    rrclass: SupportsInt,
    *,
    ttl: int = 1,
    rdata: bytes = b"",
) -> bytes:
    return wire(
        question(
            owner, dns.rdatatype.RdataType(rrtype), dns.rdataclass.RdataClass(rrclass)
        ),
        u32(ttl),
        u16(len(rdata)),
        rdata,
    )


def oversized_name() -> bytes:
    labels = [bytes([15]) + b"A" * 15 for _ in range(15)]
    labels.append(bytes([14]) + b"A" * 14 + root())
    return wire(*labels)


def soa_rr(
    *,
    minimum: int,
) -> bytes:
    return rr(
        root(),
        dns.rdatatype.RdataType.SOA,
        dns.rdataclass.RdataClass.IN,
        rdata=dns.rdtypes.ANY.SOA.SOA(
            dns.rdataclass.RdataClass.IN,
            dns.rdatatype.RdataType.SOA,
            dns.name.root,
            dns.name.root,
            1,
            2,
            3,
            4,
            minimum,
        ).to_wire(),
    )


def nsec3_rr(
    *,
    owner: bytes,
) -> bytes:
    return rr(
        owner,
        dns.rdatatype.RdataType.NSEC3,
        dns.rdataclass.RdataClass.IN,
        rdata=dns.rdtypes.ANY.NSEC3.NSEC3(
            dns.rdataclass.RdataClass.IN,
            dns.rdatatype.RdataType.NSEC3,
            algorithm=240,
            flags=0,
            iterations=0,
            salt=b"",
            next=b"\xff",
            windows=[],
        ).to_wire(),
    )


def key_rdata(
    *,
    flags: int,
    protocol: int,
    algorithm: int,
    keydata: bytes,
) -> bytes:
    # No dns.rdtypes.ANY.KEY.KEY class, so construct the rdata manually
    return wire(u16(flags), bytes([protocol, algorithm]), keydata)


def key_rr(*, rdclass: dns.rdataclass.RdataClass) -> bytes:
    return rr(
        root(),
        dns.rdatatype.RdataType.KEY,
        rdclass,
        rdata=key_rdata(
            flags=0,
            protocol=0,
            algorithm=248,
            keydata=b"\x00",
        ),
    )


def malformed_rrsig_rr() -> bytes:
    return rr(
        root(),
        dns.rdatatype.RdataType.RRSIG,
        dns.rdataclass.RdataClass.IN,
        rdata=dns.rdtypes.ANY.RRSIG.RRSIG(
            dns.rdataclass.RdataClass.IN,
            dns.rdatatype.RdataType.RRSIG,
            0,
            240,
            0,
            1,
            2,
            3,
            0,
            dns.name.root,
            b"\x00",
        ).to_wire(),
    )


def tsig_rr(
    *,
    owner: bytes = root(),
    rdclass: dns.rdataclass.RdataClass = dns.rdataclass.RdataClass.ANY,
    algorithm: dns.name.Name = dns.name.root,
    time_signed: int = 0x010203040506,
    fudge: int = 0x0102,
    mac: bytes = b"\x00",
    original_id: int = 0,
    error: int = 0,
    other: bytes = b"",
) -> bytes:
    return rr(
        owner,
        dns.rdatatype.RdataType.TSIG,
        rdclass,
        rdata=dns.rdtypes.ANY.TSIG.TSIG(
            rdclass,
            dns.rdatatype.RdataType.TSIG,
            algorithm,
            time_signed,
            fudge,
            mac,
            original_id,
            error,
            other,
        ).to_wire(),
    )


def opt_rr(*, owner: bytes) -> bytes:
    return rr(
        owner,
        dns.rdatatype.RdataType.OPT,
        dns.rdataclass.RdataClass.IN,
        ttl=0,
        rdata=dns.rdtypes.ANY.OPT.OPT(
            dns.rdataclass.RdataClass.IN,
            dns.rdatatype.RdataType.OPT,
            [],
        ).to_wire(),
    )


def a_rdata(ipv4_bytes: bytes = b"\x00\x00\x00\x00") -> bytes:
    return dns.rdtypes.IN.A.A(
        dns.rdataclass.RdataClass.IN,
        dns.rdatatype.RdataType.A,
        ipv4_bytes,
    ).to_wire()


def a_rr(owner: bytes = root()) -> bytes:
    return rr(
        owner,
        dns.rdatatype.RdataType.A,
        dns.rdataclass.RdataClass.IN,
        rdata=a_rdata(),
    )


def query_raw_tcp(host: str, port: int, packet_wire: bytes) -> bytes:
    with (
        socket.create_connection((host, port), timeout=10) as sock,
        sock.makefile("rwb") as f,
    ):
        f.write(u16(len(packet_wire)))
        f.write(packet_wire)
        f.flush()
        length = int.from_bytes(f.read(2), byteorder="big")
        return f.read(length)


@pytest.mark.parametrize(
    "query_wire,expected_wire",
    [
        pytest.param(
            wire(
                header(qdcount=1),
                question(oversized_name()),
            ),
            formerr_response_header(),
            id="nametoolong",
        ),
        pytest.param(
            wire(
                header(qdcount=2),
                question(name("AAAAAAAAAAAAAA."), dns.rdatatype.RdataType.A),
                # Two names concatenated in the QNAME field
                question(
                    wire(name("AAAAAAAAAAAAAA."), name("AAAAAAAAAAAAAB.")),
                    dns.rdatatype.RdataType.A,
                ),
            ),
            formerr_response_header(),
            id="twoquestionnames",
        ),
        pytest.param(
            wire(
                header(qdcount=2),
                question(name("AAAAAAAAAAAAAA."), dns.rdatatype.RdataType.A),
                question(name("AAAAAAAAAAAAAA."), dns.rdatatype.RdataType.NS),
            ),
            wire(
                formerr_response_header(qdcount=2),
                question(name("AAAAAAAAAAAAAA."), dns.rdatatype.RdataType.A),
                question(name_pointer(12), dns.rdatatype.RdataType.NS),
            ),
            id="twoquestiontypes",
        ),
        pytest.param(
            wire(
                header(qdcount=2),
                question(name("AAAAAAAAAAAAAA."), dns.rdatatype.RdataType.A),
                question(name("AAAAAAAAAAAAAA."), dns.rdatatype.RdataType.A),
            ),
            formerr_response_header(),
            id="dupquestion",
        ),
        pytest.param(
            wire(
                header(qdcount=1, ancount=2),
                question(root(), dns.rdatatype.RdataType.SOA),
                soa_rr(minimum=5),
                soa_rr(minimum=6),
            ),
            wire(
                formerr_response_header(qdcount=1),
                question(root(), dns.rdatatype.RdataType.SOA),
            ),
            id="dupans",
        ),
        pytest.param(
            wire(
                header(ancount=1),
                rr(
                    root(),
                    dns.rdatatype.RdataType.MAILB,
                    dns.rdataclass.RdataClass.IN,
                ),
            ),
            formerr_response_header(),
            id="qtypeasanswer",
        ),
        pytest.param(
            header(),
            # This would be NOERROR if it included a COOKIE option,
            # but is a FORMERR without one.
            formerr_response_header(),
            id="noquestions",
        ),
        pytest.param(
            wire(
                header(qdcount=1, nscount=1),
                question(root(), dns.rdatatype.RdataType.A),
                # Bad NSEC3 owner: X. is not in the base32hex alphabet.
                nsec3_rr(owner=name("X.")),
            ),
            wire(
                formerr_response_header(rcode=dns.rcode.SERVFAIL, qdcount=1),
                question(root(), dns.rdatatype.RdataType.A),
            ),
            id="badnsec3owner",
        ),
        pytest.param(
            wire(
                header(arcount=1),
                # Truncated A record (no ttl, length or data)
                question(root(), dns.rdatatype.RdataType.A),
            ),
            formerr_response_header(),
            id="shortrecord",
        ),
        pytest.param(
            wire(
                header(qdcount=1),
                # Truncated question (no class)
                root(),
                u16(dns.rdatatype.RdataType.A),
            ),
            formerr_response_header(),
            id="shortquestion",
        ),
        pytest.param(
            wire(
                header(qdcount=2),
                question(
                    root(),
                    dns.rdatatype.RdataType.A,
                    dns.rdataclass.RdataClass.IN,
                ),
                question(
                    root(),
                    dns.rdatatype.RdataType.A,
                    dns.rdataclass.RdataClass(2),
                ),
            ),
            formerr_response_header(),
            id="twoquestionclasses",
        ),
        pytest.param(
            wire(
                header(arcount=1),
                a_rr(owner=oversized_name()),
            ),
            formerr_response_header(),
            id="badrecordname",
        ),
        pytest.param(
            wire(
                header(arcount=2),
                a_rr(),
                rr(
                    root(),
                    dns.rdatatype.RdataType(65280),
                    dns.rdataclass.RdataClass(256),
                    rdata=a_rdata(),
                ),
            ),
            formerr_response_header(),
            id="wrongclass",
        ),
        pytest.param(
            wire(
                header(qdcount=1, arcount=1),
                question(root(), dns.rdatatype.RdataType.A),
                key_rr(rdclass=dns.rdataclass.RdataClass(2)),
            ),
            wire(
                formerr_response_header(qdcount=1),
                question(root(), dns.rdatatype.RdataType.A),
            ),
            id="keyclass",
        ),
        pytest.param(
            wire(
                header(arcount=1),
                # OPT owner should be root
                opt_rr(owner=name("A.")),
            ),
            formerr_response_header(),
            id="optwrongname",
        ),
        pytest.param(
            wire(
                header(arcount=1),
                malformed_rrsig_rr(),
            ),
            formerr_response_header(),
            id="malformedrrsig",
        ),
        pytest.param(
            wire(
                header(opcode=dns.opcode.UPDATE, nscount=1),
                rr(
                    root(),
                    dns.rdatatype.RdataType.A,
                    dns.rdataclass.RdataClass.ANY,
                    ttl=0,
                    # Non-empty rdata for DELETE type
                    rdata=b"\x00",
                ),
            ),
            formerr_response_header(opcode=dns.opcode.UPDATE),
            id="malformeddeltype",
        ),
        pytest.param(
            wire(
                header(arcount=1),
                # Class should be ANY not IN
                tsig_rr(rdclass=dns.rdataclass.RdataClass.IN),
            ),
            formerr_response_header(),
            id="tsigwrongclass",
        ),
        pytest.param(
            wire(
                # Test non-zero message ID is preserved in the response
                header(message_id=67, arcount=2),
                tsig_rr(),
                # TSIG should be the last record
                a_rr(),
            ),
            formerr_response_header(message_id=67),
            id="tsignotlast",
        ),
    ],
)
def test_formerr(
    query_wire: bytes,
    expected_wire: bytes,
    named_port: int,
    ns1,
) -> None:
    response_wire = query_raw_tcp(ns1.ip, named_port, query_wire)
    assert response_wire == expected_wire
