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
import subprocess
import time

from datetime import timedelta

import dns
import dns.update
import pytest

pytest.importorskip("dns", minversion="2.0.0")
import isctest
import isctest.mark
from isctest.kasp import (
    KeyProperties,
    KeyTimingMetadata,
)
from isctest.util import param
from isctest.vars.algorithms import ECDSAP256SHA256, ECDSAP384SHA384

pytestmark = pytest.mark.extra_artifacts(
    [
        "K*.private",
        "K*.backup",
        "K*.cmp",
        "K*.key",
        "K*.state",
        "*.axfr",
        "*.created",
        "dig.out*",
        "keyevent.out.*",
        "keygen.out.*",
        "keys",
        "published.test*",
        "python.out.*",
        "retired.test*",
        "rndc.dnssec.*.out.*",
        "rndc.zonestatus.out.*",
        "rrsig.out.*",
        "created.key-*",
        "unused.key-*",
        "verify.out.*",
        "zone.out.*",
        "ns*/K*.key",
        "ns*/K*.offline",
        "ns*/K*.private",
        "ns*/K*.state",
        "ns*/*.db",
        "ns*/*.db.infile",
        "ns*/*.db.signed",
        "ns*/*.db.signed.tmp",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/*.zsk1",
        "ns*/*.zsk2",
        "ns*/dsset-*",
        "ns*/keygen.out.*",
        "ns*/keys",
        "ns*/ksk",
        "ns*/ksk/K*",
        "ns*/zsk",
        "ns*/zsk",
        "ns*/zsk/K*",
        "ns*/named*.conf",
        "ns*/settime.out.*",
        "ns*/signer.out.*",
        "ns*/zones",
        "ns*/policies/*.conf",
        "ns3/legacy-keys.*",
        "ns3/dynamic-signed-inline-signing.kasp.db.signed.signed",
        "ns4/purgekeys.conf",
        "ns4/purgekeys2.conf",
    ]
)


kasp_config = {
    "dnskey-ttl": timedelta(seconds=1234),
    "ds-ttl": timedelta(days=1),
    "key-directory": "{keydir}",
    "max-zone-ttl": timedelta(days=1),
    "parent-propagation-delay": timedelta(hours=1),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=5),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(minutes=5),
}

autosign_config = {
    "dnskey-ttl": timedelta(seconds=300),
    "ds-ttl": timedelta(days=1),
    "key-directory": "{keydir}",
    "max-zone-ttl": timedelta(days=1),
    "parent-propagation-delay": timedelta(hours=1),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=7),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(minutes=5),
}

lifetime = {
    "P10Y": int(timedelta(days=10 * 365).total_seconds()),
    "P5Y": int(timedelta(days=5 * 365).total_seconds()),
    "P2Y": int(timedelta(days=2 * 365).total_seconds()),
    "P1Y": int(timedelta(days=365).total_seconds()),
    "P30D": int(timedelta(days=30).total_seconds()),
    "P6M": int(timedelta(days=31 * 6).total_seconds()),
    "P2M": int(timedelta(days=31 * 2).total_seconds()),
}

KASP_INHERIT_TSIG_SECRET = {
    "sha1": "FrSt77yPTFx6hTs4i2tKLB9LmE0=",
    "sha224": "hXfwwwiag2QGqblopofai9NuW28q/1rH4CaTnA==",
    "sha256": "R16NojROxtxH/xbDl//ehDsHm5DjWTQ2YXV+hGC2iBY=",
    "view1": "YPfMoAk6h+3iN8MDRQC004iSNHY=",
    "view2": "4xILSZQnuO1UKubXHkYUsvBRPu8=",
    "view3": "C1Azf+gGPMmxrUg/WQINP6eV9Y0=",
}


