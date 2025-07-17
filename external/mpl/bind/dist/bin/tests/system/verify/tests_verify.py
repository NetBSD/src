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


def get_bad_zone_output(zone):
    only_opt = ["-z"] if re.match(r"[zk]sk-only", zone) else []
    output = isctest.run.cmd(
        [VERIFY, *only_opt, "-o", zone, f"zones/{zone}.bad"],
        raise_on_exception=False,
    )
    stream = (output.stdout + output.stderr).decode("utf-8").replace("\n", "")
    return stream


@pytest.mark.parametrize(
    "zone",
    [
        "ksk-only.dnskeyonly",
        "ksk+zsk.dnskeyonly",
        "zsk-only.dnskeyonly",
    ],
)
def test_verify_bad_zone_files_dnskeyonly(zone):
    assert re.match(r".*DNSKEY is not signed.*", get_bad_zone_output(zone))


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
    assert re.match(
        r".*signature has expired.*|.*No self-signed .*DNSKEY found.*",
        get_bad_zone_output(zone),
    )


@pytest.mark.parametrize(
    "zone",
    [
        "ksk+zsk.nsec.out-of-zone-nsec",
        "ksk+zsk.nsec.below-bottom-of-zone-nsec",
        "ksk+zsk.nsec.below-dname-nsec",
    ],
)
def test_verify_bad_zone_files_unexpected_nsec_rrset(zone):
    assert re.match(r".*unexpected NSEC RRset at.*", get_bad_zone_output(zone))


def test_verify_bad_zone_files_bad_nsec_record():
    assert re.match(
        r".*Bad NSEC record for.*, next name mismatch.*",
        get_bad_zone_output("ksk+zsk.nsec.broken-chain"),
    )


def test_verify_bad_zone_files_bad_bitmap():
    assert re.match(
        r".*bit map mismatch.*", get_bad_zone_output("ksk+zsk.nsec.bad-bitmap")
    )


def test_verify_bad_zone_files_missing_nsec3_record():
    assert re.match(
        r".*Missing NSEC3 record for.*",
        get_bad_zone_output("ksk+zsk.nsec3.missing-empty"),
    )


def test_verify_bad_zone_files_no_dnssec_keys():
    assert re.match(
        r".*Zone contains no DNSSEC keys.*", get_bad_zone_output("unsigned")
    )


def test_verify_bad_zone_files_unequal_nsec3_chains():
    assert re.match(
        r".*Expected and found NSEC3 chains not equal.*",
        get_bad_zone_output("ksk+zsk.nsec3.extra-nsec3"),
    )


# checking error message when -o is not used
# and a SOA record not at top of zone is found
def test_verify_soa_not_at_top_error():
    # when -o is not used, origin is set to zone file name,
    # which should cause an error in this case
    output = isctest.run.cmd(
        [VERIFY, "zones/ksk+zsk.nsec.good"], raise_on_exception=False
    ).stderr.decode("utf-8")
    assert "not at top of zone" in output
    assert "use -o to specify a different zone origin" in output


# checking error message when an invalid -o is specified
# and a SOA record not at top of zone is found
def test_verify_invalid_o_option_soa_not_at_top_error():
    output = isctest.run.cmd(
        [VERIFY, "-o", "invalid.origin", "zones/ksk+zsk.nsec.good"],
        raise_on_exception=False,
    ).stderr.decode("utf-8")
    assert "not at top of zone" in output
    assert "use -o to specify a different zone origin" not in output


# checking dnssec-verify -J reads journal file
def test_verify_j_reads_journal_file():
    output = isctest.run.cmd(
        [
            VERIFY,
            "-o",
            "updated",
            "-J",
            "zones/updated.other.jnl",
            "zones/updated.other",
        ]
    ).stdout.decode("utf-8")
    assert "Loading zone 'updated' from file 'zones/updated.other'" in output
