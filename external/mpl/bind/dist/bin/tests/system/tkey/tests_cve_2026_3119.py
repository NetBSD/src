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

# pylint: disable=unused-variable

import time

import dns.message
import dns.rdataclass
import dns.rdatatype
import dns.rdtypes.ANY.TKEY
import dns.rrset
import dns.tsigkeyring
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts([])


def create_tkey_msg(qname, mode, alg="hmac-sha256"):
    msg = dns.message.make_query(qname, "TKEY")
    now = int(time.time())
    rdata = dns.rdtypes.ANY.TKEY.TKEY(
        rdclass=dns.rdataclass.ANY,
        rdtype=dns.rdatatype.TKEY,
        algorithm=alg,
        inception=now - 3600,
        expiration=now + 86400,
        mode=mode,
        error=0,
        key=b"",
    )
    rrset = dns.rrset.from_rdata(qname, dns.rdatatype.TKEY, rdata)
    msg.additional.append(rrset)
    return msg


def test_tkey_cve_2026_3119(ns1):
    keyring = dns.tsigkeyring.from_text(
        {
            "test-key": "R16NojROxtxH/xbDl//ehDsHm5DjWTQ2YXV+hGC2iBY=",
        }
    )

    msg_delete = create_tkey_msg("a.example.nil.", 5)
    msg_delete.use_tsig(keyring, keyname="test-key")
    isctest.query.tcp(msg_delete, ns1.ip, attempts=1)

    msg_unsupported = create_tkey_msg("a.example.nil.", 99)
    msg_unsupported.use_tsig(keyring, keyname="test-key")
    isctest.query.tcp(msg_unsupported, ns1.ip, attempts=1)