def autosign_properties(alg, size):
    return [
        f"ksk {lifetime['P2Y']} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk {lifetime['P1Y']} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]


def rsa1_properties(alg):
    return [
        f"ksk {lifetime['P10Y']} {alg} 2048 goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk {lifetime['P5Y']} {alg} 2048 goal:omnipresent dnskey:rumoured zrrsig:rumoured",
        f"zsk {lifetime['P1Y']} {alg} 2000 goal:omnipresent dnskey:rumoured zrrsig:rumoured",
    ]


def fips_properties(alg, bits=None):
    sizes = [2048, 2048, 3072]
    if bits is not None:
        sizes = [bits, bits, bits]

    return [
        f"ksk {lifetime['P10Y']} {alg} {sizes[0]} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk {lifetime['P5Y']} {alg} {sizes[1]} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
        f"zsk {lifetime['P1Y']} {alg} {sizes[2]} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
    ]


def check_all(
    server, zone, policy, ksks, zsks, manual_mode=False, zsk_missing=False, tsig=None
):
    isctest.kasp.check_dnssecstatus(server, zone, ksks + zsks, policy=policy)
    isctest.kasp.check_apex(
        server,
        zone,
        ksks,
        zsks,
        manual_mode=manual_mode,
        zsk_missing=zsk_missing,
        tsig=tsig,
    )
    isctest.kasp.check_subdomain(server, zone, ksks, zsks, tsig=tsig)


def set_keytimes_default_policy(kp):
    # The first key is immediately published and activated.
    kp.timing["Generated"] = kp.key.get_timing("Created")
    kp.timing["Published"] = kp.timing["Generated"]
    kp.timing["Active"] = kp.timing["Generated"]
    # The DS can be published if the DNSKEY and RRSIG records are
    # OMNIPRESENT.  This happens after max-zone-ttl (1d) plus
    # plus zone-propagation-delay (300s).
    kp.timing["PublishCDS"] = kp.timing["Published"] + timedelta(days=1, seconds=300)
    # Key lifetime is unlimited, so not setting 'Retired' nor 'Removed'.
    kp.timing["DNSKEYChange"] = kp.timing["Published"]
    kp.timing["DSChange"] = kp.timing["Published"]
    kp.timing["KRRSIGChange"] = kp.timing["Active"]
    kp.timing["ZRRSIGChange"] = kp.timing["Active"]


def cb_ixfr_is_signed(expected_updates, params, ksks=None, zsks=None):
    zone = params["zone"]
    policy = params["policy"]
    servers = params["servers"]

    isctest.log.info(f"check that the zone {zone} is correctly signed after ixfr")
    isctest.log.debug(
        f"expected updates {expected_updates} policy {policy} ksks {ksks} zsks {zsks}"
    )
    shutil.copyfile(f"ns2/{zone}.db.in2", f"ns2/{zone}.db")
    servers["ns2"].rndc(f"reload {zone}")

    def update_is_signed():
        parts = update.split()
        qname = parts[0]
        qtype = dns.rdatatype.from_text(parts[1])
        rdata = parts[2]
        return isctest.kasp.verify_update_is_signed(
            servers["ns3"], zone, qname, qtype, rdata, ksks, zsks
        )

    for update in expected_updates:
        isctest.run.retry_with_timeout(update_is_signed, timeout=10)


def cb_rrsig_refresh(params, ksks=None, zsks=None):
    zone = params["zone"]
    servers = params["servers"]

    isctest.log.info(f"check that the zone {zone} refreshes expired signatures")

    def rrsig_is_refreshed():
        parts = query.split()
        qname = parts[0]
        qtype = dns.rdatatype.from_text(parts[1])
        return isctest.kasp.verify_rrsig_is_refreshed(
            servers["ns3"], zone, f"ns3/{zone}.db.signed", qname, qtype, ksks, zsks
        )

    queries = [
        f"{zone} DNSKEY",
        f"{zone} SOA",
        f"{zone} NS",
        f"{zone} NSEC",
        f"a.{zone} A",
        f"a.{zone} NSEC",
        f"b.{zone} A",
        f"b.{zone} NSEC",
        f"c.{zone} A",
        f"c.{zone} NSEC",
        f"ns3.{zone} A",
        f"ns3.{zone} NSEC",
    ]

    for query in queries:
        isctest.run.retry_with_timeout(rrsig_is_refreshed, timeout=5)


def cb_rrsig_reuse(params, ksks=None, zsks=None):
    zone = params["zone"]
    servers = params["servers"]

    isctest.log.info(f"check that the zone {zone} reuses fresh signatures")

    def rrsig_is_reused():
        parts = query.split()
        qname = parts[0]
        qtype = dns.rdatatype.from_text(parts[1])
        return isctest.kasp.verify_rrsig_is_reused(
            servers["ns3"], zone, f"ns3/{zone}.db.signed", qname, qtype, ksks, zsks
        )

    queries = [
        f"{zone} NS",
        f"{zone} NSEC",
        f"a.{zone} A",
        f"a.{zone} NSEC",
        f"b.{zone} A",
        f"b.{zone} NSEC",
        f"c.{zone} A",
        f"c.{zone} NSEC",
        f"ns3.{zone} A",
        f"ns3.{zone} NSEC",
    ]

    for query in queries:
        rrsig_is_reused()


def cb_legacy_keys(params, ksks=None, zsks=None):
    zone = params["zone"]
    keydir = params["config"]["key-directory"]

    isctest.log.info(f"check that the zone {zone} uses correct legacy keys")

    assert len(ksks) == 1
    assert len(zsks) == 1

    # This assumes the zone has a policy that dictates one KSK and one ZSK.
    # The right keys to be used are stored in "{zone}.ksk" and "{zone}.zsk".
    with open(f"{keydir}/{zone}.ksk", "r", encoding="utf-8") as file:
        kskfile = file.read()
    with open(f"{keydir}/{zone}.zsk", "r", encoding="utf-8") as file:
        zskfile = file.read()

    assert f"{keydir}/{kskfile}".strip() == ksks[0].path
    assert f"{keydir}/{zskfile}".strip() == zsks[0].path


def cb_remove_keyfiles(params, ksks=None, zsks=None):
    zone = params["zone"]
    servers = params["servers"]
    keydir = params["config"]["key-directory"]

    isctest.log.info(
        "check that removing key files does not create new keys to be generated"
    )

    for k in ksks + zsks:
        os.remove(k.keyfile)
        os.remove(k.privatefile)
        os.remove(k.statefile)

    with servers["ns3"].watch_log_from_here() as watcher:
        servers["ns3"].rndc(f"loadkeys {zone}")
        watcher.wait_for_line(
            f"zone {zone}/IN (signed): zone_rekey:zone_verifykeys failed: some key files are missing"
        )

    # Check keys again, make sure no new keys are created.
    keys = isctest.kasp.keydir_to_keylist(zone, keydir)
    isctest.kasp.check_keys(zone, keys, [])
    # Zone is still signed correctly.
    isctest.kasp.check_dnssec_verify(servers["ns3"], zone)


@pytest.mark.parametrize(
    "params",
    [
        pytest.param(
            {
                "zone": "rsasha1.kasp",
                "policy": "rsasha1",
                "config": kasp_config,
                "key-properties": rsa1_properties(5),
            },
            id="rsasha1.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "rsasha1-nsec3.kasp",
                "policy": "rsasha1",
                "config": kasp_config,
                "key-properties": rsa1_properties(7),
            },
            id="rsasha1-nsec3.kasp",
            marks=isctest.mark.with_algorithm("RSASHA1"),
        ),
        pytest.param(
            {
                "zone": "dnskey-ttl-mismatch.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": autosign_properties(
                    os.environ["DEFAULT_ALGORITHM_NUMBER"], os.environ["DEFAULT_BITS"]
                ),
            },
            id="dnskey-ttl-mismatch.autosign",
        ),
        pytest.param(
            {
                "zone": "expired-sigs.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": autosign_properties(
                    os.environ["DEFAULT_ALGORITHM_NUMBER"], os.environ["DEFAULT_BITS"]
                ),
                "additional-tests": [
                    {
                        "callback": cb_rrsig_refresh,
                        "arguments": [],
                    },
                ],
            },
            id="expired-sigs.autosign",
        ),
        pytest.param(
            {
                "zone": "fresh-sigs.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": autosign_properties(
                    os.environ["DEFAULT_ALGORITHM_NUMBER"], os.environ["DEFAULT_BITS"]
                ),
                "additional-tests": [
                    {
                        "callback": cb_rrsig_reuse,
                        "arguments": [],
                    },
                ],
            },
            id="fresh-sigs.autosign",
        ),
        pytest.param(
            {
                "zone": "unfresh-sigs.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": autosign_properties(
                    os.environ["DEFAULT_ALGORITHM_NUMBER"], os.environ["DEFAULT_BITS"]
                ),
                "additional-tests": [
                    {
                        "callback": cb_rrsig_refresh,
                        "arguments": [],
                    },
                ],
            },
            id="unfresh-sigs.autosign",
        ),
        pytest.param(
            {
                "zone": "keyfiles-missing.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": autosign_properties(
                    os.environ["DEFAULT_ALGORITHM_NUMBER"], os.environ["DEFAULT_BITS"]
                ),
                "additional-tests": [
                    {
                        "callback": cb_remove_keyfiles,
                        "arguments": [],
                    },
                ],
            },
            id="keyfiles-missing.autosign",
        ),
        pytest.param(
            {
                "zone": "ksk-missing.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": [
                    f"ksk 63072000 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent missing",
                    f"zsk 31536000 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
                ],
            },
            id="ksk-missing.autosign",
        ),
        pytest.param(
            {
                "zone": "zsk-missing.autosign",
                "policy": "autosign",
                "config": autosign_config,
                "offset": -timedelta(days=30 * 6),
                "key-properties": [
                    f"ksk 63072000 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                    f"zsk 31536000 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent missing",
                ],
            },
            id="zsk-missing.autosign",
        ),
        pytest.param(
            {
                "zone": "dnssec-keygen.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "key-properties": fips_properties(8),
            },
            id="dnssec-keygen.kasp",
        ),
        pytest.param(
            {
                "zone": "ecdsa256.kasp",
                "policy": "ecdsa256",
                "config": kasp_config,
                "key-properties": fips_properties(13, bits=256),
            },
            id="ecdsa256.kasp",
        ),
        pytest.param(
            {
                "zone": "ecdsa384.kasp",
                "policy": "ecdsa384",
                "config": kasp_config,
                "key-properties": fips_properties(14, bits=384),
            },
            id="ecdsa384.kasp",
        ),
        pytest.param(
            {
                "zone": "inherit.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "key-properties": fips_properties(8),
            },
            id="inherit.kasp",
        ),
        pytest.param(
            {
                "zone": "keystore.kasp",
                "policy": "keystore",
                "config": {
                    "dnskey-ttl": timedelta(seconds=303),
                    "ds-ttl": timedelta(days=1),
                    "key-directory": "{keydir}",
                    "max-zone-ttl": timedelta(days=1),
                    "parent-propagation-delay": timedelta(hours=1),
                    "publish-safety": timedelta(hours=1),
                    "retire-safety": timedelta(hours=1),
                    "signatures-refresh": timedelta(days=5),
                    "signatures-validity": timedelta(days=14),
                    "zone-propagation-delay": timedelta(minutes=5),
                },
                "key-directories": ["{keydir}/ksk", "{keydir}/zsk"],
                "key-properties": [
                    f"ksk unlimited {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
                    f"zsk unlimited {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
                ],
            },
            id="keystore.kasp",
        ),
        pytest.param(
            {
                "zone": "legacy-keys.kasp",
                "policy": "migrate-to-dnssec-policy",
                "config": kasp_config,
                "pregenerated": True,
                "key-properties": [
                    "ksk 16070400 8 2048 goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
                    "zsk 16070400 8 2048 goal:omnipresent dnskey:rumoured zrrsig:rumoured",
                ],
                "additional-tests": [
                    {
                        "callback": cb_legacy_keys,
                        "arguments": [],
                    },
                ],
            },
            id="legacy-keys.kasp",
        ),
        pytest.param(
            {
                "zone": "pregenerated.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "pregenerated": True,
                "key-properties": fips_properties(8),
            },
            id="pregenerated.kasp",
        ),
        pytest.param(
            {
                "zone": "rsasha256.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "key-properties": fips_properties(8),
            },
            id="rsasha256.kasp",
        ),
        pytest.param(
            {
                "zone": "rsasha512.kasp",
                "policy": "rsasha512",
                "config": kasp_config,
                "key-properties": fips_properties(10),
            },
            id="rsasha512.kasp",
        ),
        pytest.param(
            {
                "zone": "rumoured.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "rumoured": True,
                "key-properties": fips_properties(8),
            },
            id="rumoured.kasp",
        ),
        pytest.param(
            {
                "zone": "secondary.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "key-properties": fips_properties(8),
                "additional-tests": [
                    {
                        "callback": cb_ixfr_is_signed,
                        "arguments": [
                            [
                                "a.secondary.kasp. A 10.0.0.11",
                                "d.secondary.kasp. A 10.0.0.4",
                            ],
                        ],
                    },
                ],
            },
            id="secondary.kasp",
            marks=pytest.mark.flaky(
                max_runs=2, rerun_filter=isctest.mark.is_host_freebsd_13
            ),
        ),
        pytest.param(
            {
                "zone": "some-keys.kasp",
                "policy": "rsasha256",
                "config": kasp_config,
                "pregenerated": True,
                "key-properties": fips_properties(8),
            },
            id="some-keys.kasp",
        ),
        pytest.param(
            {
                "zone": "unlimited.kasp",
                "policy": "unlimited",
                "config": kasp_config,
                "key-properties": [
                    f"csk 0 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
                ],
            },
            id="unlimited.kasp",
        ),
        pytest.param(
            {
                "zone": "ed25519.kasp",
                "policy": "ed25519",
                "config": kasp_config,
                "key-properties": fips_properties(15, bits=256),
            },
            id="ed25519.kasp",
            marks=isctest.mark.with_algorithm("ED25519"),
        ),
        pytest.param(
            {
                "zone": "ed448.kasp",
                "policy": "ed448",
                "config": kasp_config,
                "key-properties": fips_properties(16, bits=456),
            },
            id="ed448.kasp",
            marks=isctest.mark.with_algorithm("ED448"),
        ),
    ],
)
def test_kasp_case(servers, ns3, params):
    # Test many different configurations and expected keys and states after
    # initial startup.
    keydir = ns3.identifier

    # Get test parameters.
    zone = params["zone"]
    policy = params["policy"]

    isctest.kasp.wait_keymgr_done(ns3, zone)

    params["config"]["key-directory"] = params["config"]["key-directory"].replace(
        "{keydir}", keydir
    )
    if "key-directories" in params:
        for i, val in enumerate(params["key-directories"]):
            params["key-directories"][i] = val.replace("{keydir}", keydir)

    ttl = int(params["config"]["dnskey-ttl"].total_seconds())
    pregenerated = False
    if params.get("pregenerated"):
        pregenerated = params["pregenerated"]
    zsk_missing = zone == "zsk-missing.autosign"

    # Test case.
    isctest.log.info(f"check test case zone {zone} policy {policy}")

    # First make sure the zone is signed.
    isctest.kasp.check_dnssec_verify(ns3, zone)

    # Key properties.
    expected = isctest.kasp.policy_to_properties(ttl=ttl, keys=params["key-properties"])
    # Key files.
    if "key-directories" in params:
        kdir = params["key-directories"][0]
        ksks = isctest.kasp.keydir_to_keylist(zone, kdir, in_use=pregenerated)
        kdir = params["key-directories"][1]
        zsks = isctest.kasp.keydir_to_keylist(zone, kdir, in_use=pregenerated)
        keys = ksks + zsks
    else:
        keys = isctest.kasp.keydir_to_keylist(
            zone, params["config"]["key-directory"], in_use=pregenerated
        )
        ksks = [k for k in keys if k.is_ksk()]
        zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)

    offset = params["offset"] if "offset" in params else None

    for kp in expected:
        kp.set_expected_keytimes(
            params["config"], offset=offset, pregenerated=pregenerated
        )

    if "rumoured" not in params:
        isctest.kasp.check_keytimes(keys, expected)

    check_all(ns3, zone, policy, ksks, zsks, zsk_missing=zsk_missing)

    if "additional-tests" in params:
        params["servers"] = servers
        for additional_test in params["additional-tests"]:
            callback = additional_test["callback"]
            arguments = additional_test["arguments"]
            callback(*arguments, params=params, ksks=ksks, zsks=zsks)


