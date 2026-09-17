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

"""DelegationTrap characterization (issue #5347, part of the #5341 ReTrap set).

An attacker who controls a DNSSEC-signed zone can hand out a deep chain of
secure delegations so that a single query forces the resolver to build a
very long chain of trust: to validate the leaf answer it must fetch and
validate a DNSKEY and a DS at every label.  This is an algorithmic-
complexity attack -- a low query rate turns into a large amount of
signature-verification work.

In the default configuration this is already bounded by the resolver's
layered work limits, with no extra validation quota required:

  * max-recursion-queries (default 50) caps the whole fetch tree.  Every
    label forces a DNSKEY *and* a DS fetch -- distinct names the validator
    must resolve -- so the chain exhausts the recursion-query budget after
    ~25 labels and the deep query is terminated with SERVFAIL.
  * max-validations-per-fetch (default 16) independently caps the crypto
    within any single RRset's validation (the KeyTrap unit).

This test pins that default-config behaviour: a shallow name still validates
as secure, and a deep chain is stopped by the recursion-query budget
(SERVFAIL, logged as "exceeded max queries ... max-recursion-queries").

Zone hierarchy used by this module:
  .                          (ns1)  signed
  delegationtrap.            (ns2)  signed, secure delegation to:
    attack.delegationtrap.   (ans4) custom auth; every label below the
                                    apex is served as its own secure zone
                                    (see ans4/delegationtrap_ans.py)
  ns9: recursive validating resolver, default work limits
"""

from dnssec_py.common import DNSSEC_PY_MARK
from isctest.algorithms import ECDSAP256SHA256
from isctest.template import NS2, Nameserver, zones
from isctest.zone import PythonZoneKey, Zone, configure_root

import isctest
import isctest.check
import isctest.query

pytestmark = DNSSEC_PY_MARK

ANS4 = Nameserver("ans4")

MAX_VALIDATIONS = 16

# A depth whose chain of trust needs more DNSKEY/DS fetches than the default
# max-recursion-queries budget allows, so the recursion-query limit stops it.
DEEP = ".".join(["w"] * 32) + ".attack.delegationtrap"


def bootstrap():
    """Set up the delegationtrap hierarchy and the attacker key material.

    The attacker zone attack.delegationtrap. is served dynamically by ans4;
    only its key needs to exist on disk.  It is written both as a PEM (read
    by the custom server) and, via a PythonZoneKey attached to the Zone, as
    the DS that delegationtrap. (signed by dnssec-signzone) delegates to.
    """
    attack = Zone("attack.delegationtrap", ANS4, signed=False)
    attack_key = PythonZoneKey.generate(attack, alg=ECDSAP256SHA256)
    attack_key.write_private_key_pem("ans4/attack_delegationtrap.pem")
    attack.keys = [attack_key]

    parent = Zone("delegationtrap", NS2, signed=True)
    parent.delegations = [attack]
    parent.configure()

    root = configure_root([parent])

    return {
        "trust_anchors": root.trust_anchors(),
        "zones": zones([root, parent]),
        "max_validations_per_fetch": MAX_VALIDATIONS,
    }


def test_delegationtrap_shallow_secure(ns9):
    """A shallow name under the attacker zone still validates as secure.

    Its chain of trust is short (well under the query budget), so the
    resolver answers NOERROR with the AD bit set.  This guards against the
    budget breaking ordinary validation.
    """
    msg = isctest.query.create("leaf.attack.delegationtrap", "A")
    res = isctest.query.udp(msg, ns9.ip)
    isctest.check.noerror(res)
    isctest.check.adflag(res)


def test_delegationtrap_bounded(ns9):
    """A deep delegation chain is bounded by max-recursion-queries.

    Building the chain of trust for the 32-label name needs a DNSKEY and a DS
    fetch at every label -- more outbound queries than the default
    max-recursion-queries (50) budget allows.  The resolver therefore exhausts
    that budget mid-chain and terminates the query: SERVFAIL, logged as
    "exceeded max queries ... max-recursion-queries".  This is the
    default-config bound that makes the attack a non-issue with no extra
    validation quota.
    """
    msg = isctest.query.create(DEEP, "A")
    with ns9.watch_log_from_here() as watcher:
        res = isctest.query.udp(msg, ns9.ip)
        watcher.wait_for_line("exceeded max queries")
    isctest.check.servfail(res)
