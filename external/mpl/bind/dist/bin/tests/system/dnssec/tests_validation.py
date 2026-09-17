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


from dns import rdatatype

import pytest

import isctest
import isctest.mark

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/K*",
        "*/NSEC*",
        "*/dsset-*",
        "*/*.bk",
        "*/*.conf",
        "*/*.db",
        "*/*.id",
        "*/*.jnl",
        "*/*.jbk",
        "*/*.key",
        "*/*.signed",
        "*/settime.out.*",
        "ans*/ans.run",
        "*/trusted.keys",
        "*/*.bad",
        "*/*.next",
        "*/*.stripped",
        "*/*.tmp",
        "*/*.stage?",
        "*/*.patched",
        "*/*.lower",
        "*/*.upper",
        "*/*.unsplit",
        "kasp.conf",
    ]
)


def test_positive_validation_dname_at_apex():
    # an apex DNAME is signed by the DNSKEY living at the DNAME owner
    # name itself; fetching that key must not be mistaken for a
    # non-advancing alias chain (GL #6176)
    msg = isctest.query.create("a.dname-at-apex-nsec3.example", "A")
    res = isctest.query.tcp(msg, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)
    answers = {(str(rr.name), rr.rdtype) for rr in res.answer}
    assert ("dname-at-apex-nsec3.example.", rdatatype.DNAME) in answers
    assert ("a.example.", rdatatype.A) in answers


def test_ds_keytag_flood(ns4):
    # GL #5349: a securely-delegated zone whose parent DS RRset is flooded
    # with many mismatched DS records (each a unique key tag) must be
    # rejected without unbounded per-DS matching work.  The per-fetch
    # validation quota caps the DS x DNSKEY matching, so the DNSKEY fetch
    # fails with ISC_R_QUOTA and the answer is SERVFAIL.  An unbounded
    # walk would grind through every DS and never hit the quota.
    with ns4.watch_log_from_here() as watcher:
        msg = isctest.query.create("keytrap.example", "SOA")
        res = isctest.query.tcp(msg, "10.53.0.4")
        isctest.check.servfail(res)
        watcher.wait_for_line("quota reached resolving 'keytrap.example/DNSKEY/IN'")


def test_ds_keytag_flood_combinations(ns4):
    # GL #5349: the parent DS RRset here is smaller than the per-fetch
    # validation quota, but the child DNSKEY RRset has several keys, so the
    # DS-by-DNSKEY combination count exceeds the up-front product cap.  The
    # DNSKEY fetch must fail terminally with the validation quota (SERVFAIL,
    # no retry).  Without the product cap the per-DS walk would finish below
    # the quota and never reject the zone; returning ISC_R_RANGE instead of
    # ISC_R_QUOTA would log "out of range" and retry rather than stop here.
    with ns4.watch_log_from_here() as watcher:
        msg = isctest.query.create("keytrap2.example", "SOA")
        res = isctest.query.tcp(msg, "10.53.0.4")
        isctest.check.servfail(res)
        watcher.wait_for_line("quota reached resolving 'keytrap2.example/DNSKEY/IN'")


def test_ds_keytag_flood_ignores_unsupported_ds():
    # GL #5349: unsupported DS algorithms and digest types do not reach
    # DNSKEY matching and therefore must not contribute to its combination
    # cap.  The raw DS-by-DNSKEY count exceeds the cap, but the delegation has
    # one supported DS and must validate successfully.
    msg = isctest.query.create("keytrap3.example", "SOA")
    res = isctest.query.tcp(msg, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)