@pytest.mark.parametrize(
    "zone, server_id, tsig_kind",
    [
        param("unsigned.tld", "ns2", None),
        param("none.inherit.signed", "ns4", "sha1"),
        param("none.override.signed", "ns4", "sha224"),
        param("inherit.none.signed", "ns4", "sha256"),
        param("none.none.signed", "ns4", "sha256"),
        param("inherit.inherit.unsigned", "ns5", "sha1"),
        param("none.inherit.unsigned", "ns5", "sha1"),
        param("none.override.unsigned", "ns5", "sha224"),
        param("inherit.none.unsigned", "ns5", "sha256"),
        param("none.none.unsigned", "ns5", "sha256"),
    ],
)
def test_kasp_inherit_unsigned(zone, server_id, tsig_kind, servers):
    server = servers[server_id]
    tsig = (
        f"hmac-{tsig_kind}:{tsig_kind}:{KASP_INHERIT_TSIG_SECRET[tsig_kind]}"
        if tsig_kind
        else None
    )

    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)
    isctest.kasp.check_keys(zone, keys, [])
    isctest.kasp.check_dnssecstatus(server, zone, [])
    isctest.kasp.check_apex(server, zone, [], [], tsig=tsig)
    isctest.kasp.check_subdomain(server, zone, [], [], tsig=tsig)


