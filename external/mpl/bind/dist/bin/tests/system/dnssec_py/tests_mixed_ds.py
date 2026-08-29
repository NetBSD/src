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

from re import compile as Re

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.algorithms import Algorithm
from isctest.template import NS2, NS3, zones
from isctest.zone import Zone, configure_root

import isctest

pytestmark = DNSSEC_PY_MARK


def modify_dsset():
    with open("ns3/dsset-child.mixed-ds.", encoding="utf-8") as dsset_file:
        ds_orig = dsset_file.readline()

    alg = Algorithm.default().number
    alg_re = Re(rf"\s+{alg}\s+")
    ds_unsupported = alg_re.sub(" 12 ", ds_orig)

    digest_re = Re(rf"\s+{alg}\s+2\s+.*")
    ds_bogus = digest_re.sub(
        f" {alg} 2 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        ds_orig,
    )

    with open("ns3/dsset-child.mixed-ds.", "w", encoding="utf-8") as dsset_file:
        dsset_file.writelines([ds_unsupported, ds_bogus])


def bootstrap():
    child = Zone("child.mixed-ds", NS3, signed=True)
    child.configure()

    isctest.log.info(
        "child.mixed-ds: modify DS set to have unsupported and bogus DS records"
    )
    modify_dsset()

    mixed_ds = Zone("mixed-ds", NS2, signed=True)
    mixed_ds.delegations = [child]
    mixed_ds.configure()

    root = configure_root([mixed_ds])

    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, mixed_ds, child]),
    }


def test_mixed_ds(ns9):
    msg = isctest.query.create("child.mixed-ds.", "DNSKEY")
    with ns9.watch_log_from_here() as watcher:
        res = isctest.query.tcp(msg, ns9.ip)
        watcher.wait_for_line("child.mixed-ds/DNSKEY: insecurity proof failed")
    isctest.check.servfail(res)

    msg = isctest.query.create("a.child.mixed-ds.", "A")
    res = isctest.query.tcp(msg, ns9.ip)
    isctest.check.servfail(res)
