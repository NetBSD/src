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

from pathlib import Path
from re import compile as Re

import os
import re
import time

from cryptography.hazmat.primitives.asymmetric import rsa
from dns.dnssectypes import NSEC3Hash
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.name
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.zone
import pytest

from isctest.template import NO_NS, zones
from isctest.zone import Zone

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "verify.out.*",
        "zones/K*",
        "zones/dsset-*",
        "zones/*.bad",
        "zones/*.good",
        "zones/*.out*",
        "zones/*.tmp",
        "zones/updated*",
        "zones/bad-nsec3param-hash*",
        "zones/nsec-bad-nsec3param-hash*",
        "zones/good-bad-nsec3param-hash*",
        "zones/nsec+non-zero-nsec3param-flags*",
        "zones/no-nsec+non-zero-nsec3param-flags*",
        "zones/nseconly-nsec3param*",
    ]
)

VERIFY = os.environ.get("VERIFY")


def bootstrap():

    def generate_keys():
        algorithm = dns.dnssec.Algorithm.RSASHA256
        ksk_private_key = rsa.generate_private_key(public_exponent=65537, key_size=1024)
        try:
            ksk_dnskey = dns.dnssec.make_dnskey(
                public_key=ksk_private_key.public_key(),
                algorithm=algorithm,
                flags=Flag.ZONE | Flag.SEP,
            )
        except ImportError as exc:
            # if the cryptography package is too old, the make_dnskey() function
            # will raise ImportError at runtime
            pytest.skip(f"{exc}")
        return ksk_private_key, ksk_dnskey

    def gen_and_sign(name, nsec3=False):
        zone = Zone(name, NO_NS, signed=True)
        ksk_private_key, ksk_dnskey = generate_keys()
        keys = [(ksk_private_key, ksk_dnskey)]
        zone.render()

        # read the rendered zone
        unsigned_path = str(Path(zone.ns.name) / zone.filepath_unsigned)
        signed_path = str(Path(zone.ns.name) / zone.filepath_signed)
        zoneobj = dns.zone.from_file(unsigned_path, origin=f"{name}.")
        lifetime = 30 * 86400

        # sign the zone
        with zoneobj.writer() as txn:
            dns.dnssec.sign_zone(
                zone=zoneobj,
                txn=txn,
                keys=keys,
                lifetime=lifetime,
                add_dnskey=True,
                deterministic=False,  # for OpenSSL<3.2.0 compat
            )

        # This generates an NSEC3 record for the apex, so that we can
        # verify a zone with both a valid and invalid NSEC3PARAM record.
        if nsec3:
            origin = dns.name.from_text(f"{name}.")
            hashname = dns.dnssec.nsec3_hash(
                origin, salt=b"", iterations=0, algorithm=NSEC3Hash.SHA1
            )
            nsec3rdata = dns.rdata.from_text(
                rdclass=dns.rdataclass.IN,
                rdtype=dns.rdatatype.NSEC3,
                tok=f"1 0 0 - {hashname} SOA NS A NSEC3PARAM DNSKEY RRSIG",
            )
            nsec3_owner = dns.name.from_text(f"{hashname}.{name}.")
            node = zoneobj.find_node(nsec3_owner, create=True)
            nsec3_rrset = node.find_rdataset(
                rdclass=dns.rdataclass.IN, rdtype=dns.rdatatype.NSEC3, create=True
            )
            nsec3_rrset.ttl = 300
            nsec3_rrset.add(nsec3rdata)

            inception = int(time.time()) - 30
            nsec3_sigdata = dns.dnssec.sign(
                rrset=(nsec3_owner, nsec3_rrset),
                private_key=ksk_private_key,
                signer=origin,
                dnskey=ksk_dnskey,
                inception=inception,
                lifetime=lifetime,
            )
            nsec3_rrsig = node.find_rdataset(
                rdclass=dns.rdataclass.IN,
                rdtype=dns.rdatatype.RRSIG,
                covers=dns.rdatatype.NSEC3,
                create=True,
            )
            nsec3_rrsig.ttl = 300
            nsec3_rrsig.add(nsec3_sigdata)

        zoneobj.to_file(signed_path)

        return zone

    return {
        "zones": zones(
            [
                gen_and_sign("bad-nsec3param-hash"),
                gen_and_sign("nsec-bad-nsec3param-hash"),
                gen_and_sign("good-bad-nsec3param-hash", True),
                gen_and_sign("nsec+non-zero-nsec3param-flags"),
                gen_and_sign("no-nsec+non-zero-nsec3param-flags"),
                gen_and_sign("nseconly-nsec3param", True),
            ]
        ),
    }


