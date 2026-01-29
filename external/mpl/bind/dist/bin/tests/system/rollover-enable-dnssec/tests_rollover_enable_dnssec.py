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

import pytest

import isctest
from isctest.kasp import Ipub, IpubC, Iret
from isctest.util import param
from rollover.common import (
    pytestmark,
    alg,
    size,
    CDSS,
    TIMEDELTA,
)
from rollover.setup import (
    configure_root,
    configure_tld,
    configure_enable_dnssec,
)

CONFIG = {
    "dnskey-ttl": TIMEDELTA["PT5M"],
    "ds-ttl": TIMEDELTA["PT2H"],
    "max-zone-ttl": TIMEDELTA["PT12H"],
    "parent-propagation-delay": TIMEDELTA["PT1H"],
    "publish-safety": TIMEDELTA["PT5M"],
    "retire-safety": TIMEDELTA["PT20M"],
    "signatures-refresh": TIMEDELTA["P7D"],
    "signatures-validity": TIMEDELTA["P14D"],
    "zone-propagation-delay": TIMEDELTA["PT5M"],
}
POLICY = "enable-dnssec"
IPUB = Ipub(CONFIG)
IPUBC = IpubC(CONFIG, rollover=False)
IRETZSK = Iret(CONFIG, rollover=False)
IRETKSK = Iret(CONFIG, zsk=False, ksk=True, rollover=False)
OFFSETS = {}
OFFSETS["step1"] = 0
OFFSETS["step2"] = -int(IPUB.total_seconds())
OFFSETS["step3"] = -int(IRETZSK.total_seconds())
OFFSETS["step4"] = -int(IPUBC.total_seconds() + IRETKSK.total_seconds())


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
        delegations = configure_enable_dnssec(tld_name, f"{POLICY}-{tld_name}")

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
def test_rollover_enable_dnssec_step1(tld, alg, size, ns3):
    zone = f"step1.enable-dnssec.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as insecure.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [],
            "manual-mode": True,
            "zone-signed": False,
            "nextev": None,
        }
        isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        msg = f"keymgr-manual-mode: block new key generation for zone {zone} (policy {policy})"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(f"keymgr: {zone} done")

    step = {
        "zone": zone,
        "cdss": CDSS,
        "keyprops": [
            f"csk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden offset:{OFFSETS['step1']}",
        ],
        # Next key event is when the DNSKEY RRset becomes OMNIPRESENT,
        # after the publication interval.
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
def test_rollover_enable_dnssec_step2(tld, alg, size, ns3):
    zone = f"step2.enable-dnssec.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        "zone": zone,
        "cdss": CDSS,
        # The DNSKEY is omnipresent, but the zone signatures not yet.
        # Thus, the DS remains hidden.
        # dnskey: rumoured -> omnipresent
        # krrsig: rumoured -> omnipresent
        "keyprops": [
            f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:hidden offset:{OFFSETS['step2']}",
        ],
        # Next key event is when the zone signatures become OMNIPRESENT,
        # Minus the time already elapsed.
        "nextev": IRETZSK - IPUB,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_rollover_enable_dnssec_step3(tld, alg, size, ns3):
    zone = f"step3.enable-dnssec.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    if tld == "manual":
        # Same as step 2, but zone signatures have become OMNIPRESENT.
        step = {
            "zone": zone,
            "cdss": CDSS,
            "keyprops": [
                f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:hidden offset:{OFFSETS['step3']}",
            ],
            "manual-mode": True,
            "nextev": None,
        }
        keys = isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)

        # Check logs.
        tag = keys[0].key.tag
        msg = f"keymgr-manual-mode: block transition CSK {zone}/ECDSAP256SHA256/{tag} type DS state HIDDEN to state RUMOURED"
        assert msg in ns3.log

        # Force step.
        with ns3.watch_log_from_here() as watcher:
            ns3.rndc(f"dnssec -step {zone}")
            watcher.wait_for_line(
                f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
            )

    step = {
        "zone": zone,
        "cdss": CDSS,
        # All signatures should be omnipresent, so the DS can be submitted.
        # zrrsig: rumoured -> omnipresent
        # ds: hidden -> rumoured
        "keyprops": [
            f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:rumoured offset:{OFFSETS['step3']}",
        ],
        # Next key event is when the DS can move to the OMNIPRESENT state.
        # This is after the retire interval.
        "nextev": IRETKSK,
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)


@pytest.mark.parametrize(
    "tld",
    [
        param("autosign"),
        param("manual"),
    ],
)
def test_rollover_enable_dnssec_step4(tld, alg, size, ns3):
    zone = f"step4.enable-dnssec.{tld}"
    policy = f"{POLICY}-{tld}"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # manual-mode: Nothing changing in the zone, no 'dnssec -step' required.

    step = {
        "zone": zone,
        "cdss": CDSS,
        # DS has been published long enough.
        # ds: rumoured -> omnipresent
        "keyprops": [
            f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{OFFSETS['step4']}",
        ],
        # Next key event is never, the zone dnssec-policy has been
        # established. So we fall back to the default loadkeys interval.
        "nextev": TIMEDELTA["PT1H"],
    }
    isctest.kasp.check_rollover_step(ns3, CONFIG, policy, step)