@pytest.mark.parametrize(
    "zone, policy, server_id, alg, tsig_kind",
    [
        param("signed.tld", "default", "ns2", ECDSAP256SHA256, None),
        param("override.inherit.signed", "default", "ns4", ECDSAP256SHA256, "sha1"),
        param("inherit.override.signed", "default", "ns4", ECDSAP256SHA256, "sha224"),
        param("override.inherit.unsigned", "default", "ns5", ECDSAP256SHA256, "sha1"),
        param("inherit.override.unsigned", "default", "ns5", ECDSAP256SHA256, "sha224"),
        param("inherit.inherit.signed", "test", "ns4", ECDSAP384SHA384, "sha1"),
        param("override.override.signed", "test", "ns4", ECDSAP384SHA384, "sha224"),
        param("override.none.signed", "test", "ns4", ECDSAP384SHA384, "sha256"),
        param("override.override.unsigned", "test", "ns5", ECDSAP384SHA384, "sha224"),
        param("override.none.unsigned", "test", "ns5", ECDSAP384SHA384, "sha256"),
    ],
)
def test_kasp_inherit_signed(zone, policy, server_id, alg, tsig_kind, servers):
    server = servers[server_id]
    tsig = (
        f"hmac-{tsig_kind}:{tsig_kind}:{KASP_INHERIT_TSIG_SECRET[tsig_kind]}"
        if tsig_kind
        else None
    )

    isctest.kasp.wait_keymgr_done(server, zone)

    key1 = KeyProperties.default()
    key1.metadata["Algorithm"] = alg.number
    key1.metadata["Length"] = alg.bits
    keys = isctest.kasp.keydir_to_keylist(zone, server.identifier)

    isctest.kasp.check_dnssec_verify(server, zone, tsig=tsig)
    isctest.kasp.check_keys(zone, keys, [key1])
    set_keytimes_default_policy(key1)
    isctest.kasp.check_keytimes(keys, [key1])
    check_all(server, zone, policy, keys, [], tsig=tsig)


@pytest.mark.parametrize(
    "number, dynamic, inline_signing, txt_rdata",
    [
        param("1", "yes", "no", "view1"),
        param("2", "no", "yes", "view2"),
        param("3", "no", "yes", "view2"),
    ],
)
def test_kasp_inherit_view(number, dynamic, inline_signing, txt_rdata, ns4):
    zone = "example.net"
    policy = "test"
    view = f"example{number}"
    tsig = f"{os.environ['DEFAULT_HMAC']}:keyforview{number}:{KASP_INHERIT_TSIG_SECRET[f'view{number}']}"

    isctest.kasp.wait_keymgr_done(ns4, zone)

    key1 = KeyProperties.default()
    key1.metadata["Algorithm"] = ECDSAP384SHA384.number
    key1.metadata["Length"] = ECDSAP384SHA384.bits
    keys = isctest.kasp.keydir_to_keylist(zone, ns4.identifier)

    isctest.kasp.check_dnssec_verify(ns4, zone, tsig=tsig)
    isctest.kasp.check_keys(zone, keys, [key1])
    set_keytimes_default_policy(key1)
    isctest.kasp.check_keytimes(keys, [key1])
    isctest.kasp.check_dnssecstatus(ns4, zone, keys, policy=policy, view=view)
    isctest.kasp.check_apex(ns4, zone, keys, [], tsig=tsig)
    # check zonestatus
    response = ns4.rndc(f"zonestatus {zone} in {view}")
    assert f"dynamic: {dynamic}" in response.out
    assert f"inline signing: {inline_signing}" in response.out
    # check subdomain
    fqdn = f"{zone}."
    qname = f"view.{zone}."
    qtype = dns.rdatatype.TXT
    rdata = txt_rdata
    query = isctest.query.create(qname, qtype)
    tsigkey = tsig.split(":")
    keyring = dns.tsig.Key(tsigkey[1], tsigkey[2], tsigkey[0])
    query.use_tsig(keyring)
    try:
        response = isctest.query.tcp(query, ns4.ip, ns4.ports.dns, timeout=3)
    except dns.exception.Timeout:
        isctest.log.debug(f"query timeout for query {qname} {qtype} to {ns4.ip}")
        response = None
    assert response.rcode() == dns.rcode.NOERROR
    match = f'{qname} 300 IN TXT "{rdata}"'
    rrsigs = []
    for rrset in response.answer:
        if rrset.match(
            dns.name.from_text(qname), dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype
        ):
            rrsigs.append(rrset)
        else:
            assert match in rrset.to_text()
    assert len(rrsigs) > 0
    isctest.kasp.check_signatures(rrsigs, qtype, fqdn, keys, [])


def test_kasp_default(ns3):
    # check the zone with default kasp policy has loaded and is signed.
    isctest.log.info("check a zone with the default policy is signed")
    zone = "default.kasp"
    policy = "default"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Key properties.
    # DNSKEY, RRSIG (ksk), RRSIG (zsk) are published. DS needs to wait.
    keyprops = [
        "csk 0 13 256 goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
    ]
    expected = isctest.kasp.policy_to_properties(ttl=3600, keys=keyprops)
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    set_keytimes_default_policy(expected[0])
    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, keys, [])

    # Trigger a keymgr run. Make sure the key files are not touched if there
    # are no modifications to the key metadata.
    isctest.log.info(
        "check that key files are untouched if there are no metadata changes"
    )
    key = keys[0]
    privkey_stat = os.stat(key.privatefile)
    pubkey_stat = os.stat(key.keyfile)
    state_stat = os.stat(key.statefile)

    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    assert privkey_stat.st_mtime == os.stat(key.privatefile).st_mtime
    assert pubkey_stat.st_mtime == os.stat(key.keyfile).st_mtime
    assert state_stat.st_mtime == os.stat(key.statefile).st_mtime

    # again
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    assert privkey_stat.st_mtime == os.stat(key.privatefile).st_mtime
    assert pubkey_stat.st_mtime == os.stat(key.keyfile).st_mtime
    assert state_stat.st_mtime == os.stat(key.statefile).st_mtime

    # modify unsigned zone file and check that new record is signed.
    isctest.log.info("check that an updated zone signs the new record")
    shutil.copyfile("ns3/template2.db.in", f"ns3/{zone}.db")
    ns3.rndc(f"reload {zone}")

    def update_is_signed():
        parts = update.split()
        qname = parts[0]
        qtype = dns.rdatatype.from_text(parts[1])
        rdata = parts[2]
        return isctest.kasp.verify_update_is_signed(
            ns3, zone, qname, qtype, rdata, keys, []
        )

    expected_updates = [f"a.{zone}. A 10.0.0.11", f"d.{zone}. A 10.0.0.44"]
    for update in expected_updates:
        isctest.run.retry_with_timeout(update_is_signed, timeout=5)

    # Move the private key file, a rekey event should not introduce
    # replacement keys.
    isctest.log.info("check that missing private key doesn't trigger rollover")
    shutil.move(f"{key.privatefile}", f"{key.path}.offline")
    expectmsg = "zone_rekey:zone_verifykeys failed: some key files are missing"
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"zone {zone}/IN (signed): {expectmsg}")
    # Nothing has changed.
    expected[0].private = False  # noqa
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, keys, [])

    # A zone that uses inline-signing.
    isctest.log.info("check an inline-signed zone with the default policy is signed")
    zone = "inline-signing.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Key properties.
    key1 = KeyProperties.default()
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    expected = [key1]
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    set_keytimes_default_policy(key1)
    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, keys, [])


