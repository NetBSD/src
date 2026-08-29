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
from isctest.template import NS2, TrustAnchor, zones
from isctest.zone import Zone, configure_root

import isctest

pytestmark = DNSSEC_PY_MARK


def bootstrap():
    revoked_zone = Zone("truncated-revoked.selfsigned", NS2, signed=True)
    active_zone = Zone("truncated-active.selfsigned", NS2, signed=True)

    # just delegation, the trust anchors are added directly
    root = configure_root([revoked_zone, active_zone], signed=False)

    # The trust anchor key tag must match the revoked truncated self-signed key
    # in the zone (key tag 33167). The flags differ here (257 vs 385) because
    # the revoked bit is not part of the trust anchor, but it is part of the key
    # tag calculation.
    revoked_ta = TrustAnchor(
        "truncated-revoked.selfsigned", "static-key", '257 3 14 "fYA="'
    )

    # The active truncated key is too short for the ECDSA curve but passes the
    # parser's minimum-length check; trusting it directly exercises the
    # key-construction failure path.
    active_ta = TrustAnchor(
        "truncated-active.selfsigned", "static-key", '257 3 14 "fYA="'
    )

    return {
        "trust_anchors": [revoked_ta, active_ta],
        "zones": zones([root, revoked_zone, active_zone]),
    }


def test_truncated_revoked_dnskey(ns9):
    msg = isctest.query.create("a.truncated-revoked.selfsigned.", "A")
    with ns9.watch_log_from_here() as watcher:
        res = isctest.query.tcp(msg, ns9.ip)
        watcher.wait_for_line(
            Re("a.truncated-revoked.selfsigned/A.*broken trust chain")
        )
    isctest.check.servfail(res)


def test_truncated_active_dnskey(ns9):
    msg = isctest.query.create("a.truncated-active.selfsigned.", "A")
    res = isctest.query.tcp(msg, ns9.ip)
    isctest.check.servfail(res)
