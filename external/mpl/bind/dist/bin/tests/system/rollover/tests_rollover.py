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

import os
import shutil

from datetime import timedelta

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.update

import isctest
from isctest.kasp import KeyTimingMetadata

pytestmark = pytest.mark.extra_artifacts(
    [
        "*.axfr*",
        "dig.out*",
        "K*.key*",
        "K*.private*",
        "ns*/*.db",
        "ns*/*.db.infile",
        "ns*/*.db.jnl",
        "ns*/*.db.jbk",
        "ns*/*.db.signed",
        "ns*/*.db.signed.jnl",
        "ns*/*.conf",
        "ns*/dsset-*",
        "ns*/K*.key",
        "ns*/K*.private",
        "ns*/K*.state",
        "ns*/keygen.out.*",
        "ns*/settime.out.*",
        "ns*/signer.out.*",
        "ns*/zones",
    ]
)


def Ipub(config):
    return (
        config["dnskey-ttl"]
        + config["zone-propagation-delay"]
        + config["publish-safety"]
    )


def IpubC(config, rollover=True):
    if rollover:
        ttl = config["dnskey-ttl"]
        safety_interval = config["publish-safety"]
    else:
        ttl = config["max-zone-ttl"]
        safety_interval = timedelta(0)

    return ttl + config["zone-propagation-delay"] + safety_interval


def Iret(config, zsk=True, ksk=False, rollover=True):
    sign_delay = timedelta(0)
    safety_interval = timedelta(0)
    if rollover:
        sign_delay = config["signatures-validity"] - config["signatures-refresh"]
        safety_interval = config["retire-safety"]

    iretKSK = timedelta(0)
    if ksk:
        # KSK: Double-KSK Method: Iret = DprpP + TTLds
        iretKSK = (
            config["parent-propagation-delay"] + config["ds-ttl"] + safety_interval
        )

    iretZSK = timedelta(0)
    if zsk:
        # ZSK: Pre-Publication Method: Iret = Dsgn + Dprp + TTLsig
        iretZSK = (
            sign_delay
            + config["zone-propagation-delay"]
            + config["max-zone-ttl"]
            + safety_interval
        )

    return max(iretKSK, iretZSK)


def test_rollover_manual(servers):
    server = servers["ns3"]
    policy = "manual-rollover"
    config = {
        "dnskey-ttl": timedelta(hours=1),
        "ds-ttl": timedelta(days=1),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(hours=1),
        "retire-safety": timedelta(hours=1),
        "signatures-refresh": timedelta(days=7),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(minutes=5),
    }
    ttl = int(config["dnskey-ttl"].total_seconds())
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    zone = "manual-rollover.kasp"

    with server.watch_log_from_start() as watcher:
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(server, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    offset = -timedelta(days=7)
    for kp in expected:
        kp.set_expected_keytimes(config, offset=offset)

    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)

    # Schedule KSK rollover in six months.
    assert len(ksks) == 1
    ksk = ksks[0]
    startroll = expected[0].timing["Active"] + timedelta(days=30 * 6)
    expected[0].timing["Retired"] = startroll + Ipub(config)
    expected[0].timing["Removed"] = expected[0].timing["Retired"] + Iret(
        config, zsk=False, ksk=True
    )

    with server.watch_log_from_here() as watcher:
        server.rndc(f"dnssec -rollover -key {ksk.tag} -when {startroll} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(server, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)

    # Schedule KSK rollover now.
    now = KeyTimingMetadata.now()
    with server.watch_log_from_here() as watcher:
        server.rndc(f"dnssec -rollover -key {ksk.tag} -when {now} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(server, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    expected[0].metadata["Successor"] = expected[1].key.tag
    expected[1].metadata["Predecessor"] = expected[0].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)

    for kp in expected:
        off = offset
        if "Predecessor" in kp.metadata:
            off = 0
        kp.set_expected_keytimes(config, offset=off)

    expected[0].timing["Retired"] = now + Ipub(config)
    expected[0].timing["Removed"] = expected[0].timing["Retired"] + Iret(
        config, zsk=False, ksk=True
    )

    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)

    # Schedule ZSK rollover now.
    assert len(zsks) == 1
    zsk = zsks[0]
    now = KeyTimingMetadata.now()
    with server.watch_log_from_here() as watcher:
        server.rndc(f"dnssec -rollover -key {zsk.tag} -when {now} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(server, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk unlimited {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    expected[0].metadata["Successor"] = expected[1].key.tag
    expected[1].metadata["Predecessor"] = expected[0].key.tag
    expected[2].metadata["Successor"] = expected[3].key.tag
    expected[3].metadata["Predecessor"] = expected[2].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)

    # Try to schedule a ZSK rollover for an inactive key (should fail).
    zsk = expected[3].key
    response = server.rndc(f"dnssec -rollover -key {zsk.tag} {zone}")
    assert "key is not actively signing" in response


def test_rollover_multisigner(servers):
    server = servers["ns3"]
    policy = "multisigner-model2"
    config = {
        "dnskey-ttl": timedelta(hours=1),
        "ds-ttl": timedelta(days=1),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(hours=1),
        "retire-safety": timedelta(hours=1),
        "signatures-refresh": timedelta(days=5),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(minutes=5),
    }
    ttl = int(config["dnskey-ttl"].total_seconds())
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]

    offset = -timedelta(days=7)
    offval = int(offset.total_seconds())

    def keygen(zone):
        keygen_command = [
            os.environ.get("KEYGEN"),
            "-a",
            alg,
            "-L",
            "3600",
            "-M",
            "0:32767",
            zone,
        ]

        return isctest.run.cmd(keygen_command).stdout.decode("utf-8")

    zone = "multisigner-model2.kasp"

    isctest.kasp.check_dnssec_verify(server, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden tag-range:32768-65535",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:rumoured tag-range:32768-65535",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)

    newprops = [f"zsk unlimited {alg} {size} tag-range:0-32767"]
    expected2 = isctest.kasp.policy_to_properties(ttl, newprops)
    expected2[0].properties["private"] = False
    expected2[0].properties["legacy"] = True
    expected = expected + expected2

    ownkeys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    extkeys = isctest.kasp.keydir_to_keylist(zone)
    keys = ownkeys + extkeys
    ksks = [k for k in ownkeys if k.is_ksk()]
    zsks = [k for k in ownkeys if not k.is_ksk()]
    zsks = zsks + extkeys

    isctest.kasp.check_keys(zone, keys, expected)
    for kp in expected:
        kp.set_expected_keytimes(config)
    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)

    # Update zone with ZSK from another provider for zone.
    out = keygen(zone)
    newkeys = isctest.kasp.keystr_to_keylist(out)
    newprops = [f"zsk unlimited {alg} {size} tag-range:0-32767"]
    expected2 = isctest.kasp.policy_to_properties(ttl, newprops)
    expected2[0].properties["private"] = False
    expected2[0].properties["legacy"] = True
    expected = expected + expected2

    dnskey = newkeys[0].dnskey().split()
    rdata = " ".join(dnskey[4:])

    update_msg = dns.update.UpdateMessage(zone)
    update_msg.add(f"{dnskey[0]}", 3600, "DNSKEY", rdata)
    server.nsupdate(update_msg)

    isctest.kasp.check_dnssec_verify(server, zone)

    keys = keys + newkeys
    zsks = zsks + newkeys
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)

    # Remove ZSKs from the other providers for zone.
    dnskey2 = extkeys[0].dnskey().split()
    rdata2 = " ".join(dnskey2[4:])
    update_msg = dns.update.UpdateMessage(zone)
    update_msg.delete(f"{dnskey[0]}", "DNSKEY", rdata)
    update_msg.delete(f"{dnskey2[0]}", "DNSKEY", rdata2)
    server.nsupdate(update_msg)

    isctest.kasp.check_dnssec_verify(server, zone)

    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = ownkeys
    ksks = [k for k in ownkeys if k.is_ksk()]
    zsks = [k for k in ownkeys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)

    # A zone transitioning from single-signed to multi-signed. We should have
    # the old omnipresent keys outside of the desired key range and the new
    # keys in the desired key range.
    zone = "single-to-multisigner.kasp"

    isctest.kasp.check_dnssec_verify(server, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden tag-range:32768-65535",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden tag-range:32768-65535",
        f"ksk unlimited {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent tag-range:0-32767 offset:{offval}",
        f"zsk unlimited {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent tag-range:0-32767 offset:{offval}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    for kp in expected:
        kp.set_expected_keytimes(config)

    start = expected[0].key.get_timing("Created")
    expected[2].timing["Retired"] = start
    expected[2].timing["Removed"] = expected[2].timing["Retired"] + Iret(
        config, zsk=False, ksk=True
    )
    expected[3].timing["Retired"] = start
    expected[3].timing["Removed"] = expected[3].timing["Retired"] + Iret(
        config, zsk=True, ksk=False
    )

    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)


