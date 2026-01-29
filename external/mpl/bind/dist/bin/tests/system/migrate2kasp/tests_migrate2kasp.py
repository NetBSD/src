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

from datetime import timedelta

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import isctest
import isctest.mark

pytestmark = pytest.mark.extra_artifacts(
    [
        "*.axfr",
        "*.created",
        "created.key-*",
        "dig.out*",
        "ns*/*.mkeys*",
        "ns*/dsset-*",
        "ns*/K*.key",
        "ns*/K*.private",
        "ns*/K*.state",
        "ns*/kasp.conf",
        "ns*/keygen.out*",
        "ns*/managed-keys.bind*",
        "ns*/named.conf",
        "ns*/named.memstats",
        "ns*/named.run",
        "ns*/signer.out*",
        "ns*/zones",
        "ns*/*.db",
        "ns*/*.db.infile",
        "ns*/*.db.jbk",
        "ns*/*.db.jnl",
        "ns*/*.db.signed*",
        "python.out.*",
        "retired.*",
        "rndc.dnssec.*",
        "unused.key*",
        "verify.out.*",
    ]
)

default_config = {
    "dnskey-ttl": timedelta(hours=1),
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

standard_config = {
    "dnskey-ttl": timedelta(seconds=7200),
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

timing_config = {
    "dnskey-ttl": timedelta(seconds=300),
    "ds-ttl": timedelta(seconds=7200),
    "key-directory": "{keydir}",
    "max-zone-ttl": timedelta(hours=11),
    "parent-propagation-delay": timedelta(hours=1),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=5),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(seconds=3600),
}

migrate_config = {
    "dnskey-ttl": timedelta(seconds=300),
    "ds-ttl": timedelta(seconds=7200),
    "key-directory": "{keydir}",
    "max-zone-ttl": timedelta(hours=11),
    "parent-propagation-delay": timedelta(hours=1),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=5),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(seconds=3600),
}

view_config = {
    "dnskey-ttl": timedelta(seconds=300),
    "ds-ttl": timedelta(seconds=86400),
    "key-directory": "{keydir}",
    "max-zone-ttl": timedelta(days=1),
    "parent-propagation-delay": timedelta(hours=3),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=5),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(seconds=300),
}

lifetime = {
    "P60D": int(timedelta(days=60).total_seconds()),
    "P3M": int(timedelta(days=31 * 3).total_seconds()),
    "P1Y": int(timedelta(days=365).total_seconds()),
}


