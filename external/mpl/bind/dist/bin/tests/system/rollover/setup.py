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

import shutil
from typing import List

import isctest
from isctest.kasp import private_type_record
from isctest.template import Nameserver, TrustAnchor, Zone
from isctest.run import EnvCmd
from rollover.common import default_algorithm


def configure_tld(zonename: str, delegations: List[Zone]) -> Zone:
    templates = isctest.template.TemplateEngine(".")
    alg = default_algorithm()
    keygen = EnvCmd("KEYGEN", f"-q -a {alg.number} -b {alg.bits} -L 3600")
    signer = EnvCmd("SIGNER", "-S -g")

    isctest.log.info(f"create {zonename} zone with delegations and sign")

    for zone in delegations:
        try:
            shutil.copy(f"{zone.ns.name}/dsset-{zone.name}.", "ns2/")
        except FileNotFoundError:
            # Some delegations are unsigned.
            pass

    ksk_name = keygen(f"-f KSK {zonename}", cwd="ns2").out.strip()
    zsk_name = keygen(f"{zonename}", cwd="ns2").out.strip()
    ksk = isctest.kasp.Key(ksk_name, keydir="ns2")
    zsk = isctest.kasp.Key(zsk_name, keydir="ns2")
    dnskeys = [ksk.dnskey, zsk.dnskey]

    template = "template.db.j2.manual"
    outfile = f"{zonename}.db"
    tdata = {
        "fqdn": f"{zonename}.",
        "delegations": delegations,
        "dnskeys": dnskeys,
    }
    templates.render(f"ns2/{outfile}", tdata, template=f"ns2/{template}")
    signer(f"-P -x -O full -o {zonename} -f {outfile}.signed {outfile}", cwd="ns2")

    return Zone(zonename, f"{outfile}.signed", Nameserver("ns2", "10.53.0.2"))


def configure_root(delegations: List[Zone]) -> TrustAnchor:
    templates = isctest.template.TemplateEngine(".")
    alg = default_algorithm()
    keygen = EnvCmd("KEYGEN", f"-q -a {alg.number} -b {alg.bits} -L 3600")
    signer = EnvCmd("SIGNER", "-S -g")

    zonename = "."
    isctest.log.info("create root zone with delegations and sign")

    for zone in delegations:
        shutil.copy(f"{zone.ns.name}/dsset-{zone.name}.", "ns1/")

    ksk_name = keygen(f"-f KSK {zonename}", cwd="ns1").out.strip()
    zsk_name = keygen(f"{zonename}", cwd="ns1").out.strip()
    ksk = isctest.kasp.Key(ksk_name, keydir="ns1")
    zsk = isctest.kasp.Key(zsk_name, keydir="ns1")
    dnskeys = [ksk.dnskey, zsk.dnskey]

    template = "root.db.j2.manual"
    infile = "root.db.in"
    outfile = "root.db.signed"
    tdata = {
        "fdqn": f"{zonename}.",
        "delegations": delegations,
        "dnskeys": dnskeys,
    }
    templates.render(f"ns1/{infile}", tdata, template=f"ns1/{template}")
    signer(f"-P -x -O full -o {zonename} -f {outfile} {infile}", cwd="ns1")

    return ksk.into_ta("static-ds")


def fake_lifetime(key: str, lifetime: int):
    """
    Fake lifetime of key.
    """
    with open(f"ns3/{key}.state", "a", encoding="utf-8") as statefile:
        statefile.write(f"Lifetime: {lifetime}\n")


def set_key_relationship(key1: str, key2: str):
    """
    Set in the key state files the Predecessor/Successor fields.
    """
    predecessor = isctest.kasp.Key(key1, keydir="ns3")
    successor = isctest.kasp.Key(key2, keydir="ns3")

    with open(f"ns3/{key1}.state", "a", encoding="utf-8") as statefile:
        statefile.write(f"Successor: {successor.tag}\n")

    with open(f"ns3/{key2}.state", "a", encoding="utf-8") as statefile:
        statefile.write(f"Predecessor: {predecessor.tag}\n")


def render_and_sign_zone(
    zonename: str, keys: List[str], signing: bool = True, extra_options: str = ""
):
    dnskeys = []
    privaterrs = []
    for key_name in keys:
        key = isctest.kasp.Key(key_name, keydir="ns3")
        privaterr = private_type_record(zonename, key)
        dnskeys.append(key.dnskey)
        privaterrs.append(privaterr)

    outfile = f"{zonename}.db"
    templates = isctest.template.TemplateEngine(".")
    template = "template.db.j2.manual"
    tdata = {
        "fqdn": f"{zonename}.",
        "dnskeys": dnskeys,
        "privaterrs": privaterrs,
    }
    templates.render(f"ns3/{outfile}", tdata, template=f"ns3/{template}")

    if signing:
        signer = EnvCmd("SIGNER", "-S -g -x -s now-1h -e now+2w -O raw")
        signer(
            f"{extra_options} -o {zonename} -f {outfile}.signed {outfile}", cwd="ns3"
        )