def check_rollover_step(server, config, policy, step):
    zone = step["zone"]
    keyprops = step["keyprops"]
    nextev = step["nextev"]
    cdss = step.get("cdss", None)
    keyrelationships = step.get("keyrelationships", None)
    smooth = step.get("smooth", False)
    ds_swap = step.get("ds-swap", True)
    cds_delete = step.get("cds-delete", False)
    check_keytimes = step.get("check-keytimes", True)
    zone_signed = step.get("zone-signed", True)

    isctest.log.info(f"check rollover step {zone}")

    if zone_signed:
        isctest.kasp.check_dnssec_verify(server, zone)

    ttl = int(config["dnskey-ttl"].total_seconds())
    expected = isctest.kasp.policy_to_properties(ttl, keyprops)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)

    for kp in expected:
        key = kp.key

        # Set expected key timing metadata.
        kp.set_expected_keytimes(config)

        # Set rollover relationships.
        if keyrelationships is not None:
            prd = keyrelationships[0]
            suc = keyrelationships[1]
            expected[prd].metadata["Successor"] = expected[suc].key.tag
            expected[suc].metadata["Predecessor"] = expected[prd].key.tag
            isctest.kasp.check_keyrelationships(keys, expected)

        # Policy changes may retire keys, set expected timing metadata.
        if kp.metadata["GoalState"] == "hidden" and "Retired" not in kp.timing:
            retired = kp.key.get_timing("Inactive")
            kp.timing["Retired"] = retired
            kp.timing["Removed"] = retired + Iret(
                config, zsk=key.is_zsk(), ksk=key.is_ksk()
            )

        # Check that CDS publication/withdrawal is logged.
        if "KSK" not in kp.metadata:
            continue
        if kp.metadata["KSK"] == "no":
            continue

        if ds_swap and kp.metadata["DSState"] == "rumoured":
            assert cdss is not None
            for algstr in ["CDNSKEY", "CDS (SHA-256)", "CDS (SHA-384)"]:
                if algstr in cdss:
                    isctest.kasp.check_cdslog(server, zone, key, algstr)
                else:
                    isctest.kasp.check_cdslog_prohibit(server, zone, key, algstr)

            # The DS can be introduced. We ignore any parent registration delay,
            # so set the DS publish time to now.
            server.rndc(f"dnssec -checkds -key {key.tag} published {zone}")

        if ds_swap and kp.metadata["DSState"] == "unretentive":
            # The DS can be withdrawn. We ignore any parent registration
            # delay, so set the DS withdraw time to now.
            server.rndc(f"dnssec -checkds -key {key.tag} withdrawn {zone}")

    if check_keytimes:
        isctest.kasp.check_keytimes(keys, expected)

    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks, cdss=cdss, cds_delete=cds_delete)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks, smooth=smooth)

    def check_next_key_event():
        return isctest.kasp.next_key_event_equals(server, zone, nextev)

    isctest.run.retry_with_timeout(check_next_key_event, timeout=5)


def test_rollover_enable_dnssec(servers):
    server = servers["ns3"]
    policy = "enable-dnssec"
    cdss = ["CDNSKEY", "CDS (SHA-256)"]
    config = {
        "dnskey-ttl": timedelta(seconds=300),
        "ds-ttl": timedelta(hours=2),
        "max-zone-ttl": timedelta(hours=12),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(minutes=5),
        "retire-safety": timedelta(minutes=20),
        "signatures-refresh": timedelta(days=7),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(minutes=5),
    }
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]

    ipub = Ipub(config)
    ipubC = IpubC(config, rollover=False)
    iretZSK = Iret(config, rollover=False)
    iretKSK = Iret(config, zsk=False, ksk=True, rollover=False)
    offsets = {
        "step1": 0,
        "step2": -int(ipub.total_seconds()),
        "step3": -int(iretZSK.total_seconds()),
        "step4": -int(ipubC.total_seconds() + iretKSK.total_seconds()),
    }

    steps = [
        {
            # Step 1.
            "zone": "step1.enable-dnssec.autosign",
            "cdss": cdss,
            "keyprops": [
                f"csk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden offset:{offsets['step1']}",
            ],
            # Next key event is when the DNSKEY RRset becomes OMNIPRESENT,
            # after the publication interval.
            "nextev": ipub,
        },
        {
            # Step 2.
            "zone": "step2.enable-dnssec.autosign",
            "cdss": cdss,
            # The DNSKEY is omnipresent, but the zone signatures not yet.
            # Thus, the DS remains hidden.
            # dnskey: rumoured -> omnipresent
            # krrsig: rumoured -> omnipresent
            "keyprops": [
                f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:hidden offset:{offsets['step2']}",
            ],
            # Next key event is when the zone signatures become OMNIPRESENT,
            # Minus the time already elapsed.
            "nextev": iretZSK - ipub,
        },
        {
            # Step 3.
            "zone": "step3.enable-dnssec.autosign",
            "cdss": cdss,
            # All signatures should be omnipresent, so the DS can be submitted.
            # zrrsig: rumoured -> omnipresent
            # ds: hidden -> rumoured
            "keyprops": [
                f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:rumoured offset:{offsets['step3']}",
            ],
            # Next key event is when the DS can move to the OMNIPRESENT state.
            # This is after the retire interval.
            "nextev": iretKSK,
        },
        {
            # Step 4.
            "zone": "step4.enable-dnssec.autosign",
            "cdss": cdss,
            # DS has been published long enough.
            # ds: rumoured -> omnipresent
            "keyprops": [
                f"csk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step4']}",
            ],
            # Next key event is never, the zone dnssec-policy has been
            # established. So we fall back to the default loadkeys interval.
            "nextev": timedelta(hours=1),
        },
    ]

    for step in steps:
        check_rollover_step(server, config, policy, step)


