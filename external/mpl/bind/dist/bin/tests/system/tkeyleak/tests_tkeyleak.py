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

"""
Regression test for GSS-API context leak via repeated TKEY queries.

An unauthenticated attacker could exhaust server memory by sending
repeated TKEY queries with crafted SPNEGO NegTokenInit tokens.
Each query triggers gss_accept_sec_context() which returns
GSS_S_CONTINUE_NEEDED and allocates a GSS context.  On the unfixed
code path, the context handle in process_gsstkey() is never stored
or freed, leaking ~520 bytes per query.

The fix rejects GSS_S_CONTINUE_NEEDED in dst_gssapi_acceptctx() and
deletes the context immediately.

The key distinguishing signal in the TKEY response:
  - CONTINUE (vulnerable): error=0, output token present, no TSIG
  - BADKEY (fixed):        error=17, no output token
"""

import struct
import time

import dns.name
import dns.query
import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.TKEY
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/*.db",
    ]
)

TKEY_NAME = dns.name.from_text("test.key.")
GSSAPI_ALGORITHM = dns.name.from_text("gss-tsig.")
TKEY_MODE_GSSAPI = 3

# OID 1.2.840.113554.1.2.2 (Kerberos 5)
KRB5_OID = b"\x06\x09\x2a\x86\x48\x86\xf7\x12\x01\x02\x02"

# OID 1.3.6.1.5.5.2 (SPNEGO)
SPNEGO_OID = b"\x06\x06\x2b\x06\x01\x05\x05\x02"


def der_encode(tag, data):
    """Encode data in ASN.1 DER TLV format."""
    length = len(data)
    if length < 128:
        return tag + bytes([length]) + data
    if length < 256:
        return tag + b"\x81" + bytes([length]) + data
    return tag + b"\x82" + struct.pack(">H", length) + data


def spnego_negtokeninit():
    """Build a SPNEGO NegTokenInit proposing krb5 without a mechToken.

    This forces gss_accept_sec_context() to return GSS_S_CONTINUE_NEEDED
    because the acceptor recognizes the krb5 mechanism but has not
    received an actual AP-REQ token yet.
    """
    # MechTypeList ::= SEQUENCE OF MechType
    mechtype_list = der_encode(b"\x30", KRB5_OID)
    # [0] mechTypes
    mechtypes = der_encode(b"\xa0", mechtype_list)
    # NegTokenInit ::= SEQUENCE { mechTypes, ... }
    negtokeninit = der_encode(b"\x30", mechtypes)
    # [0] CONSTRUCTED (wrapping NegTokenInit)
    wrapped = der_encode(b"\xa0", negtokeninit)
    # APPLICATION 0 CONSTRUCTED (SPNEGO OID + body)
    return der_encode(b"\x60", SPNEGO_OID + wrapped)


def make_tkey_query(token):
    """Build a TKEY query with a GSS-API token in the additional section."""
    now = int(time.time())
    tkey_rdata = dns.rdtypes.ANY.TKEY.TKEY(
        rdclass=dns.rdataclass.ANY,
        rdtype=dns.rdatatype.TKEY,
        algorithm=GSSAPI_ALGORITHM,
        inception=now,
        expiration=now + 86400,
        mode=TKEY_MODE_GSSAPI,
        error=0,
        key=token,
        other=b"",
    )

    msg = isctest.query.create(TKEY_NAME, dns.rdatatype.TKEY, dns.rdataclass.ANY)
    rrset = msg.find_rrset(
        msg.additional,
        TKEY_NAME,
        dns.rdataclass.ANY,
        dns.rdatatype.TKEY,
        create=True,
    )
    rrset.add(tkey_rdata)
    return msg


def test_tkey_gssapi_no_continuation(ns1):
    """TKEY with a SPNEGO NegTokenInit must be rejected, not continued.

    On unfixed code, gss_accept_sec_context() returns CONTINUE_NEEDED
    and the response has error=0 with an output token (the leaked path).
    On fixed code, CONTINUE_NEEDED is rejected and the response has
    error=BADKEY(17) with no output token.
    """
    port = ns1.ports.dns
    ip = ns1.ip

    msg = make_tkey_query(spnego_negtokeninit())
    res = dns.query.tcp(msg, ip, port=port, timeout=5)

    assert res is not None

    tkey = get_tkey_answer(res)
    assert tkey is not None, "server did not return a TKEY answer"
    assert (
        tkey.error != 0
    ), "server returned error=0 (GSS_S_CONTINUE_NEEDED not rejected)"
    assert len(tkey.key) == 0, "server returned a continuation token"


def get_tkey_answer(response):
    """Extract TKEY rdata from a DNS response, or None."""
    for rrset in response.answer:
        if rrset.rdtype == dns.rdatatype.TKEY:
            for rdata in rrset:
                return rdata
    return None
