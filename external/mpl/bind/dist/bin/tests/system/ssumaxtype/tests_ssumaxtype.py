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
Regression test for GL#5799: counter desynchronization in update-policy
max-records-per-type enforcement.

The prescan and main update loops used the same counter to index the
maxbytype[] array, but the main loop had continue paths that skipped the
increment, causing subsequent records to be checked against the wrong
quota values.

An attacker could craft an UPDATE message with CNAME-conflict padding
records (which are silently skipped in the main loop) to shift the
counter and bypass the per-type quota.
"""

import dns.exception
import dns.name
import dns.rcode
import dns.rdatatype
import dns.tsig
import dns.tsigkeyring
import dns.update
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/*.db",
        "*/*.db.jnl",
    ]
)

KEYRING = dns.tsigkeyring.from_text({"ddns-key.": "c2VjcmV0"})
KEYNAME = dns.name.from_text("ddns-key.")
KEYALGO = dns.tsig.HMAC_SHA256


def make_update():
    """Create a TSIG-signed UpdateMessage for the example zone."""
    return dns.update.UpdateMessage(
        "example.",
        keyring=KEYRING,
        keyname=KEYNAME,
        keyalgorithm=KEYALGO,
    )


def count_txt(ns1, name):
    """Query for TXT records at name and return the count."""
    msg = isctest.query.create(name, "TXT")
    try:
        res = isctest.query.udp(msg, ns1.ip, port=ns1.ports.dns, attempts=3)
    except dns.exception.Timeout:
        return 0
    for rrset in res.answer:
        if rrset.rdtype == dns.rdatatype.TXT:
            return len(rrset)
    return 0


def test_ssu_max_basic(ns1):
    """Verify that update-policy max limit is enforced for normal updates."""
    # Add 4 TXT records; policy allows max 3
    up = make_update()
    up.add("basic.example.", 300, "TXT", "record1")
    up.add("basic.example.", 300, "TXT", "record2")
    up.add("basic.example.", 300, "TXT", "record3")
    up.add("basic.example.", 300, "TXT", "record4")
    ns1.nsupdate(up)

    assert count_txt(ns1, "basic.example.") == 3


def test_ssu_max_across_updates(ns1):
    """Quota is enforced across multiple UPDATE messages."""
    # Fill up to the limit
    up = make_update()
    up.add("multi.example.", 300, "TXT", "first")
    up.add("multi.example.", 300, "TXT", "second")
    up.add("multi.example.", 300, "TXT", "third")
    ns1.nsupdate(up)
    assert count_txt(ns1, "multi.example.") == 3

    # Try to add one more in a separate update
    up = make_update()
    up.add("multi.example.", 300, "TXT", "fourth")
    ns1.nsupdate(up)
    assert count_txt(ns1, "multi.example.") == 3


def test_ssu_max_cname_padding_bypass(ns1):
    """CNAME-conflict padding must not shift the maxbytype counter."""
    # 4 CNAME+A padding pairs: the A records will be silently skipped
    # because a CNAME already exists at each pad name.  Without the fix,
    # this shifts the maxbytype counter by 4, causing the subsequent TXT
    # records to read unlimited (0) quota entries instead of 3.
    up = make_update()
    up.add("pad1.example.", 300, "CNAME", "x.example.")
    up.add("pad1.example.", 300, "A", "198.51.100.1")
    up.add("pad2.example.", 300, "CNAME", "x.example.")
    up.add("pad2.example.", 300, "A", "198.51.100.2")
    up.add("pad3.example.", 300, "CNAME", "x.example.")
    up.add("pad3.example.", 300, "A", "198.51.100.3")
    up.add("pad4.example.", 300, "CNAME", "x.example.")
    up.add("pad4.example.", 300, "A", "198.51.100.4")
    up.add("target.example.", 300, "TXT", "data1")
    up.add("target.example.", 300, "TXT", "data2")
    up.add("target.example.", 300, "TXT", "data3")
    up.add("target.example.", 300, "TXT", "data4")
    ns1.nsupdate(up)

    # With the fix: only 3 TXT records are added (4th rejected by quota)
    # Without the fix: all 4 are added (quota bypassed via counter shift)
    assert count_txt(ns1, "target.example.") == 3


def count_a(ns1, name):
    """Query for A records at name and return the count."""
    msg = isctest.query.create(name, "A")
    try:
        res = isctest.query.udp(msg, ns1.ip, port=ns1.ports.dns, attempts=3)
    except dns.exception.Timeout:
        return 0
    for rrset in res.answer:
        if rrset.rdtype == dns.rdatatype.A:
            return len(rrset)
    return 0


def test_max_records_per_type(ns1):
    """Zone option max-records-per-type rejects updates that exceed the limit."""
    # Add 10 A records; zone allows max 10 per type
    up = dns.update.UpdateMessage("maxrrperset.")
    for i in range(1, 11):
        up.add("a.maxrrperset.", 300, "A", f"192.0.2.{i}")
    ns1.nsupdate(up)
    assert count_a(ns1, "a.maxrrperset.") == 10

    # Adding an 11th must fail (SERVFAIL — entire update is rolled back)
    up = dns.update.UpdateMessage("maxrrperset.")
    up.add("a.maxrrperset.", 300, "A", "192.0.2.11")
    ns1.nsupdate(up, expected_rcode=dns.rcode.SERVFAIL)
    assert count_a(ns1, "a.maxrrperset.") == 10