def configure_algo_csk(tld: str, policy: str, reconfig: bool = False) -> List[Zone]:
    # The zones at csk-algorithm-roll.$tld represent the various steps
    # of a CSK algorithm rollover.
    zones = []
    zone = f"csk-algorithm-roll.{tld}"
    keygen = EnvCmd("KEYGEN", f"-k {policy}")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # Introduce the first key. This will immediately be active.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    TactN = "now-7d"
    TsbmN = "now-161h"
    csktimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    csk_name = keygen(f"-l csk1.conf {csktimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options="-z")

    if reconfig:
        # Step 2:
        # After the publication interval has passed the DNSKEY is OMNIPRESENT.
        zonename = f"step2.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the new algorithm keys have been introduced is 3 hours.
        TpubN1 = "now-3h"
        csktimes = f"-P {TactN} -A {TactN} -P sync {TsbmN} -I now"
        newtimes = f"-P {TpubN1} -A {TpubN1}"
        # Key generation.
        csk1_name = keygen(f"-l csk1.conf {csktimes} {zonename}", cwd="ns3").out.strip()
        csk2_name = keygen(f"-l csk2.conf {newtimes} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k RUMOURED {TpubN1} -r RUMOURED {TpubN1} -z RUMOURED {TpubN1} -d HIDDEN {TpubN1} {csk2_name}",
            cwd="ns3",
        )
        # Signing.
        render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options="-z")

        # Step 3:
        # The zone signatures are also OMNIPRESENT.
        zonename = f"step3.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the new algorithm keys have been introduced is 7 hours.
        TpubN1 = "now-7h"
        TsbmN1 = "now"
        csktimes = f"-P {TactN} -A {TactN}  -P sync {TsbmN} -I {TsbmN1}"
        newtimes = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        # Key generation.
        csk1_name = keygen(f"-l csk1.conf {csktimes} {zonename}", cwd="ns3").out.strip()
        csk2_name = keygen(f"-l csk2.conf {newtimes} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -z RUMOURED {TpubN1} -d HIDDEN {TpubN1} {csk2_name}",
            cwd="ns3",
        )
        # Signing.
        render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options="-z")

        # Step 4:
        # The DS is swapped and can become OMNIPRESENT.
        zonename = f"step4.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the DS has been swapped is 3 hours.
        TpubN1 = "now-10h"
        TsbmN1 = "now-3h"
        csktimes = f"-P {TactN} -A {TactN}  -P sync {TsbmN} -I {TsbmN1}"
        newtimes = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        # Key generation.
        csk1_name = keygen(f"-l csk1.conf {csktimes} {zonename}", cwd="ns3").out.strip()
        csk2_name = keygen(f"-l csk2.conf {newtimes} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TsbmN1} -d UNRETENTIVE {TsbmN1} -D ds {TsbmN1} {csk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -z OMNIPRESENT {TsbmN1} -d RUMOURED {TsbmN1} -P ds {TsbmN1} {csk2_name}",
            cwd="ns3",
        )
        # Signing.
        render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options="-z")

        # Step 5:
        # The DNSKEY is removed long enough to be HIDDEN.
        zonename = f"step5.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the DNSKEY has been removed is 2 hours.
        TpubN1 = "now-12h"
        TsbmN1 = "now-5h"
        csktimes = f"-P {TactN} -A {TactN} -P sync {TsbmN} -I {TsbmN1}"
        newtimes = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        # Key generation.
        csk1_name = keygen(f"-l csk1.conf {csktimes} {zonename}", cwd="ns3").out.strip()
        csk2_name = keygen(f"-l csk2.conf {newtimes} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k UNRETENTIVE {TactN} -r UNRETENTIVE {TactN} -z UNRETENTIVE {TsbmN1} -d HIDDEN {TsbmN1} {csk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -z OMNIPRESENT {TsbmN1} -d OMNIPRESENT {TsbmN1} {csk2_name}",
            cwd="ns3",
        )
        # Signing.
        render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options="-z")

        # Step 6:
        # The RRSIGs have been removed long enough to be HIDDEN.
        zonename = f"step6.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # Additional time passed: 7h.
        TpubN1 = "now-19h"
        TsbmN1 = "now-12h"
        csktimes = f"-P {TactN}  -A {TactN}  -P sync {TsbmN} -I {TsbmN1}"
        newtimes = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        # Key generation.
        csk1_name = keygen(f"-l csk1.conf {csktimes} {zonename}", cwd="ns3").out.strip()
        csk2_name = keygen(f"-l csk2.conf {newtimes} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k HIDDEN {TactN} -r UNRETENTIVE {TactN} -z UNRETENTIVE {TactN} -d HIDDEN {TsbmN1} {csk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -z OMNIPRESENT {TsbmN1} -d OMNIPRESENT {TsbmN1} {csk2_name}",
            cwd="ns3",
        )
        # Signing.
        render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options="-z")

    return zones


