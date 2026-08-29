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

from pathlib import Path
from re import compile as Re

import time

import dns.name
import dns.rdataclass
import dns.rdatatype
import dns.zone
import pytest

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.template import NS2, zones
from isctest.zone import Zone, configure_root

import isctest

pytestmark = DNSSEC_PY_MARK


def bootstrap():
    zone = Zone("rrsigs-extra-expired", NS2, signed=True)
    zone.add_keys()
    zone.render()

    signed_path = Path(zone.ns.name) / zone.filepath_signed

    # create valid but expired signatures
    expired_rdata = set()
    now = int(time.time())
    start = now - 20000
    end = now - 10000
    for i in range(2):
        zone.sign(f"-s {start - i} -e {end - i}")
        expired = dns.zone.from_file(str(signed_path), origin="rrsigs-extra-expired.")
        rdataset = expired.get_rdataset("a", "RRSIG", "A")
        expired_rdata.add(rdataset.pop())

    # sign zone with valid sigs
    zone.sign()
    valid = dns.zone.from_file(str(signed_path), origin="rrsigs-extra-expired.")
    rdataset = valid.find_rdataset("a", "RRSIG", "A")

    # add the expired RRSIGs for a.rrsigs-extra-expired
    for rd in expired_rdata:
        rdataset.add(rd)
    valid.to_file(str(signed_path))

    root = configure_root([zone])
    return {
        "max_validations_per_fetch": 2,
        "rrset_order_none": [zone.name],
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, zone]),
    }


@pytest.fixture(scope="module", autouse=True)
def after_servers_start(ns2):
    msg = isctest.query.create("a.rrsigs-extra-expired", "A")

    # Check the order of returned RRSIGs from auth. Due to rrset-order none;
    # this should remain constant for the remainder of the test.
    # Ensure the first two RRSIGs are expired, otherwise skip the test.
    res = isctest.query.tcp(msg, ns2.ip)
    rrsigs = res.get_rrset(
        res.answer,
        dns.name.from_text("a.rrsigs-extra-expired."),
        dns.rdataclass.IN,
        dns.rdatatype.RRSIG,
        dns.rdatatype.A,
    )
    now = time.time()
    assert len(rrsigs) > 2
    if rrsigs[0].expiration >= now or rrsigs[1].expiration >= now:
        pytest.skip("valid RRSIG listed first in response, re-run test")


def test_regular_query(ns9):
    # sanity check - record with no extra sigs gets NOERROR
    msg = isctest.query.create("b.rrsigs-extra-expired", "A")
    res = isctest.query.tcp(msg, ns9.ip)
    isctest.check.noerror(res)


def test_extra_expired_rrsigs(ns9):
    msg = isctest.query.create("a.rrsigs-extra-expired", "A")
    with ns9.watch_log_from_here() as watcher:
        res = isctest.query.tcp(msg, ns9.ip)
        watcher.wait_for_sequence(
            [
                Re(r"a.rrsigs-extra-expired/A: verify failed.* RRSIG has expired"),
                Re(r"a.rrsigs-extra-expired/A: maximum number of validations exceeded"),
            ]
        )
    isctest.check.servfail(res)