@pytest.mark.parametrize(
    "zone",
    [
        "ksk-only.nsec3",
        "ksk-only.nsec",
        "ksk+zsk.nsec3.apex-dname",
        "ksk+zsk.nsec3",
        "ksk+zsk.nsec.apex-dname",
        "ksk+zsk.nsec",
        "ksk+zsk.optout",
        "zsk-only.nsec3",
        "zsk-only.nsec",
    ],
)
def test_verify_good_zone_files(zone):
    isctest.run.cmd([VERIFY, "-z", "-o", zone, f"zones/{zone}.good"])


def test_verify_good_zone_nsec_next_name_case_mismatch():
    isctest.run.cmd(
        [
            VERIFY,
            "-o",
            "nsec-next-name-case-mismatch",
            "zones/nsec-next-name-case-mismatch.good",
        ],
    )


def verify_bad_zone(zone):
    only_opt = ["-z"] if re.search(r"^[zk]sk-only", zone) else []
    cmd = isctest.run.cmd(
        [VERIFY, *only_opt, "-o", zone, f"zones/{zone}.bad"],
        raise_on_exception=False,
    )
    assert cmd.rc != 0
    return cmd


@pytest.mark.parametrize(
    "zone",
    [
        "ksk-only.dnskeyonly",
        "ksk+zsk.dnskeyonly",
        "zsk-only.dnskeyonly",
    ],
)
def test_verify_bad_zone_files_dnskeyonly(zone):
    cmd = verify_bad_zone(zone)
    assert "DNSKEY is not signed" in cmd.err


@pytest.mark.parametrize(
    "zone",
    [
        "ksk-only.nsec3.expired",
        "ksk-only.nsec.expired",
        "ksk+zsk.nsec3.expired",
        "ksk+zsk.nsec.expired",
        "ksk+zsk.nsec.ksk-expired",
        "zsk-only.nsec3.expired",
        "zsk-only.nsec.expired",
        "ksk+zsk.nsec3.ksk-expired",
    ],
)
def test_verify_bad_zone_files_expired(zone):
    cmd = verify_bad_zone(zone)
    assert Re("signature has expired|No self-signed DNSKEY found") in cmd.err


@pytest.mark.parametrize(
    "zone",
    [
        "ksk+zsk.nsec.out-of-zone-nsec",
        "ksk+zsk.nsec.below-bottom-of-zone-nsec",
        "ksk+zsk.nsec.below-dname-nsec",
    ],
)
def test_verify_bad_zone_files_unexpected_nsec_rrset(zone):
    cmd = verify_bad_zone(zone)
    assert "unexpected NSEC RRset at" in cmd.err


def test_verify_bad_zone_files_bad_nsec_record():
    cmd = verify_bad_zone("ksk+zsk.nsec.broken-chain")
    assert Re("Bad NSEC record for.*, next name mismatch") in cmd.err


def test_verify_bad_zone_files_bad_bitmap():
    cmd = verify_bad_zone("ksk+zsk.nsec.bad-bitmap")
    assert "bit map mismatch" in cmd.err


def test_verify_bad_zone_files_missing_nsec3_record():
    cmd = verify_bad_zone("ksk+zsk.nsec3.missing-empty")
    assert "Missing NSEC3 record for" in cmd.err