def test_kasp_dynamic(ns3):
    # Standard dynamic zone.
    isctest.log.info("check dynamic zone is updated and signed after update")
    zone = "dynamic.kasp"
    policy = "default"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Key properties.
    key1 = KeyProperties.default()
    expected = [key1]
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    set_keytimes_default_policy(key1)
    expected = [key1]
    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, keys, [])

    def update_is_signed():
        parts = update.split()
        qname = parts[0]
        qtype = dns.rdatatype.from_text(parts[1])
        rdata = parts[2]
        return isctest.kasp.verify_update_is_signed(
            ns3, zone, qname, qtype, rdata, keys, []
        )

    update_msg = dns.update.UpdateMessage(zone)
    update_msg.delete(f"a.{zone}.", "A", "10.0.0.1")
    update_msg.add(f"a.{zone}.", 300, "A", "10.0.0.101")
    update_msg.add(f"d.{zone}.", 300, "A", "10.0.0.4")
    ns3.nsupdate(update_msg)

    expected_updates = [f"a.{zone}. A 10.0.0.101", f"d.{zone}. A 10.0.0.4"]
    for update in expected_updates:
        isctest.run.retry_with_timeout(update_is_signed, timeout=5)

    # Update zone (reverting the above change).
    update_msg = dns.update.UpdateMessage(zone)
    update_msg.add(f"a.{zone}.", 300, "A", "10.0.0.1")
    update_msg.delete(f"a.{zone}.", "A", "10.0.0.101")
    update_msg.delete(f"d.{zone}.", "A", "10.0.0.4")
    ns3.nsupdate(update_msg)

    update = f"a.{zone}. A 10.0.0.1"
    isctest.run.retry_with_timeout(update_is_signed, timeout=5)

    # Update zone with freeze/thaw.
    isctest.log.info("check dynamic zone is updated and signed after freeze and thaw")
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"freeze {zone}")
        watcher.wait_for_line(f"freezing zone '{zone}/IN': success")

    time.sleep(1)
    with open(f"ns3/{zone}.db", "a", encoding="utf-8") as zonefile:
        zonefile.write(f"d.{zone}. 300 A 10.0.0.44\n")
    time.sleep(1)

    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"thaw {zone}")
        watcher.wait_for_line(f"thawing zone '{zone}/IN': success")

    expected_updates = [f"a.{zone}. A 10.0.0.1", f"d.{zone}. A 10.0.0.44"]

    for update in expected_updates:
        isctest.run.retry_with_timeout(update_is_signed, timeout=5)

    # Dynamic, and inline-signing.
    zone = "dynamic-inline-signing.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Key properties.
    key1 = KeyProperties.default()
    expected = [key1]
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    set_keytimes_default_policy(key1)
    expected = [key1]
    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, keys, [])

    # Update zone with freeze/thaw.
    isctest.log.info(
        "check dynamic inline-signed zone is updated and signed after freeze and thaw"
    )
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"freeze {zone}")
        watcher.wait_for_line(f"freezing zone '{zone}/IN': success")

    time.sleep(1)
    shutil.copyfile("ns3/template2.db.in", f"ns3/{zone}.db")
    time.sleep(1)

    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"thaw {zone}")
        watcher.wait_for_line(f"thawing zone '{zone}/IN': success")

    expected_updates = [f"a.{zone}. A 10.0.0.11", f"d.{zone}. A 10.0.0.44"]
    for update in expected_updates:
        isctest.run.retry_with_timeout(update_is_signed, timeout=5)

    # Dynamic, signed, and inline-signing.
    isctest.log.info("check dynamic signed, and inline-signed zone")
    zone = "dynamic-signed-inline-signing.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # Key properties.
    key1 = KeyProperties.default()
    # The ns3/setup.sh script sets all states to omnipresent.
    key1.metadata["DNSKEYState"] = "omnipresent"
    key1.metadata["KRRSIGState"] = "omnipresent"
    key1.metadata["ZRRSIGState"] = "omnipresent"
    key1.metadata["DSState"] = "omnipresent"
    expected = [key1]
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3/keys")
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    check_all(ns3, zone, policy, keys, [])
    # Ensure no zone_resigninc for the unsigned version of the zone is triggered.
    assert f"zone_resigninc: zone {zone}/IN (unsigned): enter" not in "ns3/named.run"


def test_kasp_checkds(ns3):
    def wait_for_metadata():
        return isctest.util.file_contents_contain(ksk.statefile, metadata)

    # Zone: checkds-ksk.kasp.
    zone = "checkds-ksk.kasp"
    policy = "checkds-ksk"
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    policy_keys = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
    ]

    isctest.kasp.wait_keymgr_done(ns3, zone)

    expected = isctest.kasp.policy_to_properties(ttl=303, keys=policy_keys)
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if k.is_zsk()]
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    check_all(ns3, zone, policy, ksks, zsks)

    now = KeyTimingMetadata.now()
    ksk = ksks[0]

    isctest.log.info("check if checkds -publish correctly sets DSPublish")
    ns3.rndc(f"dnssec -checkds -when {now} published {zone}")
    metadata = f"DSPublish: {now}"
    isctest.run.retry_with_timeout(wait_for_metadata, timeout=3)
    expected[0].metadata["DSState"] = "rumoured"
    expected[0].timing["DSPublish"] = now
    isctest.kasp.check_keys(zone, keys, expected)

    isctest.log.info("check if checkds -withdrawn correctly sets DSRemoved")
    ns3.rndc(f"dnssec -checkds -when {now} withdrawn {zone}")
    metadata = f"DSRemoved: {now}"
    isctest.run.retry_with_timeout(wait_for_metadata, timeout=3)
    expected[0].metadata["DSState"] = "unretentive"
    expected[0].timing["DSRemoved"] = now
    isctest.kasp.check_keys(zone, keys, expected)


def test_kasp_checkds_doubleksk(ns3):
    def wait_for_metadata():
        return isctest.util.file_contents_contain(ksk.statefile, metadata)

    # Zone: checkds-doubleksk.kasp.
    zone = "checkds-doubleksk.kasp"
    policy = "checkds-doubleksk"
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    policy_keys = [
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"ksk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
        f"zsk unlimited {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
    ]

    isctest.kasp.wait_keymgr_done(ns3, zone)

    expected = isctest.kasp.policy_to_properties(ttl=303, keys=policy_keys)
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if k.is_zsk()]
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    check_all(ns3, zone, policy, ksks, zsks)

    now = KeyTimingMetadata.now()
    ksk = ksks[0]

    badalg = os.environ["ALTERNATIVE_ALGORITHM_NUMBER"]
    isctest.log.info("check invalid checkds commands")

    def check_error():
        response = ns3.rndc(test["command"], stderr=subprocess.STDOUT)
        assert test["error"] in response.out

    test_cases = [
        {
            "command": f"dnssec -checkds -when {now} published {zone}",
            "error": "multiple possible keys found, retry command with -key id",
        },
        {
            "command": f"dnssec -checkds -when {now} withdrawn {zone}",
            "error": "multiple possible keys found, retry command with -key id",
        },
        {
            "command": f"dnssec -checkds -when {now} -key {ksks[0].tag} -alg {badalg} published {zone}",
            "error": "Error executing checkds command: no matching key found",
        },
        {
            "command": f"dnssec -checkds -when {now} -key {ksks[0].tag} -alg {badalg} withdrawn {zone}",
            "error": "Error executing checkds command: no matching key found",
        },
    ]
    for test in test_cases:
        check_error()

    isctest.log.info("check if checkds -publish -key correctly sets DSPublish")
    ns3.rndc(f"dnssec -checkds -when {now} -key {ksk.tag} published {zone}")
    metadata = f"DSPublish: {now}"
    isctest.run.retry_with_timeout(wait_for_metadata, timeout=3)
    expected[0].metadata["DSState"] = "rumoured"
    expected[0].timing["DSPublish"] = now
    isctest.kasp.check_keys(zone, keys, expected)

    isctest.log.info("check if checkds -withdrawn -key correctly sets DSRemoved")
    ksk = ksks[1]
    ns3.rndc(f"dnssec -checkds -when {now} -key {ksk.tag} withdrawn {zone}")
    metadata = f"DSRemoved: {now}"
    isctest.run.retry_with_timeout(wait_for_metadata, timeout=3)
    expected[1].metadata["DSState"] = "unretentive"
    expected[1].timing["DSRemoved"] = now
    isctest.kasp.check_keys(zone, keys, expected)