def test_rollover_zsk_prepublication(servers):
    server = servers["ns3"]
    policy = "zsk-prepub"
    config = {
        "dnskey-ttl": timedelta(seconds=3600),
        "ds-ttl": timedelta(days=1),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(days=1),
        "purge-keys": timedelta(hours=1),
        "retire-safety": timedelta(days=2),
        "signatures-refresh": timedelta(days=7),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(hours=1),
    }
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    zsk_lifetime = timedelta(days=30)
    lifetime_policy = int(zsk_lifetime.total_seconds())

    ipub = Ipub(config)
    iret = Iret(config, rollover=True)
    keyttlprop = config["dnskey-ttl"] + config["zone-propagation-delay"]
    offsets = {}
    offsets["step1-p"] = -int(timedelta(days=7).total_seconds())
    offsets["step2-p"] = -int(zsk_lifetime.total_seconds() - ipub.total_seconds())
    offsets["step2-s"] = 0
    offsets["step3-p"] = -int(zsk_lifetime.total_seconds())
    offsets["step3-s"] = -int(ipub.total_seconds())
    offsets["step4-p"] = offsets["step3-p"] - int(iret.total_seconds())
    offsets["step4-s"] = offsets["step3-s"] - int(iret.total_seconds())
    offsets["step5-p"] = offsets["step4-p"] - int(keyttlprop.total_seconds())
    offsets["step5-s"] = offsets["step4-s"] - int(keyttlprop.total_seconds())
    offsets["step6-p"] = offsets["step5-p"] - int(config["purge-keys"].total_seconds())
    offsets["step6-s"] = offsets["step5-s"] - int(config["purge-keys"].total_seconds())

    steps = [
        {
            # Step 1.
            # Introduce the first key. This will immediately be active.
            "zone": "step1.zsk-prepub.autosign",
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step1-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step1-p']}",
            ],
            # Next key event is when the successor ZSK needs to be published.
            # That is the ZSK lifetime - prepublication time (minus time
            # already passed).
            "nextev": zsk_lifetime - ipub - timedelta(days=7),
        },
        {
            # Step 2.
            # It is time to pre-publish the successor ZSK.
            # ZSK1 goal: omnipresent -> hidden
            # ZSK2 goal: hidden -> omnipresent
            # ZSK2 dnskey: hidden -> rumoured
            "zone": "step2.zsk-prepub.autosign",
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step2-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step2-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden offset:{offsets['step2-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the successor ZSK becomes OMNIPRESENT.
            # That is the DNSKEY TTL plus the zone propagation delay
            "nextev": ipub,
        },
        {
            # Step 3.
            # Predecessor ZSK is no longer actively signing. Successor ZSK is
            # now actively signing.
            # ZSK1 zrrsig: omnipresent -> unretentive
            # ZSK2 dnskey: rumoured -> omnipresent
            # ZSK2 zrrsig: hidden -> rumoured
            "zone": "step3.zsk-prepub.autosign",
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step3-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:unretentive offset:{offsets['step3-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:rumoured offset:{offsets['step3-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when all the RRSIG records have been replaced
            # with signatures of the new ZSK, in other words when ZRRSIG
            # becomes OMNIPRESENT.
            "nextev": iret,
            # Set 'smooth' to true so expected signatures of subdomain are
            # from the predecessor ZSK.
            "smooth": True,
        },
        {
            # Step 4.
            # Predecessor ZSK is no longer needed. All RRsets are signed with
            # the successor ZSK.
            # ZSK1 dnskey: omnipresent -> unretentive
            # ZSK1 zrrsig: unretentive -> hidden
            # ZSK2 zrrsig: rumoured -> omnipresent
            "zone": "step4.zsk-prepub.autosign",
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step4-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:hidden dnskey:unretentive zrrsig:hidden offset:{offsets['step4-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step4-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the DNSKEY enters the HIDDEN state.
            # This is the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
        },
        {
            # Step 5.
            # Predecessor ZSK is now removed.
            # ZSK1 dnskey: unretentive -> hidden
            "zone": "step5.zsk-prepub.autosign",
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step5-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:hidden dnskey:hidden zrrsig:hidden offset:{offsets['step5-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step5-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the new successor needs to be published.
            # This is the ZSK lifetime minus Iret minus Ipub minus time
            # elapsed.
            "nextev": zsk_lifetime - iret - ipub - keyttlprop,
        },
        {
            # Step 6.
            # Predecessor ZSK is now purged.
            "zone": "step6.zsk-prepub.autosign",
            "keyprops": [
                f"ksk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step6-p']}",
                f"zsk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step6-s']}",
            ],
            "nextev": None,
        },
    ]

    for step in steps:
        check_rollover_step(server, config, policy, step)


def test_rollover_ksk_doubleksk(servers):
    server = servers["ns3"]
    policy = "ksk-doubleksk"
    cdss = ["CDS (SHA-256)"]
    config = {
        "dnskey-ttl": timedelta(hours=2),
        "ds-ttl": timedelta(seconds=3600),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(days=1),
        "purge-keys": timedelta(hours=1),
        "retire-safety": timedelta(days=2),
        "signatures-refresh": timedelta(days=7),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(hours=1),
    }
    ttl = int(config["dnskey-ttl"].total_seconds())
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    ksk_lifetime = timedelta(days=60)
    lifetime_policy = int(ksk_lifetime.total_seconds())

    ipub = Ipub(config)
    ipubc = IpubC(config)
    iret = Iret(config, zsk=False, ksk=True)
    keyttlprop = config["dnskey-ttl"] + config["zone-propagation-delay"]
    offsets = {}
    offsets["step1-p"] = -int(timedelta(days=7).total_seconds())
    offsets["step2-p"] = -int(ksk_lifetime.total_seconds() - ipubc.total_seconds())
    offsets["step2-s"] = 0
    offsets["step3-p"] = -int(ksk_lifetime.total_seconds())
    offsets["step3-s"] = -int(ipubc.total_seconds())
    offsets["step4-p"] = offsets["step3-p"] - int(iret.total_seconds())
    offsets["step4-s"] = offsets["step3-s"] - int(iret.total_seconds())
    offsets["step5-p"] = offsets["step4-p"] - int(keyttlprop.total_seconds())
    offsets["step5-s"] = offsets["step4-s"] - int(keyttlprop.total_seconds())
    offsets["step6-p"] = offsets["step5-p"] - int(config["purge-keys"].total_seconds())
    offsets["step6-s"] = offsets["step5-s"] - int(config["purge-keys"].total_seconds())

    steps = [
        {
            # Step 1.
            # Introduce the first key. This will immediately be active.
            "zone": "step1.ksk-doubleksk.autosign",
            "cdss": cdss,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step1-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step1-p']}",
            ],
            # Next key event is when the successor KSK needs to be published.
            # That is the KSK lifetime - prepublication time (minus time
            # already passed).
            "nextev": ksk_lifetime - ipub - timedelta(days=7),
        },
        {
            # Step 2.
            # Successor KSK is prepublished (and signs DNSKEY RRset).
            # KSK1 goal: omnipresent -> hidden
            # KSK2 goal: hidden -> omnipresent
            # KSK2 dnskey: hidden -> rumoured
            # KSK2 krrsig: hidden -> rumoured
            "zone": "step2.ksk-doubleksk.autosign",
            "cdss": cdss,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step2-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step2-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden offset:{offsets['step2-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the successor KSK becomes OMNIPRESENT.
            "nextev": ipub,
        },
        {
            # Step 3.
            # The successor DNSKEY RRset has become omnipresent.  The
            # predecessor DS  can be withdrawn and the successor DS can be
            # introduced.
            # KSK1 ds: omnipresent -> unretentive
            # KSK2 dnskey: rumoured -> omnipresent
            # KSK2 krrsig: rumoured -> omnipresent
            # KSK2 ds: hidden -> rumoured
            "zone": "step3.ksk-doubleksk.autosign",
            "cdss": cdss,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step3-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{offsets['step3-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{offsets['step3-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the predecessor DS has been replaced with
            # the successor DS and enough time has passed such that the all
            # validators that have this DS RRset cached only know about the
            # successor DS.  This is the the retire interval.
            "nextev": iret,
        },
        {
            # Step 4.
            # The predecessor DNSKEY may be removed, the successor DS is
            # omnipresent.
            # KSK1 dnskey: omnipresent -> unretentive
            # KSK1 krrsig: omnipresent -> unretentive
            # KSK1 ds: unretentive -> hidden
            # KSK2 ds: rumoured -> omnipresent
            "zone": "step4.ksk-doubleksk.autosign",
            "cdss": cdss,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step4-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:unretentive krrsig:unretentive ds:hidden offset:{offsets['step4-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step4-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the DNSKEY enters the HIDDEN state.
            # This is the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
        },
        {
            # Step 5.
            # The predecessor DNSKEY is long enough removed from the zone it
            # has become hidden.
            # KSK1 dnskey: unretentive -> hidden
            # KSK1 krrsig: unretentive -> hidden
            "zone": "step5.ksk-doubleksk.autosign",
            "cdss": cdss,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step5-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:hidden krrsig:hidden ds:hidden offset:{offsets['step5-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step5-s']}",
            ],
            "keyrelationships": [1, 2],
            # Next key event is when the new successor needs to be published.
            # This is the KSK lifetime minus Ipub minus Iret minus time elapsed.
            "nextev": ksk_lifetime - ipub - iret - keyttlprop,
        },
        {
            # Step 6.
            # Predecessor KSK is now purged.
            "zone": "step6.ksk-doubleksk.autosign",
            "cdss": cdss,
            "keyprops": [
                f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step6-p']}",
                f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step6-s']}",
            ],
            "nextev": None,
        },
    ]

    for step in steps:
        check_rollover_step(server, config, policy, step)

    # Test #2375: Scheduled rollovers are happening faster than they can finish.
    zone = "three-is-a-crowd.kasp"
    isctest.log.info(
        "check that fast rollovers do not remove dependent keys from zone (#2375)"
    )
    offset1 = -int(timedelta(days=60).total_seconds())
    offset2 = -int(timedelta(hours=27).total_seconds())
    isctest.kasp.check_dnssec_verify(server, zone)
    keyprops = [
        f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{offset1}",
        f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{offset2}",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offset1}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, keyprops)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)
    expected[0].metadata["Successor"] = expected[1].key.tag
    expected[1].metadata["Predecessor"] = expected[0].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)
    for kp in expected:
        kp.set_expected_keytimes(config, offset=None)
    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks, cdss=cdss)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)
    # Rollover successor KSK (with DS in rumoured state).
    key = expected[1].key
    now = KeyTimingMetadata.now()
    with server.watch_log_from_here() as watcher:
        server.rndc(f"dnssec -rollover -key {key.tag} -when {now} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")
    isctest.kasp.check_dnssec_verify(server, zone)
    # We now expect four keys (3x KSK, 1x ZSK).
    keyprops = [
        f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{offset1}",
        f"ksk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{offset2}",
        f"ksk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden offset:0",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offset1}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, keyprops)
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)
    expected[0].metadata["Successor"] = expected[1].key.tag
    expected[1].metadata["Predecessor"] = expected[0].key.tag
    # Three is a crowd scenario.
    expected[1].metadata["Successor"] = expected[2].key.tag
    expected[2].metadata["Predecessor"] = expected[1].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)
    for kp in expected:
        kp.set_expected_keytimes(config, offset=None)
    # The first successor KSK is already being retired.
    expected[1].timing["Retired"] = now + ipub
    expected[1].timing["Removed"] = now + ipub + iret
    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=policy)
    isctest.kasp.check_apex(server, zone, ksks, zsks, cdss=cdss)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)


def test_rollover_csk_roll1(servers):
    server = servers["ns3"]
    policy = "csk-roll1"
    cdss = ["CDNSKEY", "CDS (SHA-384)"]
    config = {
        "dnskey-ttl": timedelta(hours=1),
        "ds-ttl": timedelta(seconds=3600),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(hours=1),
        "purge-keys": timedelta(hours=1),
        "retire-safety": timedelta(hours=2),
        "signatures-refresh": timedelta(days=5),
        "signatures-validity": timedelta(days=30),
        "zone-propagation-delay": timedelta(hours=1),
    }
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    csk_lifetime = timedelta(days=31 * 6)
    lifetime_policy = int(csk_lifetime.total_seconds())

    ipub = Ipub(config)
    iretZSK = Iret(config)
    iretKSK = Iret(config, zsk=False, ksk=True)
    keyttlprop = config["dnskey-ttl"] + config["zone-propagation-delay"]
    signdelay = iretZSK - iretKSK - keyttlprop
    offsets = {}
    offsets["step1-p"] = -int(timedelta(days=7).total_seconds())
    offsets["step2-p"] = -int(csk_lifetime.total_seconds() - ipub.total_seconds())
    offsets["step2-s"] = 0
    offsets["step3-p"] = -int(csk_lifetime.total_seconds())
    offsets["step3-s"] = -int(ipub.total_seconds())
    offsets["step4-p"] = offsets["step3-p"] - int(iretKSK.total_seconds())
    offsets["step4-s"] = offsets["step3-s"] - int(iretKSK.total_seconds())
    offsets["step5-p"] = offsets["step4-p"] - int(keyttlprop.total_seconds())
    offsets["step5-s"] = offsets["step4-s"] - int(keyttlprop.total_seconds())
    offsets["step6-p"] = offsets["step5-p"] - int(signdelay.total_seconds())
    offsets["step6-s"] = offsets["step5-s"] - int(signdelay.total_seconds())
    offsets["step7-p"] = offsets["step6-p"] - int(keyttlprop.total_seconds())
    offsets["step7-s"] = offsets["step6-s"] - int(keyttlprop.total_seconds())
    offsets["step8-p"] = offsets["step7-p"] - int(config["purge-keys"].total_seconds())
    offsets["step8-s"] = offsets["step7-s"] - int(config["purge-keys"].total_seconds())

    steps = [
        {
            # Step 1.
            # Introduce the first key. This will immediately be active.
            "zone": "step1.csk-roll1.autosign",
            "cdss": cdss,
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step1-p']}",
            ],
            # Next key event is when the successor CSK needs to be published
            # minus time already elapsed. This is Lcsk - Ipub + Dreg (we ignore
            # registration delay).
            "nextev": csk_lifetime - ipub - timedelta(days=7),
        },
        {
            # Step 2.
            # Successor CSK is prepublished (signs DNSKEY RRset, but not yet
            # other RRsets).
            # CSK1 goal: omnipresent -> hidden
            # CSK2 goal: hidden -> omnipresent
            # CSK2 dnskey: hidden -> rumoured
            # CSK2 krrsig: hidden -> rumoured
            "zone": "step2.csk-roll1.autosign",
            "cdss": cdss,
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step2-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:hidden ds:hidden offset:{offsets['step2-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the successor CSK becomes OMNIPRESENT.
            "nextev": ipub,
        },
        {
            # Step 3.
            # Successor CSK becomes omnipresent, meaning we can start signing
            # the remainder of the zone with the successor CSK, and we can
            # submit the DS.
            "zone": "step3.csk-roll1.autosign",
            "cdss": cdss,
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
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:unretentive ds:unretentive offset:{offsets['step3-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:rumoured offset:{offsets['step3-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the predecessor DS has been replaced with
            # the successor DS and enough time has passed such that the all
            # validators that have this DS RRset cached only know about the
            # successor DS.  This is the the retire interval.
            "nextev": iretKSK,
            # Set 'smooth' to true so expected signatures of subdomain are
            # from the predecessor ZSK.
            "smooth": True,
        },
        {
            # Step 4.
            "zone": "step4.csk-roll1.autosign",
            "cdss": cdss,
            # The predecessor CSK is no longer signing the DNSKEY RRset.
            # CSK1 krrsig: omnipresent -> unretentive
            # The predecessor DS is hidden. The successor DS is now omnipresent.
            # CSK1 ds: unretentive -> hidden
            # CSK2 ds: rumoured -> omnipresent
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:unretentive zrrsig:unretentive ds:hidden offset:{offsets['step4-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:omnipresent offset:{offsets['step4-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the KRRSIG enters the HIDDEN state.
            # This is the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
            # We already swapped the DS in the previous step, so disable ds-swap.
            "ds-swap": False,
        },
        {
            # Step 5.
            "zone": "step5.csk-roll1.autosign",
            "cdss": cdss,
            # The predecessor KRRSIG records are now all hidden.
            # CSK1 krrsig: unretentive -> hidden
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:hidden zrrsig:unretentive ds:hidden offset:{offsets['step5-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:omnipresent offset:{offsets['step5-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the DNSKEY can be removed.  This is when
            # all ZRRSIG records have been replaced with signatures of the new
            # CSK.
            "nextev": signdelay,
        },
        {
            # Step 6.
            "zone": "step6.csk-roll1.autosign",
            "cdss": cdss,
            # The predecessor ZRRSIG records are now all hidden (so the DNSKEY
            # can be removed).
            # CSK1 dnskey: omnipresent -> unretentive
            # CSK1 zrrsig: unretentive -> hidden
            # CSK2 zrrsig: rumoured -> omnipresent
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:unretentive krrsig:hidden zrrsig:hidden ds:hidden offset:{offsets['step6-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step6-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the DNSKEY enters the HIDDEN state.
            # This is the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
        },
        {
            # Step 7.
            "zone": "step7.csk-roll1.autosign",
            "cdss": cdss,
            # The predecessor CSK is now completely HIDDEN.
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:hidden krrsig:hidden zrrsig:hidden ds:hidden offset:{offsets['step7-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step7-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the new successor needs to be published.
            # This is the Lcsk, minus time passed since the key started signing,
            # minus the prepublication time.
            "nextev": csk_lifetime - iretZSK - ipub - keyttlprop,
        },
        {
            # Step 8.
            # Predecessor CSK is now purged.
            "zone": "step8.csk-roll1.autosign",
            "cdss": cdss,
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step8-s']}",
            ],
            "nextev": None,
        },
    ]

    for step in steps:
        check_rollover_step(server, config, policy, step)


def test_rollover_csk_roll2(servers):
    server = servers["ns3"]
    policy = "csk-roll2"
    cdss = ["CDNSKEY", "CDS (SHA-256)", "CDS (SHA-384)"]
    config = {
        "dnskey-ttl": timedelta(hours=1),
        "ds-ttl": timedelta(seconds=3600),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(days=7),
        "publish-safety": timedelta(hours=1),
        "purge-keys": timedelta(0),
        "retire-safety": timedelta(hours=1),
        "signatures-refresh": timedelta(hours=12),
        "signatures-validity": timedelta(days=1),
        "zone-propagation-delay": timedelta(hours=1),
    }
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    csk_lifetime = timedelta(days=31 * 6)
    lifetime_policy = int(csk_lifetime.total_seconds())

    ipub = Ipub(config)
    iret = Iret(config, zsk=True, ksk=True)
    iretZSK = Iret(config)
    iretKSK = Iret(config, ksk=True)
    keyttlprop = config["dnskey-ttl"] + config["zone-propagation-delay"]
    offsets = {}
    offsets["step1-p"] = -int(timedelta(days=7).total_seconds())
    offsets["step2-p"] = -int(csk_lifetime.total_seconds() - ipub.total_seconds())
    offsets["step2-s"] = 0
    offsets["step3-p"] = -int(csk_lifetime.total_seconds())
    offsets["step3-s"] = -int(ipub.total_seconds())
    offsets["step4-p"] = offsets["step3-p"] - int(iretZSK.total_seconds())
    offsets["step4-s"] = offsets["step3-s"] - int(iretZSK.total_seconds())
    offsets["step5-p"] = offsets["step4-p"] - int(
        iretKSK.total_seconds() - iretZSK.total_seconds()
    )
    offsets["step5-s"] = offsets["step4-s"] - int(
        iretKSK.total_seconds() - iretZSK.total_seconds()
    )
    offsets["step6-p"] = offsets["step5-p"] - int(keyttlprop.total_seconds())
    offsets["step6-s"] = offsets["step5-s"] - int(keyttlprop.total_seconds())
    offsets["step7-p"] = offsets["step6-p"] - int(timedelta(days=90).total_seconds())
    offsets["step7-s"] = offsets["step6-s"] - int(timedelta(days=90).total_seconds())

    steps = [
        {
            # Step 1.
            # Introduce the first key. This will immediately be active.
            "zone": "step1.csk-roll2.autosign",
            "cdss": cdss,
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step1-p']}",
            ],
            # Next key event is when the successor CSK needs to be published
            # minus time already elapsed. This is Lcsk - Ipub + Dreg (we ignore
            # registration delay).
            "nextev": csk_lifetime - ipub - timedelta(days=7),
        },
        {
            # Step 2.
            # Successor CSK is prepublished (signs DNSKEY RRset, but not yet
            # other RRsets).
            # CSK1 goal: omnipresent -> hidden
            # CSK2 goal: hidden -> omnipresent
            # CSK2 dnskey: hidden -> rumoured
            # CSK2 krrsig: hidden -> rumoured
            "zone": "step2.csk-roll2.autosign",
            "cdss": cdss,
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step2-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:hidden ds:hidden offset:{offsets['step2-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the successor CSK becomes OMNIPRESENT.
            "nextev": ipub,
        },
        {
            # Step 3.
            # Successor CSK becomes omnipresent, meaning we can start signing
            # the remainder of the zone with the successor CSK, and we can
            # submit the DS.
            "zone": "step3.csk-roll2.autosign",
            "cdss": cdss,
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
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:unretentive ds:unretentive offset:{offsets['step3-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:rumoured offset:{offsets['step3-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the predecessor DS has been replaced with
            # the successor DS and enough time has passed such that the all
            # validators that have this DS RRset cached only know about the
            # successor DS.  This is the the retire interval.
            "nextev": iretZSK,
            # Set 'smooth' to true so expected signatures of subdomain are
            # from the predecessor ZSK.
            "smooth": True,
        },
        {
            # Step 4.
            "zone": "step4.csk-roll2.autosign",
            "cdss": cdss,
            # The predecessor ZRRSIG is HIDDEN. The successor ZRRSIG is
            # OMNIPRESENT.
            # CSK1 zrrsig: unretentive -> hidden
            # CSK2 zrrsig: rumoured -> omnipresent
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:hidden ds:unretentive offset:{offsets['step4-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:rumoured offset:{offsets['step4-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the predecessor DS has been replaced with
            # the successor DS and enough time has passed such that the all
            # validators that have this DS RRset cached only know about the
            # successor DS. This is the retire interval of the KSK part (minus)
            # time already elapsed).
            "nextev": iret - iretZSK,
            # We already swapped the DS in the previous step, so disable ds-swap.
            "ds-swap": False,
        },
        {
            # Step 5.
            "zone": "step5.csk-roll2.autosign",
            "cdss": cdss,
            # The predecessor DNSKEY can be removed.
            # CSK1 dnskey: omnipresent -> unretentive
            # CSK1 krrsig: omnipresent -> unretentive
            # CSK1 ds: unretentive -> hidden
            # The successor key is now fully OMNIPRESENT.
            # CSK2 ds: rumoured -> omnipresent
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:unretentive krrsig:unretentive zrrsig:hidden ds:hidden offset:{offsets['step5-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step5-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the DNSKEY enters the HIDDEN state.
            # This is the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
        },
        {
            # Step 6.
            "zone": "step6.csk-roll2.autosign",
            "cdss": cdss,
            # The predecessor CSK is now completely HIDDEN.
            # CSK1 dnskey: unretentive -> hidden
            # CSK1 krrsig: unretentive -> hidden
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:hidden krrsig:hidden zrrsig:hidden ds:hidden offset:{offsets['step6-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step6-s']}",
            ],
            "keyrelationships": [0, 1],
            # Next key event is when the new successor needs to be published.
            # This is the Lcsk, minus time passed since the key was published.
            "nextev": csk_lifetime - iret - ipub - keyttlprop,
        },
        {
            # Step 7.
            "zone": "step7.csk-roll2.autosign",
            "cdss": cdss,
            # The predecessor CSK is now completely HIDDEN.
            "keyprops": [
                f"csk {lifetime_policy} {alg} {size} goal:hidden dnskey:hidden krrsig:hidden zrrsig:hidden ds:hidden offset:{offsets['step7-p']}",
                f"csk {lifetime_policy} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step7-s']}",
            ],
            "keyrelationships": [0, 1],
            "nextev": None,
        },
    ]

    for step in steps:
        check_rollover_step(server, config, policy, step)


def test_rollover_policy_changes(servers):
    server = servers["ns6"]
    cdss = ["CDNSKEY", "CDS (SHA-256)"]
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]

    default_config = {
        "dnskey-ttl": timedelta(hours=1),
        "ds-ttl": timedelta(days=1),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(hours=1),
        "purge-keys": timedelta(days=90),
        "retire-safety": timedelta(hours=1),
        "signatures-refresh": timedelta(days=5),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(seconds=300),
    }

    unsigning_config = default_config.copy()
    unsigning_config["dnskey-ttl"] = timedelta(seconds=7200)

    algoroll_config = {
        "dnskey-ttl": timedelta(hours=1),
        "ds-ttl": timedelta(seconds=7200),
        "max-zone-ttl": timedelta(hours=6),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(hours=1),
        "purge-keys": timedelta(days=90),
        "retire-safety": timedelta(hours=2),
        "signatures-refresh": timedelta(days=5),
        "signatures-validity": timedelta(days=30),
        "zone-propagation-delay": timedelta(seconds=3600),
    }

    start_time = KeyTimingMetadata.now()

    # Test dynamic zones that switch to inline-signing.
    isctest.log.info("check dynamic zone that switches to inline-signing")
    d2i = {
        "zone": "dynamic2inline.kasp",
        "cdss": cdss,
        "config": default_config,
        "policy": "default",
        "keyprops": [
            f"csk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
        ],
        "nextev": None,
    }
    steps = [d2i]

    # Test key lifetime changes.
    isctest.log.info("check key lifetime changes are updated correctly")
    lifetime = {
        "P1Y": int(timedelta(days=365).total_seconds()),
        "P6M": int(timedelta(days=31 * 6).total_seconds()),
        "P60D": int(timedelta(days=60).total_seconds()),
    }
    lifetime_update_tests = [
        {
            "zone": "shorter-lifetime",
            "policy": "long-lifetime",
            "lifetime": lifetime["P1Y"],
        },
        {
            "zone": "longer-lifetime",
            "policy": "short-lifetime",
            "lifetime": lifetime["P6M"],
        },
        {
            "zone": "limit-lifetime",
            "policy": "unlimited-lifetime",
            "lifetime": 0,
        },
        {
            "zone": "unlimit-lifetime",
            "policy": "short-lifetime",
            "lifetime": lifetime["P6M"],
        },
    ]
    for lut in lifetime_update_tests:
        step = {
            "zone": lut["zone"],
            "cdss": cdss,
            "config": default_config,
            "policy": lut["policy"],
            "keyprops": [
                f"csk {lut['lifetime']} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
            ],
            "nextev": None,
        }
        steps.append(step)

    # Test going insecure.
    isctest.log.info("check going insecure")
    offset = -timedelta(days=10)
    offval = int(offset.total_seconds())
    zones = [
        "step1.going-insecure.kasp",
        "step1.going-insecure-dynamic.kasp",
    ]
    for zone in zones:
        step = {
            "zone": zone,
            "cdss": cdss,
            "config": unsigning_config,
            "policy": "unsigning",
            "keyprops": [
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offval}",
                f"zsk {lifetime['P60D']} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offval}",
            ],
            "nextev": None,
        }
        steps.append(step)

    # Test going straight to none.
    isctest.log.info("check going straight to none")
    zones = [
        "step1.going-straight-to-none.kasp",
        "step1.going-straight-to-none-dynamic.kasp",
    ]
    for zone in zones:
        step = {
            "zone": zone,
            "cdss": cdss,
            "config": default_config,
            "policy": "default",
            "keyprops": [
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offval}",
            ],
            "nextev": None,
        }
        steps.append(step)

    # Test algorithm rollover (KSK/ZSK split).
    isctest.log.info("check algorithm rollover ksk/zsk split")
    offset = -timedelta(days=7)
    offval = int(offset.total_seconds())
    step = {
        "zone": "step1.algorithm-roll.kasp",
        "cdss": cdss,
        "config": algoroll_config,
        "policy": "rsasha256",
        "keyprops": [
            f"ksk 0 8 2048 goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offval}",
            f"zsk 0 8 2048 goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offval}",
        ],
        "nextev": timedelta(hours=1),
    }
    steps.append(step)

    # Test algorithm rollover (CSK).
    isctest.log.info("check algorithm rollover csk")
    step = {
        "zone": "step1.csk-algorithm-roll.kasp",
        "cdss": cdss,
        "config": algoroll_config,
        "policy": "csk-algoroll",
        "keyprops": [
            f"csk 0 8 2048 goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offval}",
        ],
        "nextev": timedelta(hours=1),
    }
    steps.append(step)

    for step in steps:
        check_rollover_step(server, step["config"], step["policy"], step)

    # Reconfigure, changing DNSSEC policies and other configuration options,
    # triggering algorithm rollovers and other dnssec-policy changes.
    shutil.copyfile("ns6/named2.conf", "ns6/named.conf")
    server.rndc("reconfig")
    # Calculate time passed to correctly check for next key events.
    now = KeyTimingMetadata.now()
    time_passed = now.value - start_time.value

    # Test dynamic zones that switch to inline-signing (after reconfig).
    steps = [d2i]

    # Test key lifetime changes (after reconfig).
    lifetime_update_tests = [
        {
            "zone": "shorter-lifetime",
            "policy": "short-lifetime",
            "lifetime": lifetime["P6M"],
        },
        {
            "zone": "longer-lifetime",
            "policy": "long-lifetime",
            "lifetime": lifetime["P1Y"],
        },
        {
            "zone": "limit-lifetime",
            "policy": "short-lifetime",
            "lifetime": lifetime["P6M"],
        },
        {
            "zone": "unlimit-lifetime",
            "policy": "unlimited-lifetime",
            "lifetime": 0,
        },
    ]
    for lut in lifetime_update_tests:
        step = {
            "zone": lut["zone"],
            "cdss": cdss,
            "config": default_config,
            "policy": lut["policy"],
            "keyprops": [
                f"csk {lut['lifetime']} {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
            ],
            "nextev": None,
        }
        steps.append(step)

    # Test going insecure (after reconfig).
    isctest.log.info("check going insecure (after reconfig)")
    oldttl = unsigning_config["dnskey-ttl"]
    offset = -timedelta(days=10)
    offval = int(offset.total_seconds())
    zones = ["going-insecure.kasp", "going-insecure-dynamic.kasp"]
    for parent in zones:
        # Step 1.
        # Key goal states should be HIDDEN.
        # The DS may be removed if we are going insecure.
        step = {
            "zone": f"step1.{parent}",
            "cdss": cdss,
            "config": default_config,
            "policy": "insecure",
            "keyprops": [
                f"ksk 0 {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{offval}",
                f"zsk {lifetime['P60D']} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{offval}",
            ],
            # Next key event is when the DS becomes HIDDEN. This
            # happens after the# parent propagation delay plus DS TTL.
            "nextev": default_config["ds-ttl"]
            + default_config["parent-propagation-delay"],
            # Going insecure, check for CDS/CDNSKEY DELETE, and skip key timing checks.
            "cds-delete": True,
            "check-keytimes": False,
        }
        steps.append(step)

        # Step 2.
        # The DS is long enough removed from the zone to be considered
        # HIDDEN.  This means the DNSKEY and the KSK signatures can be
        # removed.
        step = {
            "zone": f"step2.{parent}",
            "cdss": cdss,
            "config": default_config,
            "policy": "insecure",
            "keyprops": [
                f"ksk 0 {alg} {size} goal:hidden dnskey:unretentive krrsig:unretentive ds:hidden offset:{offval}",
                f"zsk {lifetime['P60D']} {alg} {size} goal:hidden dnskey:unretentive zrrsig:unretentive offset:{offval}",
            ],
            # Next key event is when the DNSKEY becomes HIDDEN.
            # This happens after the propagation delay, plus DNSKEY TTL.
            "nextev": oldttl + default_config["zone-propagation-delay"],
            # Zone is no longer signed.
            "zone-signed": False,
            "check-keytimes": False,
        }
        steps.append(step)

    # Test going straight to none.
    isctest.log.info("check going straight to none (after reconfig)")
    zones = [
        "step1.going-straight-to-none.kasp",
        "step1.going-straight-to-none-dynamic.kasp",
    ]
    for zone in zones:
        step = {
            "zone": zone,
            "cdss": cdss,
            "config": default_config,
            "policy": None,
            # These zones will go bogus after signatures expire, but
            # remain validly signed for now.
            "keyprops": [
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offval}",
            ],
            "nextev": None,
        }
        steps.append(step)

    # Test algorithm rollover (KSK/ZSK split) (after reconfig).
    isctest.log.info("check algorithm rollover ksk/zsk split (after reconfig)")
    offset = -timedelta(days=7)
    offval = int(offset.total_seconds())
    ipub = Ipub(algoroll_config)
    ipubc = IpubC(algoroll_config, rollover=False)
    iret = Iret(algoroll_config, rollover=False)
    iretKSK = Iret(algoroll_config, zsk=False, ksk=True, rollover=False)
    keyttlprop = (
        algoroll_config["dnskey-ttl"] + algoroll_config["zone-propagation-delay"]
    )
    offsets = {}
    offsets["step2"] = -int(ipub.total_seconds())
    offsets["step3"] = -int(iret.total_seconds())
    offsets["step4"] = offsets["step3"] - int(iretKSK.total_seconds())
    offsets["step5"] = offsets["step4"] - int(keyttlprop.total_seconds())
    offsets["step6"] = offsets["step5"] - int(iret.total_seconds())
    algo_steps = [
        {
            # Step 1.
            "zone": "step1.algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "ecdsa256",
            "keyprops": [
                # The RSASHA keys are outroducing.
                f"ksk 0 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offval}",
                f"zsk 0 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{offval}",
                # The ECDSAP256SHA256 keys are introducing.
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
                f"zsk 0 {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
            ],
            # Next key event is when the ecdsa256 keys have been propagated.
            "nextev": ipub,
        },
        {
            # Step 2.
            "zone": "step2.algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "ecdsa256",
            "keyprops": [
                # The RSASHA keys are outroducing, but need to stay present
                # until the new algorithm chain of trust has been established.
                # Thus the expected key states of these keys stay the same.
                f"ksk 0 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offval}",
                f"zsk 0 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{offval}",
                # The ECDSAP256SHA256 keys are introducing. The DNSKEY RRset is
                # omnipresent, but the zone signatures are not.
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:hidden offset:{offsets['step2']}",
                f"zsk 0 {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:rumoured offset:{offsets['step2']}",
            ],
            # Next key event is when all zone signatures are signed with the new
            # algorithm.  This is the max-zone-ttl plus zone propagation delay.  But
            # the publication interval has already passed. Also, prevent intermittent
            # false positives on slow platforms by subtracting the time passed between
            # key creation and invoking 'rndc reconfig'.
            "nextev": ipubc - ipub - time_passed,
        },
        {
            # Step 3.
            "zone": "step3.algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "ecdsa256",
            "keyprops": [
                # The DS can be swapped.
                f"ksk 0 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent ds:unretentive offset:{offval}",
                f"zsk 0 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent offset:{offval}",
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:rumoured offset:{offsets['step3']}",
                f"zsk 0 {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step3']}",
            ],
            # Next key event is when the DS becomes OMNIPRESENT. This happens
            # after the retire interval.
            "nextev": iretKSK - time_passed,
        },
        {
            # Step 4.
            "zone": "step4.algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "ecdsa256",
            "keyprops": [
                # The old DS is HIDDEN, we can remove the old algorithm records.
                f"ksk 0 8 2048 goal:hidden dnskey:unretentive krrsig:unretentive ds:hidden offset:{offval}",
                f"zsk 0 8 2048 goal:hidden dnskey:unretentive zrrsig:unretentive offset:{offval}",
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step4']}",
                f"zsk 0 {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step4']}",
            ],
            # Next key event is when the old DNSKEY becomes HIDDEN.
            # This happens after the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
        },
        {
            # Step 5.
            "zone": "step5.algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "ecdsa256",
            "keyprops": [
                # The DNSKEY becomes HIDDEN.
                f"ksk 0 8 2048 goal:hidden dnskey:hidden krrsig:hidden ds:hidden offset:{offval}",
                f"zsk 0 8 2048 goal:hidden dnskey:hidden zrrsig:unretentive offset:{offval}",
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step5']}",
                f"zsk 0 {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step5']}",
            ],
            # Next key event is when the RSASHA signatures become HIDDEN.
            # This happens after the max-zone-ttl plus zone propagation delay
            # minus the time already passed since the UNRETENTIVE state has
            # been reached. Prevent intermittent false positives on slow
            # platforms by subtracting the number of seconds which passed
            # between key creation and invoking 'rndc reconfig'.
            "nextev": iret - iretKSK - keyttlprop - time_passed,
        },
        {
            # Step 6.
            "zone": "step6.algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "ecdsa256",
            "keyprops": [
                # The zone signatures are now HIDDEN.
                f"ksk 0 8 2048 goal:hidden dnskey:hidden krrsig:hidden ds:hidden offset:{offval}",
                f"zsk 0 8 2048 goal:hidden dnskey:hidden zrrsig:hidden offset:{offval}",
                f"ksk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{offsets['step6']}",
                f"zsk 0 {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent offset:{offsets['step6']}",
            ],
            # Next key event is never since we established the policy and the
            # keys have an unlimited lifetime.  Fallback to the default
            # loadkeys interval.
            "nextev": timedelta(hours=1),
        },
    ]
    steps = steps + algo_steps

    # Test algorithm rollover (CSK) (after reconfig).
    isctest.log.info("check algorithm rollover csk (after reconfig)")
    offsets = {}
    offsets["step2"] = -int(ipub.total_seconds())
    offsets["step3"] = -int(iret.total_seconds())
    offsets["step4"] = offsets["step3"] - int(iretKSK.total_seconds())
    offsets["step5"] = offsets["step4"] - int(keyttlprop.total_seconds())
    offsets["step6"] = offsets["step5"] - int(iret.total_seconds())
    algo_steps = [
        {
            # Step 1.
            "zone": "step1.csk-algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "csk-algoroll",
            "keyprops": [
                # The RSASHA keys are outroducing.
                f"csk 0 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offval}",
                # The ECDSAP256SHA256 keys are introducing.
                f"csk 0 {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
            ],
            # Next key event is when the ecdsa256 keys have been propagated.
            "nextev": ipub,
        },
        {
            # Step 2.
            "zone": "step2.csk-algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "csk-algoroll",
            "keyprops": [
                # The RSASHA keys are outroducing, but need to stay present
                # until the new algorithm chain of trust has been established.
                # Thus the expected key states of these keys stay the same.
                f"csk 0 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offval}",
                # The ECDSAP256SHA256 keys are introducing. The DNSKEY RRset is
                # omnipresent, but the zone signatures are not.
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:rumoured ds:hidden offset:{offsets['step2']}",
            ],
            # Next key event is when all zone signatures are signed with the
            # new algorithm.  This is the child publication interval, minus
            # the publication interval has already passed. Also, prevent
            # intermittent false positives on slow platforms by subtracting
            # the time passed between key creation and invoking 'rndc reconfig'.
            "nextev": ipubc - ipub - time_passed,
        },
        {
            # Step 3.
            "zone": "step3.csk-algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "csk-algoroll",
            "keyprops": [
                # The DS can be swapped.
                f"csk 0 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:unretentive offset:{offval}",
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:rumoured offset:{offsets['step3']}",
            ],
            # Next key event is when the DS becomes OMNIPRESENT. This happens
            # after the publication interval of the parent side.
            "nextev": iretKSK - time_passed,
        },
        {
            # Step 4.
            "zone": "step4.csk-algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "csk-algoroll",
            "keyprops": [
                # The old DS is HIDDEN, we can remove the old algorithm records.
                f"csk 0 8 2048 goal:hidden dnskey:unretentive krrsig:unretentive zrrsig:unretentive ds:hidden offset:{offval}",
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step4']}",
            ],
            # Next key event is when the old DNSKEY becomes HIDDEN.
            # This happens after the DNSKEY TTL plus zone propagation delay.
            "nextev": keyttlprop,
        },
        {
            # Step 5.
            "zone": "step5.csk-algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "csk-algoroll",
            "keyprops": [
                # The DNSKEY becomes HIDDEN.
                f"csk 0 8 2048 goal:hidden dnskey:hidden krrsig:hidden zrrsig:unretentive ds:hidden offset:{offval}",
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step5']}",
            ],
            # Next key event is when the RSASHA signatures become HIDDEN.
            # This happens after the max-zone-ttl plus zone propagation delay
            # minus the time already passed since the UNRETENTIVE state has
            # been reached. Prevent intermittent false positives on slow
            # platforms by subtracting the number of seconds which passed
            # between key creation and invoking 'rndc reconfig'.
            "nextev": iret - iretKSK - keyttlprop - time_passed,
        },
        {
            # Step 6.
            "zone": "step6.csk-algorithm-roll.kasp",
            "cdss": cdss,
            "config": algoroll_config,
            "policy": "csk-algoroll",
            "keyprops": [
                # The zone signatures are now HIDDEN.
                f"csk 0 8 2048 goal:hidden dnskey:hidden krrsig:hidden zrrsig:hidden ds:hidden offset:{offval}",
                f"csk 0 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent zrrsig:omnipresent ds:omnipresent offset:{offsets['step6']}",
            ],
            # Next key event is never since we established the policy and the
            # keys have an unlimited lifetime.  Fallback to the default
            # loadkeys interval.
            "nextev": timedelta(hours=1),
        },
    ]
    steps = steps + algo_steps

    for step in steps:
        check_rollover_step(server, step["config"], step["policy"], step)
