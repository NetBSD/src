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

"""Tests for RRSIG Labels vs signer name label count validation.

Zone hierarchy used by this module:
  .                              (ns1)  signed
  rrsig-labels-signer.           (ns2)  signed, NSEC
    attacker.rrsig-labels-signer. (ans4) custom auth (crafted responses)
    unsigned.rrsig-labels-signer. (ns2)  insecure delegation (NSEC seed)
  ns9: recursive resolver, synth-from-dnssec defaults to yes
"""

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.template import NS2, Nameserver, zones
from isctest.zone import PythonZoneKey, Zone, configure_root

import isctest
import isctest.check
import isctest.query

pytestmark = DNSSEC_PY_MARK

ANS4 = Nameserver("ans4")


def bootstrap():
    """Set up the four-zone hierarchy and generate attacker key material.

    Writes attacker_rrsig_labels_signer.pem to ans4/ so the custom server can read it
    at startup.  Attaches a PythonZoneKey to the attacker Zone so that
    parent.configure() derives the DS and passes it to dnssec-signzone.
    """
    attacker = Zone("attacker.rrsig-labels-signer", ANS4, signed=False)
    attacker_key = PythonZoneKey.generate(attacker)
    attacker_key.write_private_key_pem("ans4/attacker_rrsig_labels_signer.pem")
    attacker.keys = [attacker_key]

    unsigned = Zone("unsigned.rrsig-labels-signer", NS2, signed=False)
    unsigned.render()

    parent = Zone("rrsig-labels-signer", NS2, signed=True)
    parent.delegations = [attacker, unsigned]
    parent.configure()

    root = configure_root([parent])

    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, parent, unsigned]),
    }


def test_rrsig_labels_underflow_rejected(ns9):
    """Fixed BIND rejects RRSIG where Labels < signer_labels - 1.

    Q1: www.attacker.rrsig-labels-signer./A DO
    The attacker returns A 192.0.2.1 + RRSIG(Labels=1, signer=attacker.rrsig-labels-signer.)
    The fix returns DNS_R_SIGINVALID → resolver returns SERVFAIL.
    Without the fix, the resolver returns NOERROR+AD (accepting the forgery).

    This test FAILS on unfixed BIND (no DNS_R_SIGINVALID check).
    """
    msg = isctest.query.create("www.attacker.rrsig-labels-signer", "A")
    res = isctest.query.udp(msg, ns9.ip)
    isctest.check.servfail(res)


def test_rrsig_labels_no_wildcard_cache_poison(ns9):
    """Labels underflow must not enable synth-from-dnssec cache poisoning.

    The labels-underflow bug lets an attacker who controls
    attacker.rrsig-labels-signer. cache a wildcard *.rrsig-labels-signer.
    (one label *above* its own zone) as SECURE.  RFC 8198 synth-from-dnssec
    will then synthesise that forged wildcard onto any non-existent name in
    the parent zone — but only if the resolver can prove the victim name's
    non-existence *from cache* (otherwise it queries the authority and gets
    an honest NXDOMAIN whose apex NSEC also proves the wildcard absent,
    which defeats the attack).

    The three queries arrange exactly that cached state:

      Q1 poison   www.attacker.rrsig-labels-signer./A
                  attacker returns A 192.0.2.1 + RRSIG(Labels=1, signer=
                  attacker.rrsig-labels-signer.).  Unfixed BIND validates it
                  via DNS_R_FROMWILDCARD and caches *.rrsig-labels-signer. A.

      Q2 seed     seed.unsigned.rrsig-labels-signer./A
                  unsigned is an insecure delegation, so the resolver fetches
                  unsigned.rrsig-labels-signer./DS and caches the denial NSEC
                  "unsigned NSEC <apex>".  "unsigned" sorts last among the
                  parent's names, so that NSEC covers the whole gap up to the
                  apex (including www) — yet, unlike a real NXDOMAIN, it does
                  not carry the apex NSEC proving "*.rrsig-labels-signer." is
                  absent.  The poison wildcard is therefore never contradicted.

      victim      www.rrsig-labels-signer./A
                  unfixed BIND has a covering NSEC and the poison wildcard in
                  cache, so synth-from-dnssec answers NOERROR 192.0.2.1.
                  Fixed BIND never cached the wildcard, so www is NXDOMAIN.

    This test FAILS on unfixed BIND (the victim is answered with 192.0.2.1).
    """
    isctest.query.udp(
        isctest.query.create("www.attacker.rrsig-labels-signer", "A"), ns9.ip
    )
    isctest.query.udp(
        isctest.query.create("seed.unsigned.rrsig-labels-signer", "A"), ns9.ip
    )

    res = isctest.query.udp(
        isctest.query.create("www.rrsig-labels-signer", "A"), ns9.ip
    )
    assert not any(
        "192.0.2.1" in str(rr) for rr in res.answer
    ), f"cache poisoned: synth-from-dnssec served the forged wildcard:\n{res}"
    isctest.check.nxdomain(res)
