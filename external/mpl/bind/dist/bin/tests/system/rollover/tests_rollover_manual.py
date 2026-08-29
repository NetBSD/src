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

from datetime import timedelta

import shutil

from isctest.algorithms import Algorithm
from isctest.kasp import Ipub, Iret, KeyTimingMetadata, private_type_record
from isctest.run import EnvCmd
from isctest.template import NS3, Zone
from rollover.setup import configure_root, configure_tld

import isctest


def setup_zone(zone, ksk_time, ksk_settime, zsk_time, zsk_settime) -> Zone:
    templates = isctest.template.TemplateEngine(".")
    default_algorithm = Algorithm.default()
    keygen = EnvCmd(
        "KEYGEN",
        f"-q -a {default_algorithm.number} -b {default_algorithm.bits} -L 3600",
    )
    signer = EnvCmd("SIGNER", "-S -g")
    settime = EnvCmd("SETTIME", "-s")

    isctest.log.info(f"setup {zone}")
    template = "template.db.j2.manual"
    outfile = f"{zone}.db"

    # Configuration.
    isctest.log.info(f"setup {zone}")
    template = "template.db.j2.manual"
    outfile = f"{zone}.db"
    # Key generation.
    ksk_name = keygen(
        f"-f KSK -P {ksk_time} -A {ksk_time} -L 3600 {zone}", cwd="ns3"
    ).out.strip()
    zsk_name = keygen(
        f"-P {zsk_time} -A {zsk_time} -L 3600 {zone}", cwd="ns3"
    ).out.strip()
    # Key state timing metadata.
    settime(f"{ksk_settime} {ksk_name}", cwd="ns3")
    settime(f"{zsk_settime} {zsk_name}", cwd="ns3")
    # Signing.
    ksk = isctest.kasp.Key(ksk_name, keydir="ns3")
    zsk = isctest.kasp.Key(zsk_name, keydir="ns3")
    dnskeys = [ksk.dnskey, zsk.dnskey]
    privaterrs = [
        private_type_record(zone, ksk),
        private_type_record(zone, zsk),
    ]
    tdata = {
        "fqdn": f"{zone}.",
        "dnskeys": dnskeys,
        "privaterrs": privaterrs,
    }
    templates.render(f"ns3/{outfile}", tdata, template=f"ns3/{template}")
    signer(f"-P -x -O raw -o {zone} -f {outfile}.signed {outfile}", cwd="ns3")

    return Zone(zone, NS3)


def bootstrap():
    zones = []

    zone = "manual-rollover.kasp"
    when = "now-7d"
    ksk_settime = f"-g OMNIPRESENT -k OMNIPRESENT {when} -r OMNIPRESENT {when} -d OMNIPRESENT {when}"
    zsk_settime = f"-g OMNIPRESENT -k OMNIPRESENT {when} -z OMNIPRESENT {when}"
    zones.append(setup_zone(zone, when, ksk_settime, when, zsk_settime))

    # Copy files to ns4.
    shutil.copyfile(f"ns3/{zone}.db", f"ns4/{zone}.db")
    shutil.copyfile(f"ns3/{zone}.db.signed", f"ns4/{zone}.db.signed")

    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    for key in keys:
        keyfile = f"{key.name}.key"
        privatefile = f"{key.name}.private"
        statefile = f"{key.name}.state"
        shutil.copyfile(f"ns3/{keyfile}", f"ns4/{keyfile}")
        shutil.copyfile(f"ns3/{privatefile}", f"ns4/{privatefile}")
        shutil.copyfile(f"ns3/{statefile}", f"ns4/{statefile}")

    zone = "manual-rollover-zrrsig-rumoured.kasp"
    then = "now-2h"
    ksk_settime = f"-g OMNIPRESENT -k OMNIPRESENT {when} -r OMNIPRESENT {when} -d OMNIPRESENT {when}"
    zsk_settime = f"-g OMNIPRESENT -k OMNIPRESENT {then} -z RUMOURED {then}"
    zones.append(setup_zone(zone, when, ksk_settime, then, zsk_settime))

    # Chain of trust.
    data = {
        "tlds": [],
        "trust_anchors": [],
    }
    tld = configure_tld("kasp", zones)
    data["tlds"].append("kasp")

    ta = configure_root([tld])
    data["trust_anchors"].append(ta)

    return data


CONFIG = {
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

POLICY = "manual-rollover"


def retire_key(expected, kid, now):
    ksk = expected[kid].key.is_ksk()
    zsk = expected[kid].key.is_zsk()
    expected[kid].timing["Retired"] = now + Ipub(CONFIG)
    expected[kid].timing["Removed"] = expected[kid].timing["Retired"] + Iret(
        CONFIG, zsk=zsk, ksk=ksk
    )


def run_checks(server, zone, keys, expected):
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(server, zone, keys, policy=POLICY)
    isctest.kasp.check_apex(server, zone, ksks, zsks)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks)