def configure_algo_ksk_zsk(tld: str, reconfig: bool = False) -> List[Zone]:
    # The zones at algorithm-roll.$tld represent the various steps of a ZSK/KSK
    # algorithm rollover.
    zones = []
    zone = f"algorithm-roll.{tld}"
    keygen = EnvCmd("KEYGEN", "-L 3600")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # Introduce the first key. This will immediately be active.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    TactN = "now-7d"
    TsbmN = "now-161h"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    ksk_name = keygen(
        f"-a RSASHA256 -f KSK {keytimes} {zonename}", cwd="ns3"
    ).out.strip()
    zsk_name = keygen(f"-a RSASHA256 {keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk_name])

    if reconfig:
        # Step 2:
        # After the publication interval has passed the DNSKEY is OMNIPRESENT.
        zonename = f"step2.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the new algorithm keys have been introduced is 3 hours.
        # Tsbm(N+1) = TpubN1 + Ipub = now + TTLsig + Dprp = now - 3h + 6h + 1h = now + 4h
        TpubN1 = "now-3h"
        TsbmN1 = "now+4h"
        ksk1times = f"-P {TactN}  -A {TactN}  -P sync {TsbmN} -I {TsbmN1}"
        zsk1times = f"-P {TactN}  -A {TactN}  -I {TsbmN1}"
        ksk2times = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        zsk2times = f"-P {TpubN1} -A {TpubN1}"
        # Key generation.
        ksk1_name = keygen(
            f"-a RSASHA256 -f KSK {ksk1times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk1_name = keygen(
            f"-a RSASHA256 {zsk1times} {zonename}", cwd="ns3"
        ).out.strip()
        ksk2_name = keygen(
            f"-a ECDSA256 -f KSK {ksk2times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk2_name = keygen(f"-a ECDSA256 {zsk2times} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k RUMOURED {TpubN1} -r RUMOURED {TpubN1} -d HIDDEN {TpubN1} {ksk2_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k RUMOURED {TpubN1} -z RUMOURED {TpubN1} {zsk2_name}",
            cwd="ns3",
        )
        # Signing.
        fake_lifetime(ksk1_name, 0)
        fake_lifetime(zsk1_name, 0)
        render_and_sign_zone(zonename, [ksk1_name, zsk1_name, ksk2_name, zsk2_name])

        # Step 3:
        # The zone signatures are also OMNIPRESENT.
        zonename = f"step3.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the new algorithm keys have been introduced is 7 hours.
        TpubN1 = "now-7h"
        TsbmN1 = "now"
        ksk1times = f"-P {TactN} -A {TactN} -P sync {TsbmN} -I {TsbmN1}"
        zsk1times = f"-P {TactN} -A {TactN} -I {TsbmN1}"
        ksk2times = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        zsk2times = f"-P {TpubN1} -A {TpubN1}"
        # Key generation.
        ksk1_name = keygen(
            f"-a RSASHA256 -f KSK {ksk1times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk1_name = keygen(
            f"-a RSASHA256 {zsk1times} {zonename}", cwd="ns3"
        ).out.strip()
        ksk2_name = keygen(
            f"-a ECDSA256 -f KSK {ksk2times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk2_name = keygen(f"-a ECDSA256 {zsk2times} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -d HIDDEN {TpubN1} {ksk2_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -z RUMOURED {TpubN1} {zsk2_name}",
            cwd="ns3",
        )
        # Signing.
        fake_lifetime(ksk1_name, 0)
        fake_lifetime(zsk1_name, 0)
        render_and_sign_zone(zonename, [ksk1_name, zsk1_name, ksk2_name, zsk2_name])

        # Step 4:
        # The DS is swapped and can become OMNIPRESENT.
        zonename = f"step4.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the DS has been swapped is 3 hours.
        TpubN1 = "now-10h"
        TsbmN1 = "now-3h"
        ksk1times = f"-P {TactN} -A {TactN} -P sync {TsbmN} -I {TsbmN1}"
        zsk1times = f"-P {TactN} -A {TactN} -I {TsbmN1}"
        ksk2times = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        zsk2times = f"-P {TpubN1} -A {TpubN1}"
        # Key generation.
        ksk1_name = keygen(
            f"-a RSASHA256 -f KSK {ksk1times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk1_name = keygen(
            f"-a RSASHA256 {zsk1times} {zonename}", cwd="ns3"
        ).out.strip()
        ksk2_name = keygen(
            f"-a ECDSA256 -f KSK {ksk2times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk2_name = keygen(f"-a ECDSA256 {zsk2times} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d UNRETENTIVE {TsbmN1} -D ds {TsbmN1} {ksk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g HIDDEN -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -d RUMOURED {TsbmN1} -P ds {TsbmN1} {ksk2_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -z RUMOURED {TpubN1} {zsk2_name}",
            cwd="ns3",
        )
        # Signing.
        fake_lifetime(ksk1_name, 0)
        fake_lifetime(zsk1_name, 0)
        render_and_sign_zone(zonename, [ksk1_name, zsk1_name, ksk2_name, zsk2_name])

        # Step 5:
        # The DNSKEY is removed long enough to be HIDDEN.
        zonename = f"step5.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # The time passed since the DNSKEY has been removed is 2 hours.
        TpubN1 = "now-12h"
        TsbmN1 = "now-5h"
        ksk1times = f"-P {TactN} -A {TactN} -P sync {TsbmN} -I {TsbmN1}"
        zsk1times = f"-P {TactN} -A {TactN} -I {TsbmN1}"
        ksk2times = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        zsk2times = f"-P {TpubN1} -A {TpubN1}"
        # Key generation.
        ksk1_name = keygen(
            f"-a RSASHA256 -f KSK {ksk1times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk1_name = keygen(
            f"-a RSASHA256 {zsk1times} {zonename}", cwd="ns3"
        ).out.strip()
        ksk2_name = keygen(
            f"-a ECDSA256 -f KSK {ksk2times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk2_name = keygen(f"-a ECDSA256 {zsk2times} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k UNRETENTIVE {TsbmN1} -r UNRETENTIVE {TsbmN1} -d HIDDEN {TsbmN1} {ksk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g HIDDEN -k UNRETENTIVE {TsbmN1} -z UNRETENTIVE {TsbmN1} {zsk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -d OMNIPRESENT {TsbmN1} {ksk2_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -z RUMOURED {TpubN1} {zsk2_name}",
            cwd="ns3",
        )
        # Signing.
        fake_lifetime(ksk1_name, 0)
        fake_lifetime(zsk1_name, 0)
        render_and_sign_zone(zonename, [ksk1_name, zsk1_name, ksk2_name, zsk2_name])

        # Step 6:
        # The RRSIGs have been removed long enough to be HIDDEN.
        zonename = f"step6.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # Additional time passed: 7h.
        TpubN1 = "now-19h"
        TsbmN1 = "now-12h"
        ksk1times = f"-P {TactN} -A {TactN} -P sync {TsbmN} -I {TsbmN1}"
        zsk1times = f"-P {TactN} -A {TactN} -I {TsbmN1}"
        ksk2times = f"-P {TpubN1} -A {TpubN1} -P sync {TsbmN1}"
        zsk2times = f"-P {TpubN1} -A {TpubN1}"
        ksk1_name = keygen(
            f"-a RSASHA256 -f KSK {ksk1times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk1_name = keygen(
            f"-a RSASHA256 {zsk1times} {zonename}", cwd="ns3"
        ).out.strip()
        ksk2_name = keygen(
            f"-a ECDSA256 -f KSK {ksk2times} {zonename}", cwd="ns3"
        ).out.strip()
        zsk2_name = keygen(f"-a ECDSA256 {zsk2times} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g HIDDEN -k HIDDEN {TsbmN1} -r UNRETENTIVE {TsbmN1} -d HIDDEN {TsbmN1} {ksk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g HIDDEN -k HIDDEN {TsbmN1} -z UNRETENTIVE {TsbmN1} {zsk1_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -r OMNIPRESENT {TpubN1} -d OMNIPRESENT {TsbmN1} {ksk2_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN1} -z RUMOURED {TpubN1} {zsk2_name}",
            cwd="ns3",
        )
        # Signing.
        fake_lifetime(ksk1_name, 0)
        fake_lifetime(zsk1_name, 0)
        render_and_sign_zone(zonename, [ksk1_name, zsk1_name, ksk2_name, zsk2_name])

    return zones


def configure_cskroll1(tld: str, policy: str) -> List[Zone]:
    # The zones at csk-roll1.$tld represent the various steps of a CSK rollover
    # (which is essentially a ZSK Pre-Publication / KSK Double-KSK rollover).
    zones = []
    zone = f"csk-roll1.{tld}"
    cds = "cdnskey,cds:sha384"
    keygen = EnvCmd("KEYGEN", f"-k {policy} -l kasp.conf")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # Introduce the first key. This will immediately be active.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    TactN = "now-7d"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options=f"-z -G {cds}")

    # Step 2:
    # It is time to introduce the new CSK.
    zonename = f"step2.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # KSK: Tpub(N+1) <= Tact(N) + Lksk - IpubC
    # ZSK: Tpub(N+1) <= Tact(N) + Lzsk - Ipub
    # IpubC = DprpC + TTLkey (+publish-safety)
    # Ipub  = IpubC
    # Lcsk = Lksk = Lzsk
    #
    # Lcsk:           6mo (186d, 4464h)
    # Dreg:           N/A
    # DprpC:          1h
    # TTLkey:         1h
    # publish-safety: 1h
    # Ipub:           3h
    #
    # Tact(N) = now - Lcsk + Ipub = now - 186d + 3h
    #         = now - 4464h + 3h  = now - 4461h
    TactN = "now-4461h"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options=f"-z -G {cds}")

    # Step 3:
    # It is time to submit the DS and to roll signatures.
    zonename = f"step3.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    #
    # Tsbm(N+1) >= Trdy(N+1)
    # KSK: Tact(N+1) = Tsbm(N+1)
    # ZSK: Tact(N+1) = Tpub(N+1) + Ipub = Tsbm(N+1)
    # KSK: Iret  = DprpP + TTLds (+retire-safety)
    # ZSK: IretZ = Dsgn + Dprp + TTLsig (+retire-safety)
    #
    # Lcsk:           186d
    # Dprp:           1h
    # DprpP:          1h
    # Dreg:           N/A
    # Dsgn:           25d
    # TTLds:          1h
    # TTLsig:         1d
    # retire-safety:  2h
    # Iret:           4h
    # IretZ:          26d3h
    # Ipub:           3h
    #
    # Tpub(N)   = now - Lcsk = now - 186d
    # Tact(N)   = now - Lcsk + Dprp + TTLsig = now - 4439h
    # Tret(N)   = now
    # Trem(N)   = now + IretZ = now + 26d3h = now + 627h
    # Tpub(N+1) = now - Ipub = now - 3h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now + Lcsk = now + 186d = now + 186d
    # Trem(N+1) = now + Lcsk + IretZ = now + 186d + 26d3h =
    #           = now + 5091h
    TpubN = "now-186d"
    TactN = "now-4439h"
    TretN = "now"
    TremN = "now+627h"
    TpubN1 = "now-3h"
    TactN1 = TretN
    TretN1 = "now+186d"
    TremN1 = "now+5091h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k RUMOURED {TpubN1} -r RUMOURED {TpubN1} -z HIDDEN {TpubN1} -d HIDDEN {TpubN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 4:
    # Some time later all the ZRRSIG records should be from the new CSK, and the
    # DS should be swapped.  The ZRRSIG records are all replaced after IretZ
    # (which is 26d3h).  The DS is swapped after Iret (which is 4h).
    # In other words, the DS is swapped before all zone signatures are replaced.
    zonename = f"step4.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # Trem(N)    = Tret(N) - Iret + IretZ
    # now       = Tsbm(N+1) + Iret
    #
    # Lcsk:   186d
    # Iret:   4h
    # IretZ:  26d3h
    #
    # Tpub(N)   = now - Iret - Lcsk = now - 4h - 186d = now - 4468h
    # Tret(N)   = now - Iret = now - 4h = now - 4h
    # Trem(N)   = now - Iret + IretZ = now - 4h + 26d3h
    #           = now + 623h
    # Tpub(N+1) = now - Iret - IpubC = now - 4h - 3h = now - 7h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now - Iret + Lcsk = now - 4h + 186d = now + 4460h
    # Trem(N+1) = now - Iret + Lcsk + IretZ = now - 4h + 186d + 26d3h
    #           = now + 5087h
    TpubN = "now-4468h"
    TactN = "now-4443h"
    TretN = "now-4h"
    TremN = "now+623h"
    TpubN1 = "now-7h"
    TactN1 = TretN
    TretN1 = "now+4460h"
    TremN1 = "now+5087h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z UNRETENTIVE {TactN1} -d UNRETENTIVE {TactN1} -D ds {TactN1} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z RUMOURED {TactN1} -d RUMOURED {TactN1} -P ds {TactN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 5:
    # After the DS is swapped in step 4, also the KRRSIG records can be removed.
    # At this time these have all become hidden.
    zonename = f"step5.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract DNSKEY TTL plus zone propagation delay from all the times (2h).
    TpubN = "now-4470h"
    TactN = "now-4445h"
    TretN = "now-6h"
    TremN = "now+621h"
    TpubN1 = "now-9h"
    TactN1 = TretN
    TretN1 = "now+4458h"
    TremN1 = "now+5085h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r UNRETENTIVE now-2h -z UNRETENTIVE {TactN1} -d HIDDEN now-2h {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z RUMOURED {TactN1} -d OMNIPRESENT now-2h {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 6:
    # After the retire interval has passed the predecessor DNSKEY can be
    # removed from the zone.
    zonename = f"step6.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # Trem(N) = Tret(N) + IretZ
    # Tret(N) = Tact(N) + Lcsk
    #
    # Lcsk:   186d
    # Iret:   4h
    # IretZ:  26d3h
    #
    # Tpub(N)   = now - IretZ - Lcsk = now - 627h - 186d
    #           = now - 627h - 4464h = now - 5091h
    # Tact(N)   = now - 627h - 186d
    # Tret(N)   = now - IretZ = now - 627h
    # Trem(N)   = now
    # Tpub(N+1) = now - IretZ - Ipub = now - 627h - 3h = now - 630h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now - IretZ + Lcsk = now - 627h + 186d = now + 3837h
    # Trem(N+1) = now + Lcsk = now + 186d
    TpubN = "now-5091h"
    TactN = "now-5066h"
    TretN = "now-627h"
    TremN = "now"
    TpubN1 = "now-630h"
    TactN1 = TretN
    TretN1 = "now+3837h"
    TremN1 = "now+186d"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r HIDDEN {TremN} -z UNRETENTIVE {TactN1} -d HIDDEN {TremN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z RUMOURED {TactN1} -d OMNIPRESENT {TremN} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 7:
    # Some time later the predecessor DNSKEY enters the HIDDEN state.
    zonename = f"step7.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract DNSKEY TTL plus zone propagation delay from all the times (2h).
    TpubN = "now-5093h"
    TactN = "now-5068h"
    TretN = "now-629h"
    TremN = "now-2h"
    TpubN1 = "now-632h"
    TactN1 = TretN
    TretN1 = "now+3835h"
    TremN1 = "now+4462h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k UNRETENTIVE {TremN} -r HIDDEN {TremN} -z HIDDEN {TactN1} -d HIDDEN {TremN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z OMNIPRESENT {TactN1} -d OMNIPRESENT {TactN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 8:
    # The predecessor DNSKEY can be purged.
    zonename = f"step8.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract purge-keys interval from all the times (1h).
    TpubN = "now-5094h"
    TactN = "now-5069h"
    TretN = "now-630h"
    TremN = "now-3h"
    TpubN1 = "now-633h"
    TactN1 = TretN
    TretN1 = "now+3834h"
    TremN1 = "now+4461h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k HIDDEN {TremN} -r HIDDEN {TremN} -z HIDDEN {TactN1} -d HIDDEN {TremN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z OMNIPRESENT {TactN1} -d OMNIPRESENT {TactN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    return zones


def configure_cskroll2(tld: str, policy: str) -> List[Zone]:
    # The zones at csk-roll2.$tld represent the various steps of a CSK rollover
    # (which is essentially a ZSK Pre-Publication / KSK Double-KSK rollover).
    # This scenario differs from the csk-roll1 one because the zone signatures (ZRRSIG)
    # are replaced with the new key sooner than the DS is swapped.
    zones = []
    zone = f"csk-roll2.{tld}"
    cds = "cdnskey,cds:sha-256,cds:sha-384"
    keygen = EnvCmd("KEYGEN", f"-k {policy} -l kasp.conf")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # Introduce the first key. This will immediately be active.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    TactN = "now-7d"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options=f"-z -G {cds}")

    # Step 2:
    # It is time to introduce the new CSK.
    zonename = f"step2.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # KSK: Tpub(N+1) <= Tact(N) + Lksk - IpubC
    # ZSK: Tpub(N+1) <= Tact(N) + Lzsk - Ipub
    # IpubC = DprpC + TTLkey (+publish-safety)
    # Ipub  = IpubC
    # Lcsk = Lksk = Lzsk
    #
    # Lcsk:           6mo (186d, 4464h)
    # Dreg:           N/A
    # DprpC:          1h
    # TTLkey:         1h
    # publish-safety: 1h
    # Ipub:           3h
    #
    # Tact(N)  = now - Lcsk + Ipub = now - 186d + 3h
    #          = now - 4464h + 3h = now - 4461h
    TactN = "now-4461h"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options=f"-z -G {cds}")

    # Step 3:
    # It is time to submit the DS and to roll signatures.
    zonename = f"step3.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    #
    # Tsbm(N+1) >= Trdy(N+1)
    # KSK: Tact(N+1) = Tsbm(N+1)
    # ZSK: Tact(N+1) = Tpub(N+1) + Ipub = Tsbm(N+1)
    # KSK: Iret  = DprpP + TTLds (+retire-safety)
    # ZSK: IretZ = Dsgn + Dprp + TTLsig (+retire-safety)
    #
    # Lcsk:           186d
    # Dprp:           1h
    # DprpP:          1w
    # Dreg:           N/A
    # Dsgn:           12h
    # TTLds:          1h
    # TTLsig:         1d
    # retire-safety:  1h
    # Iret:           170h
    # IretZ:          38h
    # Ipub:           3h
    #
    # Tpub(N)   = now - Lcsk = now - 186d
    # Tact(N)   = now - Lcsk + Dprp + TTLsig = now - 4439h
    # Tret(N)   = now
    # Trem(N)   = now + IretZ = now + 26d3h = now + 627h
    # Tpub(N+1) = now - Ipub = now - 3h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now + Lcsk = now + 186d = now + 186d
    # Trem(N+1) = now + Lcsk + IretZ = now + 186d + 26d3h =
    #           = now + 5091h
    TpubN = "now-186d"
    TactN = "now-4439h"
    TretN = "now"
    TremN = "now+170h"
    TpubN1 = "now-3h"
    TactN1 = TretN
    TretN1 = "now+186d"
    TremN1 = "now+4634h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k RUMOURED {TpubN1} -r RUMOURED {TpubN1} -z HIDDEN {TpubN1} -d HIDDEN {TpubN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 4:
    # Some time later all the ZRRSIG records should be from the new CSK, and the
    # DS should be swapped.  The ZRRSIG records are all replaced after IretZ (38h).
    # The DS is swapped after Dreg + Iret (1w3h). In other words, the zone
    # signatures are replaced before the DS is swapped.
    zonename = f"step4.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # Trem(N)    = Tret(N) + IretZ
    #
    # Lcsk:   186d
    # Dreg:   N/A
    # Iret:   170h
    # IretZ:  38h
    #
    # Tpub(N)    = now - IretZ - Lcsk = now - 38h - 186d
    #            = now - 38h - 4464h = now - 4502h
    # Tact(N)    = now - Iret - Lcsk + TTLsig = now - 4502h + 25h = now - 4477h
    # Tret(N)    = now - IretZ = now - 38h
    # Trem(N)    = now - IretZ + Iret = now - 38h + 170h = now + 132h
    # Tpub(N+1)  = now - IretZ - IpubC = now - 38h - 3h = now - 41h
    # Tact(N+1)  = Tret(N)
    # Tret(N+1)  = now - IretZ + Lcsk = now - 38h + 186d
    #            = now + 4426h
    # Trem(N+1)  = now - IretZ + Lcsk + Iret
    #            = now + 4426h + 3h = now + 4429h
    TpubN = "now-4502h"
    TactN = "now-4477h"
    TretN = "now-38h"
    TremN = "now+132h"
    TpubN1 = "now-41h"
    TactN1 = TretN
    TretN1 = "now+4426h"
    TremN1 = "now+4429h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z UNRETENTIVE {TretN} -d UNRETENTIVE {TretN} -D ds {TretN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z RUMOURED {TactN1} -d RUMOURED {TactN1} -P ds {TactN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 5:
    # Some time later the DS can be swapped and the old DNSKEY can be removed from
    # the zone.
    zonename = f"step5.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract Iret (170h) - IretZ (38h) = 132h.
    #
    # Tpub(N)   = now - 4502h - 132h = now - 4634h
    # Tact(N)   = now - 4477h - 132h = now - 4609h
    # Tret(N)   = now - 38h - 132h = now - 170h
    # Trem(N)   = now + 132h - 132h = now
    # Tpub(N+1) = now - 41h - 132h = now - 173h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now + 4426h - 132h = now + 4294h
    # Trem(N+1) = now + 4492h - 132h = now + 4360h
    TpubN = "now-4634h"
    TactN = "now-4609h"
    TretN = "now-170h"
    TremN = "now"
    TpubN1 = "now-173h"
    TactN1 = TretN
    TretN1 = "now+4294h"
    TremN1 = "now+4360h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -z HIDDEN now-133h -d UNRETENTIVE {TactN1} -D ds {TactN1} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z OMNIPRESENT now-133h -d RUMOURED {TactN1} -P ds {TactN1} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 6:
    # Some time later the predecessor DNSKEY enters the HIDDEN state.
    zonename = f"step6.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract DNSKEY TTL plus zone propagation delay (2h).
    #
    # Tpub(N)   = now - 4634h - 2h = now - 4636h
    # Tact(N)   = now - 4609h - 2h = now - 4611h
    # Tret(N)   = now - 170h - 2h = now - 172h
    # Trem(N)   = now - 2h
    # Tpub(N+1) = now - 173h - 2h = now - 175h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now + 4294h - 2h = now + 4292h
    # Trem(N+1) = now + 4360h - 2h = now + 4358h
    TpubN = "now-4636h"
    TactN = "now-4611h"
    TretN = "now-172h"
    TremN = "now-2h"
    TpubN1 = "now-175h"
    TactN1 = TretN
    TretN1 = "now+4292h"
    TremN1 = "now+4358h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k UNRETENTIVE {TremN} -r UNRETENTIVE {TremN} -z HIDDEN now-135h -d HIDDEN {TremN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z OMNIPRESENT now-135h -d OMNIPRESENT {TremN} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 7:
    # The predecessor DNSKEY can be purged, but purge-keys is disabled.
    zonename = f"step7.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract 90 days (default, 2160h) from all the times.
    #
    # Tpub(N)   = now - 4636h - 2160h = now - 6796h
    # Tact(N)   = now - 4611h - 2160h = now - 6771h
    # Tret(N)   = now - 172h - 2160h = now - 2332h
    # Trem(N)   = now - 2h - 2160h = now - 2162h
    # Tpub(N+1) = now - 175h - 2160h = now - 2335h
    # Tact(N+1) = Tret(N)
    # Tret(N+1) = now + 4292h - 2160h = now + 2132h
    # Trem(N+1) = now + 4358h - 2160h = now + 2198h
    TpubN = "now-6796h"
    TactN = "now-6771h"
    TretN = "now-2332h"
    TremN = "now-2162h"
    TpubN1 = "now-2335h"
    TactN1 = TretN
    TretN1 = "now+2132h"
    TremN1 = "now+2198h"

    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k UNRETENTIVE {TremN} -r HIDDEN {TremN} -z HIDDEN {TactN1} -d HIDDEN {TremN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z OMNIPRESENT {TactN1} -d OMNIPRESENT {TremN} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    # Step 8:
    # The predecessor DNSKEY can be purged.
    zonename = f"step8.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract purge-keys interval from all the times (1h).
    TpubN = "now-5094h"
    TactN = "now-5069h"
    TretN = "now-630h"
    TremN = "now-3h"
    TpubN1 = "now-633h"
    TactN1 = TretN
    TretN1 = "now+3834h"
    TremN1 = "now+4461h"
    keytimes = (
        f"-P {TpubN} -P sync {TactN} -A {TpubN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -P sync {TactN1} -A {TactN1} -I {TretN1} -D {TremN1}"
    # Key generation.
    csk1_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    csk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k UNRETENTIVE {TremN} -r UNRETENTIVE {TremN} -z HIDDEN now-2295h -d HIDDEN {TremN} {csk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -z OMNIPRESENT now-2295h -d OMNIPRESENT {TremN} {csk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(csk1_name, csk2_name)
    # Signing.
    render_and_sign_zone(zonename, [csk1_name, csk2_name], extra_options=f"-z -G {cds}")

    return zones


def configure_enable_dnssec(tld: str, policy: str) -> List[Zone]:
    # The zones at enable-dnssec.$tld represent the various steps of the
    # initial signing of a zone.
    zones = []
    zone = f"enable-dnssec.{tld}"
    keygen = EnvCmd("KEYGEN", f"-k {policy} -l kasp.conf")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # This is an unsigned zone and named should perform the initial steps of
    # introducing the DNSSEC records in the right order.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    render_and_sign_zone(zonename, [], signing=False)

    # Step 2:
    # The DNSKEY has been published long enough to become OMNIPRESENT.
    zonename = f"step2.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # DNSKEY TTL:             300 seconds
    # zone-propagation-delay: 5 minutes (300 seconds)
    # publish-safety:         5 minutes (300 seconds)
    # Total:                  900 seconds
    TpubN = "now-900s"
    keytimes = f"-P {TpubN} -A {TpubN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k RUMOURED {TpubN} -r RUMOURED {TpubN} -z RUMOURED {TpubN} -d HIDDEN {TpubN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options="-z")

    # Step 3:
    # The zone signatures have been published long enough to become OMNIPRESENT.
    zonename = f"step3.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Passed time since publication:
    # max-zone-ttl:           12 hours (43200 seconds)
    # zone-propagation-delay: 5 minutes (300 seconds)
    # We can submit the DS now.
    TpubN = "now-43500s"
    keytimes = f"-P {TpubN} -A {TpubN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} -z RUMOURED {TpubN} -d HIDDEN {TpubN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options="-z")

    # Step 4:
    # The DS has been submitted long enough ago to become OMNIPRESENT.
    zonename = f"step4.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # DS TTL:                    2 hour (7200 seconds)
    # parent-propagation-delay:  1 hour (3600 seconds)
    # Total aditional time:      10800 seconds
    # 43500 + 10800 = 54300
    TpubN = "now-54300s"
    TsbmN = "now-10800s"
    keytimes = f"-P {TpubN} -A {TpubN} -P sync {TsbmN}"
    # Key generation.
    csk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} -z OMNIPRESENT {TsbmN} -d RUMOURED {TpubN} -P ds {TsbmN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options="-z")

    return zones


def configure_going_insecure(tld: str, reconfig: bool = False) -> List[Zone]:
    zones = []
    keygen = EnvCmd("KEYGEN", "-a ECDSA256 -L 7200")
    settime = EnvCmd("SETTIME", "-s")

    # The child zones (step1, step2) beneath these zones represent the various
    # steps of unsigning a zone.
    for zone in [f"going-insecure.{tld}", f"going-insecure-dynamic.{tld}"]:
        # Set up a zone with dnssec-policy that is going insecure.

        # Step 1:
        zonename = f"step1.{zone}"
        zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
        isctest.log.info(f"setup {zonename}")
        # Timing metadata.
        TpubN = "now-10d"
        TsbmN = "now-12955mi"
        keytimes = f"-P {TpubN} -A {TpubN}"
        cdstimes = f"-P sync {TsbmN}"
        # Key generation.
        ksk_name = keygen(
            f"-f KSK {keytimes} {cdstimes} {zonename}", cwd="ns3"
        ).out.strip()
        zsk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} -d OMNIPRESENT {TpubN} {ksk_name}",
            cwd="ns3",
        )
        settime(
            f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} {zsk_name}",
            cwd="ns3",
        )
        # Signing.
        render_and_sign_zone(zonename, [ksk_name, zsk_name])

        if reconfig:
            # Step 2:
            zonename = f"step2.{zone}"
            zones.append(
                Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3"))
            )
            isctest.log.info(f"setup {zonename}")
            # The DS was withdrawn from the parent zone 26 hours ago.
            TremN = "now-26h"
            keytimes = f"-P {TpubN} -A {TpubN} -I {TremN} -D now"
            cdstimes = f"-P sync {TsbmN} -D sync {TremN}"
            # Key generation.
            ksk_name = keygen(
                f"-f KSK {keytimes} {cdstimes} {zonename}", cwd="ns3"
            ).out.strip()
            zsk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
            settime(
                f"-g HIDDEN -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} -d UNRETENTIVE {TremN} -D ds {TremN} {ksk_name}",
                cwd="ns3",
            )
            settime(
                f"-g HIDDEN -k OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} {zsk_name}",
                cwd="ns3",
            )
            # Fake lifetime of old algorithm keys.
            fake_lifetime(ksk_name, 0)
            fake_lifetime(zsk_name, 5184000)
            # Signing.
            render_and_sign_zone(zonename, [ksk_name, zsk_name], extra_options="-P")

    return zones


def configure_straight2none(tld: str) -> List[Zone]:
    # These zones are going straight to "none" policy. This is undefined behavior.
    zones = []
    keygen = EnvCmd("KEYGEN", "-k default")
    settime = EnvCmd("SETTIME", "-s")

    TpubN = "now-10d"
    TsbmN = "now-12955mi"
    keytimes = f"-P {TpubN} -A {TpubN} -P sync {TsbmN}"

    zonename = f"going-straight-to-none.{tld}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Key generation.
    csk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} -d OMNIPRESENT {TpubN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options="-z")

    zonename = f"going-straight-to-none-dynamic.{tld}"
    zones.append(
        Zone(zonename, f"{zonename}.db.signed", Nameserver("ns3", "10.53.0.3"))
    )
    isctest.log.info(f"setup {zonename}")
    # Key generation.
    csk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -r OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} -d OMNIPRESENT {TpubN} {csk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [csk_name], extra_options="-z -O full")

    return zones


def configure_ksk_doubleksk(tld: str) -> List[Zone]:
    # The zones at ksk-doubleksk.$tld represent the various steps of a KSK
    # Double-KSK rollover.
    zones = []
    zone = f"ksk-doubleksk.{tld}"
    cds = "cds:sha-256"
    keygen = EnvCmd("KEYGEN", "-a ECDSAP256SHA256 -L 7200")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # Introduce the first key. This will immediately be active.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Timing metadata.
    TactN = "now-7d"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk_name], extra_options=f"-G {cds}")

    # Step 2:
    # It is time to introduce the new KSK.
    zonename = f"step2.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Lksk:           60d
    # Dreg:           n/a
    # DprpC:          1h
    # TTLds:          1d
    # TTLkey:         2h
    # publish-safety: 1d
    # retire-safety:  2d
    #
    # According to RFC 7583:
    # Tpub(N+1) <= Tact(N) + Lksk - Dreg - IpubC
    # IpubC = DprpC + TTLkey (+publish-safety)
    #
    # IpubC   = 27h
    # Tact(N) = now - Lksk + Dreg + IpubC = now - 60d + 27h
    #         = now - 1440h + 27h = now - 1413h
    TactN = "now-1413h"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk_name], extra_options=f"-G {cds}")

    # Step 3:
    # It is time to submit the DS.
    zonename = f"step3.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # Iret = DprpP + TTLds (+retire-safety)
    #
    # Iret       = 50h
    # Tpub(N)    = now - Lksk = now - 60d = now - 60d
    # Tact(N)    = now - 1413h
    # Tret(N)    = now
    # Trem(N)    = now + Iret = now + 50h
    # Tpub(N+1)  = now - IpubC = now - 27h
    # Tact(N+1)  = now
    # Tret(N+1)  = now + Lksk = now + 60d
    # Trem(N+1)  = now + Lksk + Iret = now + 60d + 50h
    #            = now + 1440h + 50h = 1490h
    TpubN = "now-60d"
    TactN = "now-1413h"
    TretN = "now"
    TremN = "now+50h"
    TpubN1 = "now-27h"
    TactN1 = "now"
    TretN1 = "now+60d"
    TremN1 = "now+1490h"
    ksktimes = (
        f"-P {TpubN} -A {TpubN} -P sync {TactN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -A {TactN1} -P sync {TactN1} -I {TretN1} -D {TremN1}"
    zsktimes = f"-P {TpubN}  -A {TpubN}"
    # Key generation.
    ksk1_name = keygen(f"-f KSK {ksktimes} {zonename}", cwd="ns3").out.strip()
    ksk2_name = keygen(f"-f KSK {newtimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{zsktimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k RUMOURED {TpubN1} -r RUMOURED {TpubN1} -d HIDDEN {TpubN1} {ksk2_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} {zsk_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(ksk1_name, ksk2_name)
    # Signing.
    render_and_sign_zone(
        zonename, [ksk1_name, ksk2_name, zsk_name], extra_options=f"-G {cds}"
    )

    # Step 4:
    # The DS should be swapped now.
    zonename = f"step4.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Tpub(N)    = now - Lksk - Iret = now - 60d - 50h
    #            = now - 1440h - 50h = now - 1490h
    # Tact(N)    = now - 1490h + 27h = now - 1463h
    # Tret(N)    = now - Iret = now - 50h
    # Trem(N)    = now
    # Tpub(N+1)  = now - Iret - IpubC = now - 50h - 27h
    #            = now - 77h
    # Tact(N+1)  = Tret(N)
    # Tret(N+1)  = now + Lksk - Iret = now + 60d - 50h = now + 1390h
    # Trem(N+1)  = now + Lksk = now + 60d
    TpubN = "now-1490h"
    TactN = "now-1463h"
    TretN = "now-50h"
    TremN = "now"
    TpubN1 = "now-77h"
    TactN1 = TretN
    TretN1 = "now+1390h"
    TremN1 = "now+60d"
    ksktimes = (
        f"-P {TpubN} -A {TpubN} -P sync {TactN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -A {TactN1} -P sync {TactN1} -I {TretN1} -D {TremN1}"
    zsktimes = f"-P {TpubN} -A {TpubN}"
    # Key generation.
    ksk1_name = keygen(f"-f KSK {ksktimes} {zonename}", cwd="ns3").out.strip()
    ksk2_name = keygen(f"-f KSK {newtimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{zsktimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d UNRETENTIVE {TretN} -D ds {TretN} {ksk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -d RUMOURED {TactN1} -P ds {TactN1} {ksk2_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(ksk1_name, ksk2_name)
    # Signing.
    render_and_sign_zone(
        zonename, [ksk1_name, ksk2_name, zsk_name], extra_options=f"-G {cds}"
    )

    # Step 5:
    # The predecessor DNSKEY is removed long enough that is has become HIDDEN.
    zonename = f"step5.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract DNSKEY TTL + zone-propagation-delay from all the times (3h).
    # Tpub(N)    = now - 1490h - 3h = now - 1493h
    # Tact(N)    = now - 1463h - 3h = now - 1466h
    # Tret(N)    = now - 50h - 3h = now - 53h
    # Trem(N)    = now - 3h
    # Tpub(N+1)  = now - 77h - 3h = now - 80h
    # Tact(N+1)  = Tret(N)
    # Tret(N+1)  = now + 1390h - 3h = now + 1387h
    # Trem(N+1)  = now + 60d - 3h = now + 1441h
    TpubN = "now-1493h"
    TactN = "now-1466h"
    TretN = "now-53h"
    TremN = "now-3h"
    TpubN1 = "now-80h"
    TactN1 = TretN
    TretN1 = "now+1387h"
    TremN1 = "now+1441h"
    ksktimes = (
        f"-P {TpubN} -A {TpubN} -P sync {TactN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -A {TactN1} -P sync {TactN1} -I {TretN1} -D {TremN1}"
    zsktimes = f"-P {TpubN} -A {TpubN}"
    # Key generation.
    ksk1_name = keygen(f"-f KSK {ksktimes} {zonename}", cwd="ns3").out.strip()
    ksk2_name = keygen(f"-f KSK {newtimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{zsktimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k UNRETENTIVE {TretN} -r UNRETENTIVE {TretN} -d HIDDEN {TretN} {ksk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -d OMNIPRESENT {TactN1} {ksk2_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(ksk1_name, ksk2_name)
    # Signing.
    render_and_sign_zone(
        zonename, [ksk1_name, ksk2_name, zsk_name], extra_options=f"-G {cds}"
    )

    # Step 6:
    # The predecessor DNSKEY can be purged.
    zonename = f"step6.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract purge-keys interval from all the times (1h).
    TpubN = "now-1494h"
    TactN = "now-1467h"
    TretN = "now-54h"
    TremN = "now-4h"
    TpubN1 = "now-81h"
    TactN1 = TretN
    TretN1 = "now+1386h"
    TremN1 = "now+1440h"
    ksktimes = (
        f"-P {TpubN} -A {TpubN} -P sync {TactN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -A {TactN1} -P sync {TactN1} -I {TretN1} -D {TremN1}"
    zsktimes = f"-P {TpubN} -A {TpubN}"
    # Key generation.
    ksk1_name = keygen(f"-f KSK {ksktimes} {zonename}", cwd="ns3").out.strip()
    ksk2_name = keygen(f"-f KSK {newtimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{zsktimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k HIDDEN {TretN} -r HIDDEN {TretN} -d HIDDEN {TretN} {ksk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -r OMNIPRESENT {TactN1} -d OMNIPRESENT {TactN1} {ksk2_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(ksk1_name, ksk2_name)
    # Signing.
    render_and_sign_zone(
        zonename, [ksk1_name, ksk2_name, zsk_name], extra_options=f"-G {cds}"
    )

    return zones


def configure_ksk_3crowd(tld: str) -> List[Zone]:
    # Test #2375, the "three is a crowd" bug, where a new key is introduced but the
    # previous rollover has not finished yet. In other words, we have a key KEY2
    # that is the successor of key KEY1, and we introduce a new key KEY3 that is
    # the successor of key KEY2:
    #
    #     KEY1 < KEY2 < KEY3.
    #
    # The expected behavior is that all three keys remain in the zone, and not
    # the bug behavior where KEY2 is removed and immediately replaced with KEY3.
    #
    zones = []
    cds = "cds:sha-256"
    keygen = EnvCmd("KEYGEN", "-a ECDSAP256SHA256 -L 7200")
    settime = EnvCmd("SETTIME", "-s")

    # Set up a zone that has a KSK (KEY1) and have the successor key (KEY2)
    # published as well.
    zonename = f"three-is-a-crowd.{tld}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # These times are the same as step3.ksk-doubleksk.autosign.
    TpubN = "now-60d"
    TactN = "now-1413h"
    TretN = "now"
    TremN = "now+50h"
    TpubN1 = "now-27h"
    TactN1 = TretN
    TretN1 = "now+60d"
    TremN1 = "now+1490h"
    ksktimes = (
        f"-P {TpubN} -A {TpubN} -P sync {TactN} -I {TretN} -D {TremN} -D sync {TactN1}"
    )
    newtimes = f"-P {TpubN1} -A {TactN1} -P sync {TactN1} -I {TretN1} -D {TremN1}"
    zsktimes = f"-P {TpubN}  -A {TpubN}"
    # Key generation.
    ksk1_name = keygen(f"-f KSK {ksktimes} {zonename}", cwd="ns3").out.strip()
    ksk2_name = keygen(f"-f KSK {newtimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{zsktimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k RUMOURED {TpubN1} -r RUMOURED {TpubN1} -d HIDDEN {TpubN1} {ksk2_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TpubN} -z OMNIPRESENT {TpubN} {zsk_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(ksk1_name, ksk2_name)
    # Signing.
    render_and_sign_zone(
        zonename, [ksk1_name, ksk2_name, zsk_name], extra_options=f"-G {cds}"
    )

    return zones


def configure_zsk_prepub(tld: str) -> List[Zone]:
    # The zones at zsk-prepub.$tld represent the various steps of a ZSK
    # Pre-Publication rollover.
    zones = []
    zone = f"zsk-prepub.{tld}"
    keygen = EnvCmd("KEYGEN", "-a ECDSAP256SHA256 -L 3600")
    settime = EnvCmd("SETTIME", "-s")

    # Step 1:
    # Introduce the first key. This will immediately be active.
    zonename = f"step1.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Timing metadata.
    TactN = "now-7d"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk_name])

    # Step 2:
    # It is time to pre-publish the successor ZSK.
    zonename = f"step2.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # Tact(N) = now + Ipub - Lzsk = now + 26h - 30d
    #         = now + 26h - 30d = now − 694h
    TactN = "now-694h"
    keytimes = f"-P {TactN} -A {TactN}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk_name = keygen(f"{keytimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk_name}",
        cwd="ns3",
    )
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk_name])

    # Step 3:
    # After the publication interval has passed the DNSKEY of the successor ZSK
    # is OMNIPRESENT and the zone can thus be signed with the successor ZSK.
    zonename = f"step3.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # According to RFC 7583:
    # Tpub(N+1) <= Tact(N) + Lzsk - Ipub
    # Tact(N+1) = Tact(N) + Lzsk
    #
    # Tact(N)   = now - Lzsk = now - 30d
    # Tpub(N+1) = now - Ipub = now - 26h
    # Tact(N+1) = now
    # Tret(N) = now
    # Trem(N) = now + Iret = now + Dsign + Dprp + TTLsig + retire-safety = 8d1h = now + 241h
    TactN = "now-30d"
    TpubN1 = "now-26h"
    TactN1 = "now"
    TremN = "now+241h"
    keytimes = f"-P {TactN} -A {TactN}"
    oldtimes = f"-P {TactN} -A {TactN} -I {TactN1} -D {TremN}"
    newtimes = f"-P {TpubN1} -A {TactN1}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk1_name = keygen(f"{oldtimes} {zonename}", cwd="ns3").out.strip()
    zsk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -z OMNIPRESENT {TactN} {zsk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k RUMOURED {TpubN1} -z HIDDEN {TpubN1} {zsk2_name}", cwd="ns3"
    )
    # Set key rollover relationship.
    set_key_relationship(zsk1_name, zsk2_name)
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk1_name, zsk2_name])

    # Step 4:
    # After the retire interval has passed the predecessor DNSKEY can be
    # removed from the zone.
    zonename = f"step4.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Lzsk:          30d
    # Ipub:          26h
    # Dsgn:          1w
    # Dprp:          1h
    # TTLsig:        1d
    # retire-safety: 2d
    #
    # According to RFC 7583:
    # Iret      = Dsgn + Dprp + TTLsig (+retire-safety)
    # Iret      = 1w + 1h + 1d + 2d = 10d1h = 241h
    #
    # Tact(N)   = now - Iret - Lzsk
    #           = now - 241h - 30d = now - 241h - 720h
    #           = now - 961h
    # Tpub(N+1) = now - Iret - Ipub
    #           = now - 241h - 26h
    #           = now - 267h
    # Tact(N+1) = now - Iret = now - 241h
    TactN = "now-961h"
    TpubN1 = "now-267h"
    TactN1 = "now-241h"
    TremN = "now"
    keytimes = f"-P {TactN} -A {TactN}"
    oldtimes = f"-P {TactN} -A {TactN} -I {TactN1} -D {TremN}"
    newtimes = f"-P {TpubN1} -A {TactN1}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk1_name = keygen(f"{oldtimes} {zonename}", cwd="ns3").out.strip()
    zsk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g HIDDEN -k OMNIPRESENT {TactN} -z UNRETENTIVE {TactN1} {zsk1_name}",
        cwd="ns3",
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -z RUMOURED {TactN1} {zsk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(zsk1_name, zsk2_name)
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk1_name, zsk2_name])

    # Step 5:
    # The predecessor DNSKEY is removed long enough that is has become HIDDEN.
    zonename = f"step5.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract DNSKEY TTL + zone-propagation-delay from all the times (2h).
    # Tact(N)   = now - 961h - 2h = now - 963h
    # Tpub(N+1) = now - 267h - 2h = now - 269h
    # Tact(N+1) = now - 241h - 2h = now - 243h
    # Trem(N)   = Tact(N+1) + Iret = now -2h
    TactN = "now-963h"
    TremN = "now-2h"
    TpubN1 = "now-269h"
    TactN1 = "now-243h"
    TremN = "now-2h"
    keytimes = f"-P {TactN} -A {TactN}"
    oldtimes = f"-P {TactN} -A {TactN} -I {TactN1} -D {TremN}"
    newtimes = f"-P {TpubN1} -A {TactN1}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk1_name = keygen(f"{oldtimes} {zonename}", cwd="ns3").out.strip()
    zsk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(
        f"-g HIDDEN -k UNRETENTIVE {TremN} -z HIDDEN {TremN} {zsk1_name}", cwd="ns3"
    )
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -z OMNIPRESENT {TremN} {zsk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(zsk1_name, zsk2_name)
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk1_name, zsk2_name])

    # Step 6:
    # The predecessor DNSKEY can be purged.
    zonename = f"step6.{zone}"
    zones.append(Zone(zonename, f"{zonename}.db", Nameserver("ns3", "10.53.0.3")))
    isctest.log.info(f"setup {zonename}")
    # Subtract purge-keys interval from all the times (1h).
    TactN = "now-964h"
    TremN = "now-3h"
    TpubN1 = "now-270h"
    TactN1 = "now-244h"
    TremN = "now-3h"
    keytimes = f"-P {TactN} -A {TactN}"
    oldtimes = f"-P {TactN} -A {TactN} -I {TactN1} -D {TremN}"
    newtimes = f"-P {TpubN1} -A {TactN1}"
    # Key generation.
    ksk_name = keygen(f"-f KSK {keytimes} {zonename}", cwd="ns3").out.strip()
    zsk1_name = keygen(f"{oldtimes} {zonename}", cwd="ns3").out.strip()
    zsk2_name = keygen(f"{newtimes} {zonename}", cwd="ns3").out.strip()
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN} -r OMNIPRESENT {TactN} -d OMNIPRESENT {TactN} {ksk_name}",
        cwd="ns3",
    )
    settime(f"-g HIDDEN -k HIDDEN {TremN} -z HIDDEN {TremN} {zsk1_name}", cwd="ns3")
    settime(
        f"-g OMNIPRESENT -k OMNIPRESENT {TactN1} -z OMNIPRESENT {TremN} {zsk2_name}",
        cwd="ns3",
    )
    # Set key rollover relationship.
    set_key_relationship(zsk1_name, zsk2_name)
    # Signing.
    render_and_sign_zone(zonename, [ksk_name, zsk1_name, zsk2_name])

    return zones