def test_verify_bad_zone_files_no_dnssec_keys():
    cmd = verify_bad_zone("unsigned")
    assert "Zone contains no DNSSEC keys" in cmd.err


def test_verify_bad_zone_files_unequal_nsec3_chains():
    cmd = verify_bad_zone("ksk+zsk.nsec3.extra-nsec3")
    assert "Expected and found NSEC3 chains not equal" in cmd.err


# checking error message when -o is not used
# and a SOA record not at top of zone is found
def test_verify_soa_not_at_top_error():
    # when -o is not used, origin is set to zone file name,
    # which should cause an error in this case
    cmd = isctest.run.cmd([VERIFY, "zones/ksk+zsk.nsec.good"], raise_on_exception=False)
    assert "not at top of zone" in cmd.err
    assert "use -o to specify a different zone origin" in cmd.err


# checking error message when an invalid -o is specified
# and a SOA record not at top of zone is found
def test_verify_invalid_o_option_soa_not_at_top_error():
    cmd = isctest.run.cmd(
        [VERIFY, "-o", "invalid.origin", "zones/ksk+zsk.nsec.good"],
        raise_on_exception=False,
    )
    assert "not at top of zone" in cmd.err
    assert "use -o to specify a different zone origin" not in cmd.err


# checking dnssec-verify -J reads journal file
def test_verify_j_reads_journal_file():
    cmd = isctest.run.cmd(
        [
            VERIFY,
            "-o",
            "updated",
            "-J",
            "zones/updated.other.jnl",
            "zones/updated.other",
        ]
    )
    assert "Loading zone 'updated' from file 'zones/updated.other'" in cmd.out


# checking that unknown hash is detected
def test_verify_rejects_bad_nsec3param_hash():
    cmd = isctest.run.cmd(
        [
            VERIFY,
            "-z",
            "-o",
            "bad-nsec3param-hash",
            "zones/bad-nsec3param-hash.db.signed",
        ],
        raise_on_exception=False,
    )
    assert cmd.rc != 0
    assert "No usable NSEC/NSEC3 chain for testing" in cmd.err


def test_verify_ignores_bad_nsec3param_hash_with_nsec():
    isctest.run.cmd(
        [
            VERIFY,
            "-z",
            "-o",
            "nsec-bad-nsec3param-hash",
            "zones/nsec-bad-nsec3param-hash.db.signed",
        ]
    )


def test_verify_ignores_bad_nsec3param_hash_with_good_nsec3param():
    isctest.run.cmd(
        [
            VERIFY,
            "-z",
            "-o",
            "good-bad-nsec3param-hash",
            "zones/good-bad-nsec3param-hash.db.signed",
        ]
    )


def test_verify_rejects_no_nsec_and_all_nsec3param_with_non_zero_flags():
    cmd = isctest.run.cmd(
        [
            VERIFY,
            "-z",
            "-o",
            "no-nsec+non-zero-nsec3param-flags",
            "zones/no-nsec+non-zero-nsec3param-flags.db.signed",
        ],
        raise_on_exception=False,
    )
    assert cmd.rc != 0
    assert "No usable NSEC/NSEC3 chain for testing" in cmd.err


def test_verify_ignores_nsec3param_non_zero_flags():
    isctest.run.cmd(
        [
            VERIFY,
            "-z",
            "-o",
            "nsec+non-zero-nsec3param-flags",
            "zones/nsec+non-zero-nsec3param-flags.db.signed",
        ],
    )


def test_verify_rejects_nsec3param_with_nsec_only_key():
    cmd = isctest.run.cmd(
        [
            VERIFY,
            "-z",
            "-o",
            "nseconly-nsec3param",
            "zones/nseconly-nsec3param.db.signed",
        ],
        raise_on_exception=False,
    )
    assert cmd.rc != 0
    isctest.log.debug(cmd.err)
    assert "cannot use NSEC3 with key algorithm" in cmd.out
