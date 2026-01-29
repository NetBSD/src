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
import os

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.update

import isctest
from isctest.kasp import Iret
from isctest.run import EnvCmd
from rollover.common import (
    pytestmark,
    alg,
    size,
)
from rollover.setup import fake_lifetime, render_and_sign_zone


def bootstrap():
    templates = isctest.template.TemplateEngine(".")

    # Multi-signer zones.
    keygen = EnvCmd("KEYGEN", "-a ECDSA256 -L 3600")
    settime = EnvCmd("SETTIME", "-s")

    # Model 2.
    zonename = "multisigner-model2.kasp"
    isctest.log.info(f"setup {zonename}")
    # Key generation.
    ksk_name = keygen(f"-M 32768:65535 -f KSK {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"-M 32768:65535 {zonename}", cwd="ns3").out.strip()
    # Signing.
    dnskeys = []
    for key_name in [ksk_name, zsk_name]:
        key = isctest.kasp.Key(key_name, keydir="ns3")
        dnskeys.append(key.dnskey)
    # Import a ZSK of another provider into the DNSKEY RRset.
    zsk_extra = keygen(f"-M 0:32767 {zonename}").out.strip()
    key = isctest.kasp.Key(zsk_extra)
    dnskeys.append(key.dnskey)
    # Render zone file.
    outfile = f"{zonename}.db"
    templates = isctest.template.TemplateEngine(".")
    template = "template.db.j2.manual"
    tdata = {
        "fqdn": f"{zonename}.",
        "dnskeys": dnskeys,
        "privaterrs": [],
    }
    templates.render(f"ns3/{outfile}", tdata, template=f"ns3/{template}")

    # We are changing an existing single-signed zone to multi-signed
    # zone where the key tags do not match the dnssec-policy key tag range
    zonename = "single-to-multisigner.kasp"
    isctest.log.info(f"setup {zonename}")
    # Timing metadata.
    TpubN = "now-7d"
    TsbmN = "now-8635mi"  # T - 1d5m
    keytimes = f"-P {TpubN} -A {TpubN}"
    cdstimes = f"-P sync {TsbmN}"
    # Key generation.
    ksk_name = keygen(
        f"-M 0:32767 -f KSK {keytimes} {cdstimes} {zonename}", cwd="ns3"
    ).out.strip()
    zsk_name = keygen(f"-M 0:32767 {keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -d OMNIPRESENT {TpubN} -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} {zsk_name}",
        cwd="ns3",
    )
    # Signing.
    fake_lifetime(ksk_name, 0)
    fake_lifetime(zsk_name, 0)
    render_and_sign_zone(zonename, [ksk_name, zsk_name])

    return {}


def test_rollover_multisigner(ns3, alg, size):
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

        return isctest.run.cmd(keygen_command).out

    zone = "multisigner-model2.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    isctest.kasp.check_dnssec_verify(ns3, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden tag-range:32768-65535",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:rumoured tag-range:32768-65535",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)

    newprops = [f"zsk unlimited {alg} {size} tag-range:0-32767"]
    expected2 = isctest.kasp.policy_to_properties(ttl, newprops)
    expected2[0].private = False  # noqa
    expected2[0].legacy = True  # noqa
    expected = expected + expected2

    ownkeys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    extkeys = isctest.kasp.keydir_to_keylist(zone)
    keys = ownkeys + extkeys
    ksks = [k for k in ownkeys if k.is_ksk()]
    zsks = [k for k in ownkeys if not k.is_ksk()]
    zsks = zsks + extkeys

    isctest.kasp.check_keys(zone, keys, expected)
    for kp in expected:
        kp.set_expected_keytimes(config)
    isctest.kasp.check_keytimes(keys, expected)
    isctest.kasp.check_dnssecstatus(ns3, zone, keys, policy=policy)
    isctest.kasp.check_apex(ns3, zone, ksks, zsks)
    isctest.kasp.check_subdomain(ns3, zone, ksks, zsks)

    # Update zone with ZSK from another provider for zone.
    out = keygen(zone)
    newkeys = isctest.kasp.keystr_to_keylist(out)
    newprops = [f"zsk unlimited {alg} {size} tag-range:0-32767"]
    expected2 = isctest.kasp.policy_to_properties(ttl, newprops)
    expected2[0].private = False  # noqa
    expected2[0].legacy = True  # noqa
    expected = expected + expected2

    dnskey = newkeys[0].dnskey

    update_msg = dns.update.UpdateMessage(zone)
    update_msg.add(dnskey.name, dnskey.ttl, dnskey[0])
    ns3.nsupdate(update_msg)

    isctest.kasp.check_dnssec_verify(ns3, zone)

    keys = keys + newkeys
    zsks = zsks + newkeys
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_apex(ns3, zone, ksks, zsks)
    isctest.kasp.check_subdomain(ns3, zone, ksks, zsks)

    # Remove ZSKs from the other providers for zone.
    dnskey2 = extkeys[0].dnskey
    update_msg = dns.update.UpdateMessage(zone)
    update_msg.delete(dnskey.name, dnskey[0])
    update_msg.delete(dnskey2.name, dnskey2[0])
    ns3.nsupdate(update_msg)

    isctest.kasp.check_dnssec_verify(ns3, zone)

    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = ownkeys
    ksks = [k for k in ownkeys if k.is_ksk()]
    zsks = [k for k in ownkeys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_apex(ns3, zone, ksks, zsks)
    isctest.kasp.check_subdomain(ns3, zone, ksks, zsks)

    # A zone transitioning from single-signed to multi-signed. We should have
    # the old omnipresent keys outside of the desired key range and the new
    # keys in the desired key range.
    zone = "single-to-multisigner.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    isctest.kasp.check_dnssec_verify(ns3, zone)

    key_properties = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden tag-range:32768-65535",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden tag-range:32768-65535",
        f"ksk unlimited {alg} {size} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent tag-range:0-32767 offset:{offval}",
        f"zsk unlimited {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent tag-range:0-32767 offset:{offval}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
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
    isctest.kasp.check_dnssecstatus(ns3, zone, keys, policy=policy)
    isctest.kasp.check_apex(ns3, zone, ksks, zsks)
    isctest.kasp.check_subdomain(ns3, zone, ksks, zsks)