def test_kasp_checkds_csk(ns3):
    def wait_for_metadata():
        return isctest.util.file_contents_contain(ksk.statefile, metadata)

    # Zone: checkds-csk.kasp.
    zone = "checkds-csk.kasp"
    policy = "checkds-csk"
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    policy_keys = [
        f"csk unlimited {alg} {size} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:hidden",
    ]

    isctest.kasp.wait_keymgr_done(ns3, zone)

    expected = isctest.kasp.policy_to_properties(ttl=303, keys=policy_keys)
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)
    check_all(ns3, zone, policy, keys, [])

    now = KeyTimingMetadata.now()
    ksk = keys[0]

    isctest.log.info("check if checkds -publish csk correctly sets DSPublish")
    ns3.rndc(f"dnssec -checkds -when {now} published {zone}")
    metadata = f"DSPublish: {now}"
    isctest.run.retry_with_timeout(wait_for_metadata, timeout=3)
    expected[0].metadata["DSState"] = "rumoured"
    expected[0].timing["DSPublish"] = now
    isctest.kasp.check_keys(zone, keys, expected)

    isctest.log.info("check if checkds -withdrawn csk correctly sets DSRemoved")
    ns3.rndc(f"dnssec -checkds -when {now} withdrawn {zone}")
    metadata = f"DSRemoved: {now}"
    isctest.run.retry_with_timeout(wait_for_metadata, timeout=3)
    expected[0].metadata["DSState"] = "unretentive"
    expected[0].timing["DSRemoved"] = now
    isctest.kasp.check_keys(zone, keys, expected)


def test_kasp_special_characters(ns3):
    # A zone with special characters.
    isctest.log.info("check special characters")

    zone = r"i-am.\":\;?&[]\@!\$*+,|=\.\(\)special.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    # It is non-trivial to adapt the tests to deal with all possible different
    # escaping characters, so we will just try to verify the zone.
    isctest.kasp.check_dnssec_verify(ns3, zone)


def test_kasp_insecure(ns3):
    # Insecure zones.
    isctest.log.info("check insecure zones")

    zone = "insecure.kasp"

    isctest.kasp.wait_keymgr_done(ns3, zone)

    expected = []
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_dnssecstatus(ns3, zone, keys, policy="insecure")
    isctest.kasp.check_apex(ns3, zone, keys, [])
    isctest.kasp.check_subdomain(ns3, zone, keys, [])

    zone = "unsigned.kasp"
    expected = []
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_dnssecstatus(ns3, zone, keys, policy=None)
    isctest.kasp.check_apex(ns3, zone, keys, [])
    isctest.kasp.check_subdomain(ns3, zone, keys, [])
    # Make sure the zone file is untouched.
    isctest.check.file_contents_equal(f"ns3/{zone}.db.infile", f"ns3/{zone}.db")


def test_kasp_bad_maxzonettl(ns3):
    isctest.log.info("check max-zone-ttl rejects zones with too high TTL")
    zone = "max-zone-ttl.kasp"
    assert f"loading from master file {zone}.db failed: out of range" in ns3.log