def test_rollover_manual(servers):
    ns3 = servers["ns3"]
    ns4 = servers["ns4"]
    ns = [ns4, ns3]

    default_algorithm = Algorithm.default()
    ttl = int(CONFIG["dnskey-ttl"].total_seconds())
    zone = "manual-rollover.kasp"

    key_properties = [
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]

    expected = isctest.kasp.policy_to_properties(ttl, key_properties)

    for server in ns:
        isctest.kasp.wait_keymgr_done(server, zone)
        isctest.kasp.check_dnssec_verify(server, zone)

        keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
        isctest.kasp.check_keys(zone, keys, expected)

        offset = -timedelta(days=7)
        for kp in expected:
            kp.set_expected_keytimes(CONFIG, offset=offset)

        run_checks(server, zone, keys, expected)

    # Schedule KSK rollover in six months.
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    assert len(ksks) == 1
    ksk = ksks[0]
    startroll = expected[0].timing["Active"] + timedelta(days=30 * 6)
    expected[0].timing["Retired"] = startroll + Ipub(CONFIG)
    expected[0].timing["Removed"] = expected[0].timing["Retired"] + Iret(
        CONFIG, zsk=False, ksk=True
    )

    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -rollover -key {ksk.tag} -when {startroll} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    run_checks(ns3, zone, keys, expected)

    # Schedule KSK rollover now.
    ksknow = KeyTimingMetadata.now()
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -rollover -key {ksk.tag} -when {ksknow} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(ns3, zone)

    key_properties = [
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
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
        kp.set_expected_keytimes(CONFIG, offset=off)

    retire_key(expected, 0, ksknow)

    run_checks(ns3, zone, keys, expected)

    # Schedule ZSK rollover now.
    assert len(zsks) == 1
    zsk = zsks[0]
    zsknow = KeyTimingMetadata.now()
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -rollover -key {zsk.tag} -when {zsknow} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(ns3, zone)

    key_properties = [
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:hidden dnskey:omnipresent zrrsig:omnipresent",
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:rumoured zrrsig:hidden",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    expected[0].metadata["Successor"] = expected[1].key.tag
    expected[1].metadata["Predecessor"] = expected[0].key.tag
    expected[2].metadata["Successor"] = expected[3].key.tag
    expected[3].metadata["Predecessor"] = expected[2].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)

    for kp in expected:
        off = offset
        if "Predecessor" in kp.metadata:
            off = 0
        kp.set_expected_keytimes(CONFIG, offset=off)

    retire_key(expected, 0, ksknow)
    retire_key(expected, 2, zsknow)

    run_checks(ns3, zone, keys, expected)

    # Try to schedule a ZSK rollover for an inactive key (should fail).
    zsk = expected[3].key
    response = ns3.rndc(f"dnssec -rollover -key {zsk.tag} {zone}")
    assert "key is not actively signing" in response.out

    # Copy files to ns4, check if the state is the same after all manual rollovers issued at ns3.
    for key in keys:
        keyfile = f"{key.name}.key"
        privatefile = f"{key.name}.private"
        statefile = f"{key.name}.state"
        shutil.copyfile(f"{ns3.identifier}/{keyfile}", f"{ns4.identifier}/{keyfile}")
        shutil.copyfile(
            f"{ns3.identifier}/{privatefile}", f"{ns4.identifier}/{privatefile}"
        )
        shutil.copyfile(
            f"{ns3.identifier}/{statefile}", f"{ns4.identifier}/{statefile}"
        )

    with ns4.watch_log_from_here() as watcher:
        ns4.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(ns4, zone)

    keys = isctest.kasp.keydir_to_keylist(zone, ns4.identifier)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_keyrelationships(keys, expected)

    run_checks(ns4, zone, keys, expected)


def test_rollover_manual_zrrsig_rumoured(ns3, default_algorithm):
    ttl = int(CONFIG["dnskey-ttl"].total_seconds())
    zone = "manual-rollover-zrrsig-rumoured.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    isctest.kasp.check_dnssec_verify(ns3, zone)

    koffset = -int(timedelta(days=7).total_seconds())
    zoffset = -int(timedelta(hours=2).total_seconds())
    key_properties = [
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{koffset}",
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent zrrsig:rumoured offset:{zoffset}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    for kp in expected:
        kp.set_expected_keytimes(CONFIG)

    run_checks(ns3, zone, keys, expected)

    # Schedule ZSK rollover now.
    assert len(zsks) == 1
    zsk = zsks[0]
    now = KeyTimingMetadata.now()
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -rollover -key {zsk.tag} -when {now} {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    isctest.kasp.check_dnssec_verify(ns3, zone)

    key_properties = [
        f"ksk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent offset:{koffset}",
        # Predecessor DNSKEY must stay until successor ZSK is fully omnipresent.
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:hidden dnskey:omnipresent zrrsig:rumoured offset:{zoffset}",
        f"zsk unlimited {default_algorithm.number} {default_algorithm.bits} goal:omnipresent dnskey:rumoured zrrsig:hidden offset:0",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)

    isctest.kasp.check_keys(zone, keys, expected)

    expected[1].metadata["Successor"] = expected[2].key.tag
    expected[2].metadata["Predecessor"] = expected[1].key.tag
    isctest.kasp.check_keyrelationships(keys, expected)
