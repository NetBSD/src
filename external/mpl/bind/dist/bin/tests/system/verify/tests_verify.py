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

from re import compile as Re

import os
import re

import pytest

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
    ]
)

VERIFY = os.environ.get("VERIFY")


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