def test_kasp_dnssec_keygen():
    def keygen(zone, policy, keydir=None):
        if keydir is None:
            keydir = "."

        keygen_command = [
            os.environ.get("KEYGEN"),
            "-K",
            keydir,
            "-k",
            policy,
            "-l",
            "kasp.conf",
            zone,
        ]

        return isctest.run.cmd(keygen_command).out

    isctest.log.info(
        "check that 'dnssec-keygen -k' (configured policy) created valid files"
    )

    keyprops = [
        f"csk {lifetime['P1Y']} 13 256",
        f"ksk {lifetime['P1Y']} 8 2048",
        f"zsk {lifetime['P30D']} 8 2048",
        f"zsk {lifetime['P6M']} 8 3072",
    ]
    keydir = "keys"
    out = keygen("kasp", "kasp", keydir)
    keys = isctest.kasp.keystr_to_keylist(out, keydir)
    expected = isctest.kasp.policy_to_properties(ttl=200, keys=keyprops)
    isctest.kasp.check_keys("kasp", keys, expected)

    isctest.log.info(
        "check that 'dnssec-keygen -k' (default policy) created valid files"
    )

    keyprops = ["csk 0 13 256"]
    out = keygen("kasp", "default")
    keys = isctest.kasp.keystr_to_keylist(out)
    expected = isctest.kasp.policy_to_properties(ttl=3600, keys=keyprops)
    isctest.kasp.check_keys("kasp", keys, expected)

    isctest.log.info(
        "check that 'dnssec-settime' by default does not edit key state file"
    )

    key = keys[0]
    shutil.copyfile(key.privatefile, f"{key.privatefile}.backup")
    shutil.copyfile(key.keyfile, f"{key.keyfile}.backup")
    shutil.copyfile(key.statefile, f"{key.statefile}.backup")

    created = key.get_timing("Created")
    publish = key.get_timing("Publish") + timedelta(hours=1)
    settime = [
        os.environ.get("SETTIME"),
        "-P",
        str(publish),
        key.path,
    ]
    isctest.run.cmd(settime)

    isctest.check.file_contents_equal(f"{key.statefile}", f"{key.statefile}.backup")
    assert key.get_metadata("Publish", file=key.privatefile) == str(publish)
    assert key.get_metadata("Publish", file=key.keyfile, comment=True) == str(publish)

    isctest.log.info(
        "check that 'dnssec-settime -s' also sets publish time metadata and states in key state file"
    )

    now = KeyTimingMetadata.now()
    goal = "omnipresent"
    dnskey = "rumoured"
    krrsig = "rumoured"
    zrrsig = "omnipresent"
    ds = "hidden"
    keyprops = [
        f"csk 0 13 256 goal:{goal} dnskey:{dnskey} krrsig:{krrsig} zrrsig:{zrrsig} ds:{ds}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl=3600, keys=keyprops)
    expected[0].timing = {
        "Generated": created,
        "Published": now,
        "Active": created,
        "DNSKEYChange": now,
        "KRRSIGChange": now,
        "ZRRSIGChange": now,
        "DSChange": now,
    }

    settime = [
        os.environ.get("SETTIME"),
        "-s",
        "-P",
        str(now),
        "-g",
        goal,
        "-k",
        dnskey,
        str(now),
        "-r",
        krrsig,
        str(now),
        "-z",
        zrrsig,
        str(now),
        "-d",
        ds,
        str(now),
        key.path,
    ]
    isctest.run.cmd(settime)
    isctest.kasp.check_keys("kasp", keys, expected)
    isctest.kasp.check_keytimes(keys, expected)

    isctest.log.info(
        "check that 'dnssec-settime -s' also unsets publish time metadata and states in key state file"
    )

    now = KeyTimingMetadata.now()
    keyprops = ["csk 0 13 256"]
    expected = isctest.kasp.policy_to_properties(ttl=3600, keys=keyprops)
    expected[0].timing = {
        "Generated": created,
        "Active": created,
    }

    settime = [
        os.environ.get("SETTIME"),
        "-s",
        "-P",
        "none",
        "-g",
        "none",
        "-k",
        "none",
        str(now),
        "-z",
        "none",
        str(now),
        "-r",
        "none",
        str(now),
        "-d",
        "none",
        str(now),
        key.path,
    ]
    isctest.run.cmd(settime)
    isctest.kasp.check_keys("kasp", keys, expected)
    isctest.kasp.check_keytimes(keys, expected)

    isctest.log.info(
        "check that 'dnssec-settime -s' also sets active time metadata and states in key state file (uppercase)"
    )

    soon = now + timedelta(hours=2)
    goal = "hidden"
    dnskey = "unretentive"
    krrsig = "omnipresent"
    zrrsig = "unretentive"
    ds = "omnipresent"
    keyprops = [
        f"csk 0 13 256 goal:{goal} dnskey:{dnskey} krrsig:{krrsig} zrrsig:{zrrsig} ds:{ds}",
    ]
    expected = isctest.kasp.policy_to_properties(ttl=3600, keys=keyprops)
    expected[0].timing = {
        "Generated": created,
        "Active": soon,
        "DNSKEYChange": soon,
        "KRRSIGChange": soon,
        "ZRRSIGChange": soon,
        "DSChange": soon,
    }

    settime = [
        os.environ.get("SETTIME"),
        "-s",
        "-A",
        str(soon),
        "-g",
        "HIDDEN",
        "-k",
        "UNRETENTIVE",
        str(soon),
        "-z",
        "UNRETENTIVE",
        str(soon),
        "-r",
        "OMNIPRESENT",
        str(soon),
        "-d",
        "OMNIPRESENT",
        str(soon),
        key.path,
    ]
    isctest.run.cmd(settime)
    isctest.kasp.check_keys("kasp", keys, expected)
    isctest.kasp.check_keytimes(keys, expected)


def test_kasp_zsk_retired(ns3):
    config = {
        "dnskey-ttl": timedelta(seconds=300),
        "ds-ttl": timedelta(days=1),
        "max-zone-ttl": timedelta(days=1),
        "parent-propagation-delay": timedelta(hours=1),
        "publish-safety": timedelta(hours=1),
        "retire-safety": timedelta(hours=1),
        "signatures-refresh": timedelta(days=7),
        "signatures-validity": timedelta(days=14),
        "zone-propagation-delay": timedelta(minutes=5),
    }

    zone = "zsk-retired.autosign"
    policy = "autosign"
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    key_properties = [
        f"ksk 63072000 {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        # zsk predecessor
        f"zsk 31536000 {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent",
        # zsk successor
        f"zsk 31536000 {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden",
    ]

    isctest.kasp.wait_keymgr_done(ns3, zone)

    expected = isctest.kasp.policy_to_properties(300, key_properties)
    keys = isctest.kasp.keydir_to_keylist(zone, "ns3")
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)

    offset = -timedelta(days=30 * 6)
    sign_delay = config["signatures-validity"] - config["signatures-refresh"]

    def sumvars(variables):
        result = timedelta(0)
        for var in variables:
            result = result + config[var]
        return result

    # KSK Key Timings:
    # IpubC = DprpC + TTLkey
    # Note: Also need to wait until the signatures are omnipresent.
    # That's why we use max-zone-ttl instead of dnskey-ttl here.
    Ipub_KSK = sumvars(["zone-propagation-delay", "max-zone-ttl"])
    # Iret = DprpP + TTLds
    Iret_KSK = sumvars(["parent-propagation-delay", "retire-safety", "ds-ttl"])

    # ZSK Key Timings:
    # Ipub = Dprp + TTLkey
    Ipub_ZSK = sumvars(["zone-propagation-delay", "publish-safety", "dnskey-ttl"])
    # Iret = Dsgn + Dprp + TTLsig
    Iret_ZSK = sumvars(["zone-propagation-delay", "retire-safety", "max-zone-ttl"])
    Iret_ZSK = Iret_ZSK + sign_delay

    # KSK
    expected[0].timing["Generated"] = expected[0].key.get_timing("Created")
    expected[0].timing["Published"] = expected[0].timing["Generated"]
    expected[0].timing["Published"] = expected[0].timing["Published"] + offset
    expected[0].timing["Active"] = expected[0].timing["Published"]
    expected[0].timing["Retired"] = expected[0].timing["Published"] + int(
        expected[0].metadata["Lifetime"]
    )
    # Trdy(N) = Tpub(N) + IpubC
    expected[0].timing["PublishCDS"] = expected[0].timing["Published"] + Ipub_KSK
    # Tdea(N) = Tret(N) + Iret
    expected[0].timing["Removed"] = expected[0].timing["Retired"] + Iret_KSK
    expected[0].timing["DNSKEYChange"] = None
    expected[0].timing["DSChange"] = None
    expected[0].timing["KRRSIGChange"] = None

    # ZSK (predecessor)
    expected[1].timing["Generated"] = expected[1].key.get_timing("Created")
    expected[1].timing["Published"] = expected[1].timing["Generated"] + offset
    expected[1].timing["Active"] = expected[1].timing["Published"]
    expected[1].timing["Retired"] = expected[1].timing["Generated"]
    # Tdea(N) = Tret(N) + Iret
    expected[1].timing["Removed"] = expected[1].timing["Retired"] + Iret_ZSK
    expected[1].timing["DNSKEYChange"] = None
    expected[1].timing["ZRRSIGChange"] = None

    # ZSK (successor)
    expected[2].timing["Generated"] = expected[2].key.get_timing("Created")
    expected[2].timing["Published"] = expected[2].timing["Generated"]
    # Trdy(N) = Tpub(N) + Ipub
    expected[2].timing["Active"] = expected[2].timing["Published"] + Ipub_ZSK
    # Tret(N) = Tact(N) + Lzsk
    expected[2].timing["Retired"] = expected[2].timing["Active"] + int(
        expected[2].metadata["Lifetime"]
    )
    # Tdea(N) = Tret(N) + Iret
    expected[2].timing["Removed"] = expected[2].timing["Retired"] + Iret_ZSK
    expected[2].timing["DNSKEYChange"] = None
    expected[2].timing["ZRRSIGChange"] = None

    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, ksks, zsks)

    queries = [
        f"{zone} DNSKEY",
        f"{zone} SOA",
        f"{zone} NS",
        f"{zone} NSEC",
        f"a.{zone} A",
        f"a.{zone} NSEC",
        f"b.{zone} A",
        f"b.{zone} NSEC",
        f"c.{zone} A",
        f"c.{zone} NSEC",
        f"ns3.{zone} A",
        f"ns3.{zone} NSEC",
    ]

    def rrsig_is_refreshed():
        parts = query.split()
        qname = parts[0]
        qtype = dns.rdatatype.from_text(parts[1])
        return isctest.kasp.verify_rrsig_is_refreshed(
            ns3, zone, f"ns3/{zone}.db.signed", qname, qtype, ksks, zsks
        )

    for query in queries:
        isctest.run.retry_with_timeout(rrsig_is_refreshed, timeout=5)

    # Load again, make sure the purged key is not an issue when verifying keys.
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    msg = f"zone {zone}/IN (signed): zone_rekey:zone_verifykeys failed: some key files are missing"
    assert msg not in ns3.log


