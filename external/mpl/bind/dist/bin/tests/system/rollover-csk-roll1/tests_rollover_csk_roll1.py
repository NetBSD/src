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

# pylint: disable=redefined-outer-name,unused-import

from datetime import timedelta

import pytest

import isctest
from isctest.kasp import Ipub, Iret
from isctest.util import param
from rollover.common import (
    pytestmark,
    alg,
    size,
    TIMEDELTA,
)
from rollover.setup import (
    configure_root,
    configure_tld,
    configure_cskroll1,
)

CDSS = ["CDNSKEY", "CDS (SHA-384)"]
CONFIG = {
    "dnskey-ttl": TIMEDELTA["PT1H"],
    "ds-ttl": TIMEDELTA["PT1H"],
    "max-zone-ttl": TIMEDELTA["P1D"],
    "parent-propagation-delay": TIMEDELTA["PT1H"],
    "publish-safety": TIMEDELTA["PT1H"],
    "purge-keys": TIMEDELTA["PT1H"],
    "retire-safety": TIMEDELTA["PT2H"],
    "signatures-refresh": TIMEDELTA["P5D"],
    "signatures-validity": TIMEDELTA["P30D"],
    "zone-propagation-delay": TIMEDELTA["PT1H"],
}
POLICY = "csk-roll1"
CSK_LIFETIME = timedelta(days=31 * 6)
LIFETIME_POLICY = int(CSK_LIFETIME.total_seconds())
IPUB = Ipub(CONFIG)
IRETZSK = Iret(CONFIG)
IRETKSK = Iret(CONFIG, zsk=False, ksk=True)
KEYTTLPROP = CONFIG["dnskey-ttl"] + CONFIG["zone-propagation-delay"]
SIGNDELAY = IRETZSK - IRETKSK - KEYTTLPROP
OFFSETS = {}
OFFSETS["step1-p"] = -int(timedelta(days=7).total_seconds())
OFFSETS["step2-p"] = -int(CSK_LIFETIME.total_seconds() - IPUB.total_seconds())
OFFSETS["step2-s"] = 0
OFFSETS["step3-p"] = -int(CSK_LIFETIME.total_seconds())
OFFSETS["step3-s"] = -int(IPUB.total_seconds())
OFFSETS["step4-p"] = OFFSETS["step3-p"] - int(IRETKSK.total_seconds())
OFFSETS["step4-s"] = OFFSETS["step3-s"] - int(IRETKSK.total_seconds())
OFFSETS["step5-p"] = OFFSETS["step4-p"] - int(KEYTTLPROP.total_seconds())
OFFSETS["step5-s"] = OFFSETS["step4-s"] - int(KEYTTLPROP.total_seconds())
OFFSETS["step6-p"] = OFFSETS["step5-p"] - int(SIGNDELAY.total_seconds())
OFFSETS["step6-s"] = OFFSETS["step5-s"] - int(SIGNDELAY.total_seconds())
OFFSETS["step7-p"] = OFFSETS["step6-p"] - int(KEYTTLPROP.total_seconds())
OFFSETS["step7-s"] = OFFSETS["step6-s"] - int(KEYTTLPROP.total_seconds())
OFFSETS["step8-p"] = OFFSETS["step7-p"] - int(CONFIG["purge-keys"].total_seconds())
OFFSETS["step8-s"] = OFFSETS["step7-s"] - int(CONFIG["purge-keys"].total_seconds())


