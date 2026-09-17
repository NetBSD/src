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
import ssl
import struct
import time

from h2.connection import H2Connection

import dns.message
import dns.name
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

import isctest.mark

pytestmark = [isctest.mark.with_libnghttp2]


def sig0_wire():
    now = int(time.time())

    # dnspython doesn't seems to have a way to put this in a higher level way,
    # so let's construct the SIG(0) rdata "manually".
    # `!` means network byte order, `H` 2 bytes, `B` 1 byte, `I` 4 bytes.
    sig_rdata = (
        struct.pack(
            "!HBBIIIH",
            0,  # 0 as SIG(0) doesn't sign a specific RR type, but the whole message.
            8,  # RSA/SHA-256 (but it could by anything for this attack)
            0,  # Labels: SIG(0) does not describe a DNS name, so this is zero
            0,  # TTL
            now + 300,  # Expiration
            now - 300,  # Inception
            123,  # Key tag (could also be anything for this attack)
        )
        + (dns.name.from_text("sig0.invalid.").to_wire())
        + (b"\x00" * 256)  # The invalid signature
    )

    sig = dns.rdata.from_wire(
        dns.rdataclass.ANY,
        dns.rdatatype.SIG,
        sig_rdata,
        0,
        len(sig_rdata),
    )

    msg = dns.message.make_query(
        ".",
        dns.rdatatype.SOA,
        dns.rdataclass.IN,
    )

    msg.additional.append(
        dns.rrset.from_rdata(
            dns.name.root,
            0,
            sig,
        )
    )

    return msg.to_wire()


def test_sig0_doh(ns1, named_httpsport):
    msg = sig0_wire()

    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE

    with socket.create_connection((ns1.ip, named_httpsport)) as sock:
        with ctx.wrap_socket(sock) as conn:
            h2 = H2Connection()
            h2.initiate_connection()

            stream = h2.get_next_available_stream_id()
            h2.send_headers(
                stream,
                [
                    (":method", "POST"),
                    (":scheme", "https"),
                    (":authority", f"{ns1.ip}:{named_httpsport}"),
                    (":path", "/dns-query"),
                    ("content-type", "application/dns-message"),
                    ("content-length", str(len(msg))),
                ],
            )
            h2.send_data(stream, msg, end_stream=True)

            # Send the query and immediately close the connection.
            # named should gracefully handle the fact the connection is
            # now closed after it checked the message signature.
            conn.sendall(h2.data_to_send())

    # If the server succesfully restarted, it means it didn't crash.
    with ns1.watch_log_from_here() as watcher:
        ns1.rndc("reload")
        watcher.wait_for_line("running")
