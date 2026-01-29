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
    configure_zsk_prepub,
)

CONFIG = {
    "dnskey-ttl": TIMEDELTA["PT1H"],
    "ds-ttl": TIMEDELTA["P1D"],
    "max-zone-ttl": TIMEDELTA["P1D"],
    "parent-propagation-delay": TIMEDELTA["PT1H"],
    "publish-safety": TIMEDELTA["P1D"],
    "purge-keys": TIMEDELTA["PT1H"],
    "retire-safety": TIMEDELTA["P2D"],
    "signatures-refresh": TIMEDELTA["P7D"],
    "signatures-validity": TIMEDELTA["P14D"],
    "zone-propagation-delay": TIMEDELTA["PT1H"],
}
POLICY = "zsk-prepub"
ZSK_LIFETIME = TIMEDELTA["P30D"]
LIFETIME_POLICY = int(ZSK_LIFETIME.total_seconds())
IPUB = Ipub(CONFIG)
IRET = Iret(CONFIG)
KEYTTLPROP = CONFIG["dnskey-ttl"] + CONFIG["zone-propagation-delay"]
OFFSETS = {}
OFFSETS["step1-p"] = -int(TIMEDELTA["P7D"].total_seconds())
OFFSETS["step2-p"] = -int(ZSK_LIFETIME.total_seconds() - IPUB.total_seconds())
OFFSETS["step2-s"] = 0
OFFSETS["step3-p"] = -int(ZSK_LIFETIME.total_seconds())
OFFSETS["step3-s"] = -int(IPUB.total_seconds())
OFFSETS["step4-p"] = OFFSETS["step3-p"] - int(IRET.total_seconds())
OFFSETS["step4-s"] = OFFSETS["step3-s"] - int(IRET.total_seconds())
OFFSETS["step5-p"] = OFFSETS["step4-p"] - int(KEYTTLPROP.total_seconds())
OFFSETS["step5-s"] = OFFSETS["step4-s"] - int(KEYTTLPROP.total_seconds())
OFFSETS["step6-p"] = OFFSETS["step5-p"] - int(CONFIG["purge-keys"].total_seconds())
OFFSETS["step6-s"] = OFFSETS["step5-s"] - int(CONFIG["purge-keys"].total_seconds())


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
        delegations = configure_zsk_prepub(tld_name)

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
def test_zsk_prepub_step1(tld, alg, size, ns3):
    zone = f"step1.zsk-prepub.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.
    # Note that the key was already generated during setup.

    step = {
        # Introduce the first key. This will immediately be active.
        "zone": zone,
        "keyprops": [
            f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step1-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step1-p']}",
        ],
        # Next key event is when the successor ZSK needs to be published.
        # That is the ZSK lifetime - prepublication time (minus time
        # already passed).
        "nextev": ZSK_LIFETIME - IPUB - timedelta(days=7),
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_zsk_prepub_step2(tld, alg, size, ns3):
    zone = f"step2.zsk-prepub.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 1.
        step = {
            "zone": zone,
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step2-p']}",
                f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step2-p']}",
            ],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[1].key.tag
        msg = f"keymgr-manual-mode: block ZSK rollover for key {zone}/ECDSAP256SHA256/{tag} (policy {policy})"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        # it is time to pre-publish the successor zsk.
        # zsk1 goal: omnipresent -> hidden
        # zsk2 goal: hidden -> omnipresent
        # zsk2 dnskey: hidden -> rumoured
        "zone": zone,
        "keyprops": [
            f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step2-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step2-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden offset:{OFFSETS['step2-s']}",
        ],
        "keyrelationships": [1, 2],
        # next key event is when the successor zsk becomes omnipresent.
        # that is the dnskey ttl plus the zone propagation delay
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
def test_zsk_prepub_step3(tld, alg, size, ns3):
    zone = f"step3.zsk-prepub.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 2, but DNSKEY has become OMNIPRESENT.
        step = {
            "zone": zone,
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step3-p']}",
                f"zsk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step3-p']}",
                f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:hidden offset:{OFFSETS['step3-s']}",
            ],
            "keyrelationships": [1, 2],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[2].key.tag
        msg = f"keymgr-manual-mode: block transition ZSK {zone}/ECDSAP256SHA256/{tag} type ZRRSIG state HIDDEN to state RUMOURED"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

        # Check logs.
        tag = keys[1].key.tag
        msg = f"keymgr-manual-mode: block transition ZSK {zone}/ECDSAP256SHA256/{tag} type ZRRSIG state OMNIPRESENT to state UNRETENTIVE"
        if msg in ns3.log:
            # Force step.
            isctest.log.debug(
                f"keymgr-manual-mode blocking transition ZSK {zone}/ECDSAP256SHA256/{tag} type ZRRSIG state OMNIPRESENT to state UNRETENTIVE, step again"
            )
            with ns3.watch_log_from_here() as watcher:
                ns3.rndc(f"dnssec -step {zone}")
                watcher.wait_for_line(
                    f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
                )

    step = {
        # predecessor zsk is no longer actively signing. successor zsk is
        # now actively signing.
        # zsk1 zrrsig: omnipresent -> unretentive
        # zsk2 dnskey: rumoured -> omnipresent
        # zsk2 zrrsig: hidden -> rumoured
        "zone": zone,
        "keyprops": [
            f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step3-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:unretentive offset:{OFFSETS['step3-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:rumoured offset:{OFFSETS['step3-s']}",
        ],
        "keyrelationships": [1, 2],
        # next key event is when all the rrsig records have been replaced
        # with signatures of the new zsk, in other words when zrrsig
        # becomes omnipresent.
        "nextev": IRET,
        # set 'smooth' to true so expected signatures of subdomain are
        # from the predecessor zsk.
        "smooth": True,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

    # Force full resign and check all signatures have been replaced.
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"sign {zone}")
        watcher.wait_for_line(f"zone {zone}/IN (signed): sending notifies")

    step["smooth"] = False
    step["nextev"] = Iret(CONFIG, smooth=False)
    isctest.kasp.check_rollover_step(ns3, CONFIG, POLICY, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_zsk_prepub_step4(tld, alg, size, ns3):
    zone = f"step4.zsk-prepub.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 3, but zone signatures have become HIDDEN/OMNIPRESENT.
        step = {
            "zone": zone,
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step4-p']}",
                f"zsk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:hidden offset:{OFFSETS['step4-p']}",
                f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step4-s']}",
            ],
            "keyrelationships": [1, 2],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[1].key.tag
        msg = f"keymgr-manual-mode: block transition ZSK {zone}/ECDSAP256SHA256/{tag} type DNSKEY state OMNIPRESENT to state UNRETENTIVE"
        assert msg in ns3.log

        # Force step.
        tag = keys[2].key.tag
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        # predecessor zsk is no longer needed. all rrsets are signed with
        # the successor zsk.
        # zsk1 dnskey: omnipresent -> unretentive
        # zsk1 zrrsig: unretentive -> hidden
        # zsk2 zrrsig: rumoured -> omnipresent
        "zone": zone,
        "keyprops": [
            f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step4-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:unretentive zrrsig:hidden offset:{OFFSETS['step4-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step4-s']}",
        ],
        "keyrelationships": [1, 2],
        # next key event is when the dnskey enters the hidden state.
        # this is the dnskey ttl plus zone propagation delay.
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
def test_zsk_prepub_step5(tld, alg, size, ns3):
    zone = f"step5.zsk-prepub.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        # predecessor zsk is now removed.
        # zsk1 dnskey: unretentive -> hidden
        "zone": zone,
        "keyprops": [
            f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step5-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:hidden dnskey:hidden zrrsig:hidden offset:{OFFSETS['step5-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step5-s']}",
        ],
        "keyrelationships": [1, 2],
        # next key event is when the new successor needs to be published.
        # this is the zsk lifetime minus IRET minus IPUB minus time
        # elapsed.
        "nextev": ZSK_LIFETIME - IRET - IPUB - KEYTTLPROP,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_zsk_prepub_step6(tld, alg, size, ns3):
    zone = f"step6.zsk-prepub.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        # predecessor zsk is now purged.
        "zone": zone,
        "keyprops": [
            f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{OFFSETS['step6-p']}",
            f"zsk {LIFETIME_POLICY} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{OFFSETS['step6-s']}",
        ],
        "nextev": None,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)
