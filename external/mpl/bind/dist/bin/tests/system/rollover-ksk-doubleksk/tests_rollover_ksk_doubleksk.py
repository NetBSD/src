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
from isctest.util import param
from rollover.common import (
    pytestmark,
    alg,
    size,
    KSK_CONFIG,
    KSK_LIFETIME,
    KSK_LIFETIME_POLICY,
    KSK_IPUB,
    KSK_IPUBC,
    KSK_IRET,
    KSK_KEYTTLPROP,
    TIMEDELTA,
)
from rollover.setup import (
    configure_root,
    configure_tld,
    configure_ksk_doubleksk,
)

CDSS = ["CDS (SHA-256)"]
POLICY = "ksk-doubleksk"
OFFSETS = {}
OFFSETS["step1-p"] = -int(TIMEDELTA["P7D"].total_seconds())
OFFSETS["step2-p"] = -int(KSK_LIFETIME.total_seconds() - KSK_IPUBC.total_seconds())
OFFSETS["step2-s"] = 0
OFFSETS["step3-p"] = -int(KSK_LIFETIME.total_seconds())
OFFSETS["step3-s"] = -int(KSK_IPUBC.total_seconds())
OFFSETS["step4-p"] = OFFSETS["step3-p"] - int(KSK_IRET.total_seconds())
OFFSETS["step4-s"] = OFFSETS["step3-s"] - int(KSK_IRET.total_seconds())
OFFSETS["step5-p"] = OFFSETS["step4-p"] - int(KSK_KEYTTLPROP.total_seconds())
OFFSETS["step5-s"] = OFFSETS["step4-s"] - int(KSK_KEYTTLPROP.total_seconds())
OFFSETS["step6-p"] = OFFSETS["step5-p"] - int(KSK_CONFIG["purge-keys"].total_seconds())
OFFSETS["step6-s"] = OFFSETS["step5-s"] - int(KSK_CONFIG["purge-keys"].total_seconds())


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
        delegations = configure_ksk_doubleksk(tld_name)

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
def test_ksk_doubleksk_step1(tld, alg, size, ns3):
    zone = f"step1.ksk-doubleksk.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.
    # Note that the key was already generated during setup.

    step = {
        # Introduce the first key. This will immediately be active.
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step1-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step1-p']}",
        ],
        # Next key event is when the successor KSK needs to be published.
        # That is the KSK lifetime - prepublication time (minus time
        # already passed).
        "nextev": KSK_LIFETIME - KSK_IPUB - timedelta(days=7),
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_ksk_doubleksk_step2(tld, alg, size, ns3):
    zone = f"step2.ksk-doubleksk.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 1.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step2-p']}",
                f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step2-p']}",
            ],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)

        # Check logs.
        tag = keys[1].key.tag
        msg = f"keymgr-manual-mode: block KSK rollover for key {zone}/ECDSAP256SHA256/{tag} (policy {policy})"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        # Successor KSK is prepublished (and signs DNSKEY RRset).
        # KSK1 goal: omnipresent -> hidden
        # KSK2 goal: hidden -> omnipresent
        # KSK2 dnskey: hidden -> rumoured
        # KSK2 krrsig: hidden -> rumoured
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step2-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step2-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden offset:{OFFSETS['step2-s']}",
        ],
        "keyrelationships": [1, 2],
        # Next key event is when the successor KSK becomes OMNIPRESENT.
        "nextev": KSK_IPUB,
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_ksk_doubleksk_step3(tld, alg, size, ns3):
    zone = f"step3.ksk-doubleksk.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 2, but DNSKEY has become OMNIPRESENT.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step3-p']}",
                f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step3-p']}",
                f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:hidden offset:{OFFSETS['step3-s']}",
            ],
            "keyrelationships": [1, 2],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)

        # Check logs.
        tag = keys[2].key.tag
        msg = f"keymgr-manual-mode: block transition KSK {zone}/ECDSAP256SHA256/{tag} type DS state HIDDEN to state RUMOURED"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

        # Check logs.
        tag = keys[1].key.tag
        msg = f"keymgr-manual-mode: block transition KSK {zone}/ECDSAP256SHA256/{tag} type DS state OMNIPRESENT to state UNRETENTIVE"
        if msg in ns3.log:
            # Force step.
            isctest.log.debug(
                f"keymgr-manual-mode blocking transition KSK {zone}/ECDSAP256SHA256/{tag} type DS state OMNIPRESENT to state UNRETENTIVE, step again"
            )
            with ns3.watch_log_from_here() as watcher:
                ns3.rndc(f"dnssec -step {zone}")
                watcher.wait_for_line(
                    f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
                )

    step = {
        # The successor DNSKEY RRset has become omnipresent.  The
        # predecessor DS  can be withdrawn and the successor DS can be
        # introduced.
        # KSK1 ds: omnipresent -> unretentive
        # KSK2 dnskey: rumoured -> omnipresent
        # KSK2 krrsig: rumoured -> omnipresent
        # KSK2 ds: hidden -> rumoured
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step3-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{OFFSETS['step3-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{OFFSETS['step3-s']}",
        ],
        "keyrelationships": [1, 2],
        # Next key event is when the predecessor DS has been replaced with
        # the successor DS and enough time has passed such that the all
        # validators that have this DS RRset cached only know about the
        # successor DS.  This is the the retire interval.
        "nextev": KSK_IRET,
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_ksk_doubleksk_step4(tld, alg, size, ns3):
    zone = f"step4.ksk-doubleksk.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 3, but DS has become HIDDEN/OMNIPRESENT.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step4-p']}",
                f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:hidden offset:{OFFSETS['step4-p']}",
                f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step4-s']}",
            ],
            "keyrelationships": [1, 2],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)

        # Check logs.
        tag = keys[1].key.tag
        msg1 = f"keymgr-manual-mode: block transition KSK {zone}/ECDSAP256SHA256/{tag} type DNSKEY state OMNIPRESENT to state UNRETENTIVE"
        msg2 = f"keymgr-manual-mode: block transition KSK {zone}/ECDSAP256SHA256/{tag} type KRRSIG state OMNIPRESENT to state UNRETENTIVE"
        assert msg1 in ns3.log
        assert msg2 in ns3.log

        # Force step.
        tag = keys[2].key.tag
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        # The predecessor DNSKEY may be removed, the successor DS is
        # omnipresent.
        # KSK1 dnskey: omnipresent -> unretentive
        # KSK1 krrsig: omnipresent -> unretentive
        # KSK1 ds: unretentive -> hidden
        # KSK2 ds: rumoured -> omnipresent
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step4-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:unretentive krrsig:unretentive ds:hidden offset:{OFFSETS['step4-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step4-s']}",
        ],
        "keyrelationships": [1, 2],
        # Next key event is when the DNSKEY enters the HIDDEN state.
        # This is the DNSKEY TTL plus zone propagation delay.
        "nextev": KSK_KEYTTLPROP,
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_ksk_doubleksk_step5(tld, alg, size, ns3):
    zone = f"step5.ksk-doubleksk.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        # The predecessor DNSKEY is long enough removed from the zone it
        # has become hidden.
        # KSK1 dnskey: unretentive -> hidden
        # KSK1 krrsig: unretentive -> hidden
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step5-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:hidden krrsig:hidden ds:hidden offset:{OFFSETS['step5-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step5-s']}",
        ],
        "keyrelationships": [1, 2],
        # Next key event is when the new successor needs to be published.
        # This is the KSK lifetime minus Ipub minus Iret minus time elapsed.
        "nextev": KSK_LIFETIME - KSK_IPUB - KSK_IRET - KSK_KEYTTLPROP,
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_ksk_doubleksk_step6(tld, alg, size, ns3):
    zone = f"step6.ksk-doubleksk.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        # Predecessor KSK is now purged.
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step6-p']}",
            f"ksk {KSK_LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step6-s']}",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, KSK_CONFIG, policy, step)
