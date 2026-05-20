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

"""Test that NSEC records received via IXFR produce correct denial-of-existence
proofs for empty non-terminal names.

When a secondary receives NSEC records via IXFR (transitioning from an unsigned
zone to an NSEC-signed zone), queries for empty non-terminal names should return
the NSEC record that covers the ENT, not the zone apex NSEC."""

import shutil

import dns.name
import dns.rdatatype

import isctest

ORIGIN = dns.name.from_text("example.")
QNAME = dns.name.from_text("wildcard.example.")


def test_nsec_ixfr_empty_nonterminal(ns1, ns2):
    """Verify correct NSEC proof for ENT after IXFR from unsigned to signed.

    1. Wait for ns2 to have the unsigned zone (serial 1) via AXFR.
    2. Switch ns1 to the signed zone (serial 2), reload.
    3. Wait for ns2 to pick up serial 2 (via IXFR).
    4. Query ns2 for wildcard.example. A +dnssec.
    5. Verify the AUTHORITY section contains the correct covering NSEC.
    """

    # Step 1: Wait for initial unsigned zone transfer to complete.
    isctest.query.wait_for_serial(ns2.ip, "example", 1)

    # Step 2: Replace the zone on ns1 with the signed version and reload.
    shutil.copy("ns1/example.db.signed", "ns1/example.db")
    ns1.rndc("reload")

    # Step 3: Wait for ns2 to get the signed zone via IXFR.
    isctest.query.wait_for_serial(ns2.ip, "example", 2)

    # Step 4: Query ns2 for the empty non-terminal with DNSSEC.
    msg = isctest.query.create(QNAME, "A", dnssec=True)
    res = isctest.query.tcp(msg, ns2.ip)

    # The ENT wildcard.example. has no A record, so this should be NOERROR
    # with an empty answer (the wildcard *.wildcard.example. does not match
    # wildcard.example. itself).
    isctest.check.noerror(res)
    assert len(res.answer) == 0, f"expected empty answer for ENT, got: {res.answer}"

    # Step 5: Verify the NSEC record covers the ENT, not the apex.
    nsec_rrsets = [
        rrset for rrset in res.authority if rrset.rdtype == dns.rdatatype.NSEC
    ]
    assert (
        len(nsec_rrsets) > 0
    ), f"no NSEC records in authority section: {res.authority}"

    # The bug (f4b4f030) returns the apex NSEC instead of the correct
    # covering NSEC, because the node's havensec flag was not set
    # during IXFR.
    for rrset in nsec_rrsets:
        assert rrset.name != ORIGIN, (
            f"got apex NSEC '{rrset.name} -> {rrset[0].next}' instead "
            f"of the covering NSEC for {QNAME}"
        )

    # Verify the returned NSEC actually covers the ENT: the NSEC owner
    # must be canonically before the ENT, and the NSEC next name must be
    # canonically after (or wrap around to the apex).
    found_covering = False
    for rrset in nsec_rrsets:
        nsec_next = rrset[0].next
        if rrset.name < QNAME and (nsec_next > QNAME or nsec_next <= rrset.name):
            found_covering = True

    assert (
        found_covering
    ), f"no NSEC covers {QNAME}; " f"NSEC records found: " + ", ".join(
        f"'{rrset.name} -> {rrset[0].next}'" for rrset in nsec_rrsets
    )
