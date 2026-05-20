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

from h2.config import H2Configuration
from h2.connection import H2Connection
from h2.settings import SettingCodes

import dns.message


def test_settings_frame_flood(ns1, named_httpsport):
    msg = dns.message.make_query(".", "SOA")
    wire = msg.to_wire()

    with socket.create_connection((ns1.ip, named_httpsport), timeout=10) as sock:
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        ctx.set_alpn_protocols(["h2"])

        with ctx.wrap_socket(sock, server_hostname=ns1.ip) as tls:
            config = H2Configuration(client_side=True, header_encoding="utf-8")
            conn = H2Connection(config=config)
            conn.initiate_connection()
            tls.sendall(conn.data_to_send())

            stream_id = conn.get_next_available_stream_id()
            conn.send_headers(
                stream_id,
                [
                    (":method", "POST"),
                    (":path", "/dns-query"),
                    (":scheme", "https"),
                    (":authority", f"{ns1.ip}:{named_httpsport}"),
                    ("content-type", "application/dns-message"),
                    ("accept", "application/dns-message"),
                    ("content-length", str(len(wire))),
                ],
            )
            conn.send_data(stream_id, wire, end_stream=True)
            tls.sendall(conn.data_to_send())

            for i in range(4096):
                try:
                    conn.update_settings(
                        {
                            SettingCodes.MAX_CONCURRENT_STREAMS: (i % 100) + 1,
                            SettingCodes.INITIAL_WINDOW_SIZE: i + 1,
                        }
                    )
                    tls.sendall(conn.data_to_send())
                except Exception:  # pylint: disable=broad-except
                    break

                if i % 500 == 0:
                    tls.settimeout(0.05)
                    try:
                        while (data := tls.recv(65535)) != b"":
                            conn.receive_data(data)
                            tls.sendall(conn.data_to_send())
                    except Exception:  # pylint: disable=broad-except
                        pass