def bootstrap():
    data = {
        "tlds": [],
        "trust_anchors": [],
    }

    tlds = []
    for tld_name in [
        "autosign",
        "manual",
    ]:
        delegations = configure_cskroll1(tld_name, f"{POLICY}-{tld_name}")

        tld = configure_tld(tld_name, delegations)
        tlds.append(tld)

        data["tlds"].append(tld_name)

    ta = configure_root(tlds)
    data["trust_anchors"].append(ta)

    return data


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step1(tld, ns3, alg, size):
    zone = f"step1.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.
    # Note that the key was already generated during setup.

    step = {
        # Introduce the first key. This will immediately be active.
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step1-p']}",
        ],
        # Next key event is when the successor CSK needs to be published
        # minus time already elapsed. This is Lcsk - Ipub + Dreg (we ignore
        # registration delay).
        "nextev": CSK_LIFETIME - IPUB - timedelta(days=7),
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step2(tld, alg, size, ns3):
    zone = f"step2.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 1.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step2-p']}",
            ],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[0].key.tag
        msg = f"keymgr-manual-mode: block CSK rollover for key {zone}/ECDSAP256SHA256/{tag} (policy {policy})"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        # Successor CSK is prepublished (signs DNSKEY RRset, but not yet
        # other RRsets).
        # CSK1 goal: omnipresent -> hidden
        # CSK2 goal: hidden -> omnipresent
        # CSK2 dnskey: hidden -> rumoured
        # CSK2 krrsig: hidden -> rumoured
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step2-p']}",
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:hidden ds:hidden offset:{OFFSETS['step2-s']}",
        ],
        "keyrelationships": [0, 1],
        # Next key event is when the successor CSK becomes OMNIPRESENT.
        "nextev": IPUB,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step3(tld, alg, size, ns3):
    zone = f"step3.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 2, but DNSKEY has become OMNIPRESENT.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step3-p']}",
                f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:hidden ds:hidden offset:{OFFSETS['step3-s']}",
            ],
            "keyrelationships": [0, 1],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[1].key.tag
        msg = f"keymgr-manual-mode: block transition CSK {zone}/ECDSAP256SHA256/{tag} type ZRRSIG state HIDDEN to state RUMOURED"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

        # Check logs.
        tag = keys[0].key.tag
        msg = f"keymgr-manual-mode: block transition CSK {zone}/ECDSAP256SHA256/{tag} type ZRRSIG state OMNIPRESENT to state UNRETENTIVE"
        if msg in ns3.log:
            # Force step.
            isctest.log.debug(
                f"keymgr-manual-mode blocking transition CSK {zone}/ECDSAP256SHA256/{tag} type ZRRSIG state OMNIPRESENT to state UNRETENTIVE, step again"
            )
            with ns3.watch_log_from_here() as watcher:
                ns3.rndc(f"dnssec -step {zone}")
                watcher.wait_for_line(
                    f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
                )

    step = {
        # Successor CSK becomes omnipresent, meaning we can start signing
        # the remainder of the zone with the successor CSK, and we can
        # submit the DS.
        "zone": zone,
        "cdss": CDSS,
        # Predecessor CSK will be removed, so moving to UNRETENTIVE.
        # CSK1 zrrsig: omnipresent -> unretentive
        # Successor CSK DNSKEY is OMNIPRESENT, so moving ZRRSIG to RUMOURED.
        # CSK2 dnskey: rumoured -> omnipresent
        # CSK2 krrsig: rumoured -> omnipresent
        # CSK2 zrrsig: hidden -> rumoured
        # The predecessor DS can be withdrawn and the successor DS can be
        # introduced.
        # CSK1 ds: omnipresent -> unretentive
        # CSK2 ds: hidden -> rumoured
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:unretentive ds:unretentive offset:{OFFSETS['step3-p']}",
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:rumoured offset:{OFFSETS['step3-s']}",
        ],
        "keyrelationships": [0, 1],
        # Next key event is when the predecessor DS has been replaced with
        # the successor DS and enough time has passed such that the all
        # validators that have this DS RRset cached only know about the
        # successor DS.  This is the the retire interval.
        "nextev": IRETKSK,
        # Set 'smooth' to true so expected signatures of subdomain are
        # from the predecessor ZSK.
        "smooth": True,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step4(tld, alg, size, ns3):
    zone = f"step4.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 3, but DS has become HIDDEN/OMNIPRESENT.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:unretentive ds:hidden offset:{OFFSETS['step4-p']}",
                f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:omnipresent offset:{OFFSETS['step4-s']}",
            ],
            "keyrelationships": [0, 1],
            "manual-mode": True,
            "nextev": None,
            # We already swapped the DS in the previous step, so disable ds-swap.
            "ds-swap": False,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[0].key.tag
        msg = f"keymgr-manual-mode: block transition CSK {zone}/ECDSAP256SHA256/{tag} type KRRSIG state OMNIPRESENT to state UNRETENTIVE"
        assert msg in ns3.log

        # Force step.
        tag = keys[1].key.tag
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        "zone": zone,
        "cdss": CDSS,
        # The predecessor CSK is no longer signing the DNSKEY RRset.
        # CSK1 krrsig: omnipresent -> unretentive
        # The predecessor DS is hidden. The successor DS is now omnipresent.
        # CSK1 ds: unretentive -> hidden
        # CSK2 ds: rumoured -> omnipresent
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:unretentive zrrsig:unretentive ds:hidden offset:{OFFSETS['step4-p']}",
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:omnipresent offset:{OFFSETS['step4-s']}",
        ],
        "keyrelationships": [0, 1],
        # Next key event is when the KRRSIG enters the HIDDEN state.
        # This is the DNSKEY TTL plus zone propagation delay.
        "nextev": KEYTTLPROP,
        # We already swapped the DS in the previous step, so disable ds-swap.
        "ds-swap": False,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step5(tld, alg, size, ns3):
    zone = f"step5.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        "zone": zone,
        "cdss": CDSS,
        # The predecessor KRRSIG records are now all hidden.
        # CSK1 krrsig: unretentive -> hidden
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:hidden zrrsig:unretentive ds:hidden offset:{OFFSETS['step5-p']}",
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:omnipresent offset:{OFFSETS['step5-s']}",
        ],
        "keyrelationships": [0, 1],
        # Next key event is when the DNSKEY can be removed.  This is when
        # all ZRRSIG records have been replaced with signatures of the new
        # CSK.
        "nextev": SIGNDELAY,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step6(tld, alg, size, ns3):
    zone = f"step6.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        return

    step = {
        "zone": zone,
        "cdss": CDSS,
        # The predecessor ZRRSIG records are now all hidden (so the DNSKEY
        # can be removed).
        # CSK1 dnskey: omnipresent -> unretentive
        # CSK1 zrrsig: unretentive -> hidden
        # CSK2 zrrsig: rumoured -> omnipresent
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:unretentive krrsig:hidden zrrsig:hidden ds:hidden offset:{OFFSETS['step6-p']}",
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step6-s']}",
        ],
        "keyrelationships": [0, 1],
        # Next key event is when the DNSKEY enters the HIDDEN state.
        # This is the DNSKEY TTL plus zone propagation delay.
        "nextev": KEYTTLPROP,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step7(tld, alg, size, ns3):
    zone = f"step7.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        "zone": zone,
        "cdss": CDSS,
        # The predecessor CSK is now completely HIDDEN.
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:hidden krrsig:hidden zrrsig:hidden ds:hidden offset:{OFFSETS['step7-p']}",
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step7-s']}",
        ],
        "keyrelationships": [0, 1],
        # Next key event is when the new successor needs to be published.
        # This is the Lcsk, minus time passed since the key started signing,
        # minus the prepublication time.
        "nextev": CSK_LIFETIME - IRETZSK - IPUB - KEYTTLPROP,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_csk_roll1_step8(tld, alg, size, ns3):
    zone = f"step8.csk-roll1.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"csk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step8-s']}",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)
