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
Test that the resolver rejects DS records for sibling zones in referrals.

A custom authoritative server (ans4) returns a referral for
child.sibling-ds that includes a DS record for sibling.sibling-ds.  The
resolver must detect that the DS owner does not match the delegation NS
name and treat the response as a form error.
"""

from re import compile as Re

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.template import NS2, Nameserver, zones
from isctest.zone import Zone, configure_root

import isctest

pytestmark = DNSSEC_PY_MARK

ANS4 = Nameserver("ans4")


def bootstrap():
    # Child zone on ns2 — the test queries a.child.sibling-ds which
    # resolves to the default template A record (10.0.0.1).
    child = Zone("child.sibling-ds", NS2)
    child.configure()

    # Sibling zone on ns2 — exists so the sibling DS in the referral
    # refers to a real delegation.
    sibling = Zone("sibling.sibling-ds", NS2)
    sibling.configure()

    # Parent zone rendered into ans4/ (subdir=None puts the .db file
    # directly in the ans4 directory where AsyncDnsServer loads it).
    parent = Zone("sibling-ds", ANS4, subdir=None)
    parent.delegations = [child, sibling]
    parent.configure()

    # Root zone delegates sibling-ds. to ans4.
    root = configure_root([parent])

    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, child, sibling]),
    }


def test_sibling_ds_rejected(ns9):
    """Resolver must reject a referral that contains DS for a sibling zone."""
    log_ds_mismatch = Re(r"DS doesn't match the delegation owner name")

    msg = isctest.query.create("a.child.sibling-ds.", "A")

    with ns9.watch_log_from_here() as watcher:
        res = isctest.query.tcp(msg, ns9.ip)
        watcher.wait_for_line(log_ds_mismatch)

    isctest.check.servfail(res)
