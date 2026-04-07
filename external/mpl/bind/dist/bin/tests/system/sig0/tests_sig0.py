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

import base64
import glob
import os
import struct
import time

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding, rsa

import dns.flags
import dns.message
import dns.name
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.renderer
import dns.rrset

import isctest


def load_bind_private_key(filename):
    """Parses a BIND 9 .private key file."""
    with open(filename, "r", encoding="utf-8") as f:
        lines = f.readlines()

    data = {}
    for line in lines:
        if ":" in line:
            key, value = line.split(":", 1)
            data[key.strip()] = value.strip()

    def b64int(k):
        return int.from_bytes(base64.b64decode(data[k]), byteorder="big")

    rsa_key = rsa.RSAPrivateNumbers(
        p=b64int("Prime1"),
        q=b64int("Prime2"),
        d=b64int("PrivateExponent"),
        dmp1=b64int("Exponent1"),
        dmq1=b64int("Exponent2"),
        iqmp=b64int("Coefficient"),
        public_numbers=rsa.RSAPublicNumbers(
            e=b64int("PublicExponent"), n=b64int("Modulus")
        ),
    ).private_key(default_backend())

    return rsa_key


def make_sig0_query(key_file, key_name_str):
    private_key = load_bind_private_key(key_file)

    qname = dns.name.from_text(".")
    query = dns.message.make_query(qname, dns.rdatatype.SOA)
    query.flags |= dns.flags.RD

    # Render message to bytes (needed for signing)
    renderer = dns.renderer.Renderer()
    query.to_wire(renderer)
    msg_bytes = renderer.get_wire()

    # SIG(0) Constants
    basename = os.path.basename(key_file)
    key_tag = int(basename.split("+")[2].split(".")[0])

    now = int(time.time())
    expiration = now + 3600
    inception = now - 3600
    signer_name = dns.name.from_text(key_name_str)

    # Construct SIG RDATA header (0=SIG(0), 8=RSASHA256, 0=Labels)
    sig_rdata_header = struct.pack(
        "!HBBIIIH", 0, 8, 0, 0, expiration, inception, key_tag
    )

    sig_rdata_pre_sig = sig_rdata_header + signer_name.to_wire()

    # Sign: ( SIG RDATA sans signature ) + ( Message )
    signature = private_key.sign(
        sig_rdata_pre_sig + msg_bytes, padding.PKCS1v15(), hashes.SHA256()
    )

    # Create the SIG RR
    full_sig_rdata = sig_rdata_pre_sig + signature
    sig_rr = dns.rdata.from_wire(
        dns.rdataclass.ANY,
        dns.rdatatype.SIG,
        full_sig_rdata,
        0,
        len(full_sig_rdata),
    )
    sig_rrset = dns.rrset.from_rdata(qname, 0, sig_rr)
    query.additional.append(sig_rrset)

    return query


def test_sig0_acl_bypass():
    key_files = glob.glob("Ksig0.+*.private")
    assert len(key_files) == 1

    query = make_sig0_query(key_files[0], "sig0.")

    # Send the query
    res = isctest.query.tcp(query, "10.53.0.1")
    isctest.check.servfail(res)