def test_kasp_purge_keys(ns4):
    zone = "purgekeys.kasp"
    tsig1 = (
        f"{os.environ['DEFAULT_HMAC']}:keyforview1:{KASP_INHERIT_TSIG_SECRET['view1']}"
    )
    tsig2 = (
        f"{os.environ['DEFAULT_HMAC']}:keyforview2:{KASP_INHERIT_TSIG_SECRET['view2']}"
    )

    isctest.kasp.wait_keymgr_done(ns4, zone)

    isctest.kasp.check_dnssec_verify(ns4, zone, tsig=tsig1)
    isctest.kasp.check_dnssec_verify(ns4, zone, tsig=tsig2)

    # Reconfig, make sure the purged key is not an issue when verifying keys.
    shutil.copyfile("ns4/purgekeys2.conf", "ns4/purgekeys.conf")
    with ns4.watch_log_from_here() as watcher:
        ns4.rndc("reconfig")
        watcher.wait_for_line(f"keymgr: {zone} done")

    msg = f"zone {zone}/IN/example1 (signed): zone_rekey:zone_verifykeys failed: some key files are missing"
    assert msg not in ns4.log

    msg = f"zone {zone}/IN/example2 (signed): zone_rekey:zone_verifykeys failed: some key files are missing"
    assert msg not in ns4.log


def test_kasp_reload_restart(ns6):
    zone = "example"

    def query_soa(qname):
        fqdn = dns.name.from_text(qname)
        qtype = dns.rdatatype.SOA
        query = isctest.query.create(fqdn, qtype)
        try:
            response = isctest.query.tcp(query, ns6.ip, ns6.ports.dns, timeout=3)
        except dns.exception.Timeout:
            isctest.log.debug(f"query timeout for query {qname} SOA to {ns6.ip}")
            return 0, 0

        assert response.rcode() == dns.rcode.NOERROR

        for rr in response.answer:
            if rr.match(fqdn, dns.rdataclass.IN, dns.rdatatype.RRSIG, qtype):
                continue

            assert rr.match(fqdn, dns.rdataclass.IN, qtype, dns.rdatatype.NONE)
            assert len(rr) == 1
            return rr[0].serial, rr.ttl

        return 0, 0

    def check_soa_ttl():
        soa2, ttl2 = query_soa(zone)
        return soa1 < soa2 and ttl2 == newttl

    # Check that the SOA SERIAL increases and check the TTLs (should be 300 as
    # defined in ns6/example2.db.in).
    soa1, ttl1 = query_soa(zone)
    assert ttl1 == 300

    shutil.copyfile(f"ns6/{zone}2.db.in", f"ns6/{zone}.db")
    with ns6.watch_log_from_here() as watcher:
        ns6.rndc("reload")
        watcher.wait_for_line("all zones loaded")

    newttl = 300
    isctest.run.retry_with_timeout(check_soa_ttl, timeout=10)

    # Check that the SOA SERIAL increases and check the TTLs (should be changed
    # from 300 to 400 as defined in ns6/example3.db.in).
    soa1, ttl1 = query_soa(zone)
    assert ttl1 == 300

    ns6.stop()
    shutil.copyfile(f"ns6/{zone}3.db.in", f"ns6/{zone}.db")
    os.unlink(f"ns6/{zone}.db.jnl")
    with ns6.watch_log_from_here() as watcher:
        ns6.start(["--noclean", "--restart", "--port", os.environ["PORT"]])
        watcher.wait_for_line("all zones loaded")

    newttl = 400
    isctest.run.retry_with_timeout(check_soa_ttl, timeout=10)


def test_kasp_manual_mode(ns3):

    keydir = ns3.identifier
    zone = "keyfiles-missing.manual"
    policy = "manual"
    ttl = int(autosign_config["dnskey-ttl"].total_seconds())
    offset = -timedelta(days=30 * 6)
    alg = os.environ["DEFAULT_ALGORITHM_NUMBER"]
    size = os.environ["DEFAULT_BITS"]
    keyprops = [
        f"ksk {lifetime['P2Y']} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk {lifetime['P2M']} {alg} {size} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]

    isctest.kasp.wait_keymgr_done(ns3, zone)

    isctest.log.info(f"check test case zone {zone} policy {policy}")

    # First make sure the zone is signed.
    isctest.kasp.check_dnssec_verify(ns3, zone)

    # Key properties.
    expected = isctest.kasp.policy_to_properties(ttl=ttl, keys=keyprops)

    # Key files.
    keys = isctest.kasp.keydir_to_keylist(zone, keydir)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_dnssec_verify(ns3, zone)
    isctest.kasp.check_keys(zone, keys, expected)

    for kp in expected:
        kp.set_expected_keytimes(autosign_config, offset=offset)

    isctest.kasp.check_keytimes(keys, expected)

    check_all(ns3, zone, policy, ksks, zsks, manual_mode=True)

    # Key rollover should have been be blocked.
    tag = expected[1].key.tag
    blockmsg = f"keymgr-manual-mode: block ZSK rollover for key {zone}/ECDSAP256SHA256/{tag} (policy {policy})"
    assert blockmsg in ns3.log

    # Remove files.
    for key in ksks + zsks:
        shutil.copyfile(key.privatefile, f"{key.privatefile}.backup")
        shutil.copyfile(key.keyfile, f"{key.keyfile}.backup")
        shutil.copyfile(key.statefile, f"{key.statefile}.backup")

        os.remove(key.keyfile)
        os.remove(key.privatefile)
        os.remove(key.statefile)

    # Force step.
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -step {zone}")
        watcher.wait_for_line(
            f"zone {zone}/IN (signed): zone_rekey:zone_verifykeys failed: some key files are missing"
        )

    # Restore key files.
    for key in ksks + zsks:
        shutil.copyfile(f"{key.privatefile}.backup", key.privatefile)
        shutil.copyfile(f"{key.keyfile}.backup", key.keyfile)
        shutil.copyfile(f"{key.statefile}.backup", key.statefile)

    # Load keys.
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(blockmsg)

    # Check keys again, make sure no new keys are created.
    isctest.kasp.check_keys(zone, keys, expected)
    isctest.kasp.check_keytimes(keys, expected)
    check_all(ns3, zone, policy, ksks, zsks, manual_mode=True)
    isctest.kasp.check_dnssec_verify(ns3, zone)

    # Force step.
    with ns3.watch_log_from_here() as watcher:
        ns3.rndc(f"dnssec -step {zone}")
        watcher.wait_for_line(
            f"zone {zone}/IN (signed): zone_rekey done: key {tag}/ECDSAP256SHA256"
        )

    # Check keys again, make sure the rollover has started.
    keyprops = [
        f"ksk {lifetime['P2Y']} {alg} {size} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk {lifetime['P2M']} {alg} {size} goal:hidden dnskey:omnipresent zrrsig:omnipresent",
        f"zsk {lifetime['P2M']} {alg} {size} goal:omnipresent dnskey:rumoured zrrsig:hidden",
    ]
    expected = isctest.kasp.policy_to_properties(ttl=ttl, keys=keyprops)
    keys = isctest.kasp.keydir_to_keylist(zone, keydir)
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)
    check_all(ns3, zone, policy, ksks, zsks, manual_mode=True)
    isctest.kasp.check_dnssec_verify(ns3, zone)