@pytest.mark.parametrize(
    "params",
    [
        # Testing good migration (KSK/ZSK).
        pytest.param(
            {
                "zone": "migrate.kasp",
                "policy": "migrate",
                "server": "ns3",
                "config": standard_config,
                "offset": 0,
                "key-properties": [
                    f"ksk 0 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:rumoured",
                    f"zsk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
                ],
            },
            id="migrate.kasp",
        ),
        # Testing a good migration (CSK).
        pytest.param(
            {
                "zone": "csk.kasp",
                "policy": "default",
                "server": "ns3",
                "config": default_config,
                "offset": 0,
                "key-properties": [
                    f"csk 0 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:rumoured",
                ],
            },
            id="csk.kasp",
        ),
        # Testing a good migration (CSK, no SEP).
        pytest.param(
            {
                "zone": "csk-nosep.kasp",
                "policy": "default",
                "server": "ns3",
                "config": default_config,
                "offset": 0,
                "key-properties": [
                    f"csk 0 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:rumoured ds:rumoured",
                ],
            },
            id="csk-nosep.kasp",
        ),
        # Testing key states derived from timing metadata: rumoured.
        pytest.param(
            {
                "zone": "rumoured.kasp",
                "policy": "timing-metadata",
                "server": "ns3",
                "config": timing_config,
                "offset": -timedelta(seconds=300),
                "key-properties": [
                    f"ksk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:rumoured",
                    f"zsk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
                ],
            },
            id="rumoured.kasp",
        ),
        # Testing key states derived from timing metadata: omnipresent.
        pytest.param(
            {
                "zone": "omnipresent.kasp",
                "policy": "timing-metadata",
                "server": "ns3",
                "config": timing_config,
                "offset": -timedelta(seconds=3900),
                "key-properties": [
                    f"ksk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                    f"zsk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
                ],
            },
            id="omnipresent.kasp",
        ),
        # Testing key states derived from timing metadata: no SyncPublish.
        pytest.param(
            {
                "zone": "no-syncpublish.kasp",
                "policy": "timing-metadata",
                "server": "ns3",
                "config": timing_config,
                "offset": -timedelta(hours=12),
                "key-properties": [
                    f"ksk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:rumoured",
                    f"zsk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
                ],
            },
            id="no-syncpublish.kasp",
        ),
        # Test migration to dnssec-policy, existing keys do not match key algorithm.
        pytest.param(
            {
                "zone": "migrate-nomatch-algnum.kasp",
                "policy": "migrate-nomatch-algnum",
                "server": "ns3",
                "config": migrate_config,
                "offset": -timedelta(seconds=3900),
                "key-properties": [
                    "ksk - 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                    "zsk - 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent",
                    f"ksk 0 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
                    f"zsk {lifetime['P60D']} {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured zrrsig:rumoured",
                ],
            },
            id="migrate-nomatch-algnum.kasp",
        ),
        # Test migration to dnssec-policy, existing keys do not match key length.
        pytest.param(
            {
                "zone": "migrate-nomatch-alglen.kasp",
                "policy": "migrate-nomatch-alglen",
                "server": "ns3",
                "config": migrate_config,
                "offset": -timedelta(seconds=3900),
                "key-properties": [
                    "ksk - 8 2048 goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                    "zsk - 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent",
                    "ksk 0 8 3072 goal:omnipresent dnskey:rumoured krrsig:rumoured ds:hidden",
                    # This key is considered to be prepublished, so it is not yet signing.
                    f"zsk {lifetime['P60D']} 8 3072 goal:omnipresent dnskey:rumoured zrrsig:hidden",
                ],
            },
            id="migrate-nomatch-alglen.kasp",
        ),
        # Test migration to dnssec-policy, existing keys do not match role (KSK/ZSK -> CSK).
        pytest.param(
            {
                "zone": "migrate-nomatch-kzc.kasp",
                "policy": "migrate-nomatch-kzc",
                "server": "ns3",
                "config": migrate_config,
                "offset": -timedelta(seconds=3900),
                "key-properties": [
                    f"ksk - {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:hidden dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                    f"zsk - {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:hidden dnskey:omnipresent zrrsig:omnipresent",
                    # This key is considered to be prepublished, so it is not yet signing, nor is the DS introduced.
                    f"csk 0 {os.environ['DEFAULT_ALGORITHM_NUMBER']} {os.environ['DEFAULT_BITS']} goal:omnipresent dnskey:rumoured krrsig:rumoured zrrsig:hidden ds:hidden",
                ],
            },
            id="migrate-nomatch-kzc.kasp",
        ),
        # Test good migration with views.
        pytest.param(
            {
                "zone": "view-rsasha256.kasp",
                "policy": "rsasha256",
                "server": "ns4",
                "config": view_config,
                "offset": -timedelta(days=31 * 3),
                "key-properties": [
                    f"zsk {lifetime['P3M']} 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent",
                    f"zsk {lifetime['P3M']} 8 2048 goal:omnipresent dnskey:rumoured zrrsig:hidden",
                    f"ksk {lifetime['P1Y']} 8 2048 goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                ],
                "view": "ext",
                "tsig": "external:YPfMoAk6h+3iN8MDRQC004iSNHY=",
            },
            id="view-rsasha256.kasp (external)",
        ),
        pytest.param(
            {
                "zone": "view-rsasha256.kasp",
                "policy": "rsasha256",
                "server": "ns4",
                "config": view_config,
                "offset": -timedelta(days=31 * 3),
                "key-properties": [
                    f"zsk {lifetime['P3M']} 8 2048 goal:hidden dnskey:omnipresent zrrsig:omnipresent",
                    f"zsk {lifetime['P3M']} 8 2048 goal:omnipresent dnskey:rumoured zrrsig:hidden",
                    f"ksk {lifetime['P1Y']} 8 2048 goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
                ],
                "view": "int",
                "tsig": "internal:4xILSZQnuO1UKubXHkYUsvBRPu8=",
            },
            id="view-rsasha256.kasp (internal)",
        ),
    ],
)
def test_migrate2kasp_case(servers, params):
    # Get test parameters.
    zone = params["zone"]
    policy = params["policy"]
    server = servers[params["server"]]
    keydir = server.identifier
    view = params.get("view", None)
    tsig = None
    if "tsig" in params:
        secret = params["tsig"]
        tsig = f"{os.environ['DEFAULT_HMAC']}:{secret}"

    isctest.kasp.wait_keymgr_done(server, zone)

    params["config"]["key-directory"] = params["config"]["key-directory"].replace(
        "{keydir}", keydir
    )
    ttl = int(params["config"]["dnskey-ttl"].total_seconds())

    # Test case.
    isctest.log.info(f"check test case zone {zone} policy {policy}")

    # First make sure the zone is signed.
    isctest.kasp.check_dnssec_verify(server, zone, tsig=tsig)

    # Key properties.
    expected = isctest.kasp.policy_to_properties(ttl=ttl, keys=params["key-properties"])

    # Special case: CSK without SEP bit set.
    if zone == "csk-nosep.kasp":
        expected[0].flags = 256

    # Key files.
    keys = isctest.kasp.keydir_to_keylist(zone, params["config"]["key-directory"])
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]
    isctest.kasp.check_keys(zone, keys, expected)

    offset = params["offset"] if "offset" in params else None

    for expect in expected:
        expect.set_expected_keytimes(params["config"], offset=offset, migrate=True)

    isctest.kasp.check_dnssecstatus(server, zone, ksks + zsks, policy=policy, view=view)
    isctest.kasp.check_apex(server, zone, ksks, zsks, tsig=tsig)
    isctest.kasp.check_subdomain(server, zone, ksks, zsks, tsig=tsig)

    if "additional-tests" in params:
        for additional_test in params["additional-tests"]:
            callback = additional_test["callback"]
            arguments = additional_test["arguments"]
            callback(*arguments, params=params, ksks=ksks, zsks=zsks)
