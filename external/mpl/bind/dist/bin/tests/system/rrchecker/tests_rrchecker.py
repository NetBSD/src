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

import isctest
import pytest


@pytest.mark.parametrize(
    "option,expected_result",
    [
        ("-C", ["HS", "CH", "IN"]),
        (
            "-T",
            [
                "A",
                "A6",
                "AAAA",
                "AFSDB",
                "AMTRELAY",
                "APL",
                "ATMA",
                "AVC",
                "CAA",
                "CDNSKEY",
                "CDS",
                "CERT",
                "CNAME",
                "CSYNC",
                "DHCID",
                "DLV",
                "DNAME",
                "DNSKEY",
                "DOA",
                "DS",
                "EID",
                "EUI48",
                "EUI64",
                "GID",
                "GPOS",
                "HINFO",
                "HIP",
                "HTTPS",
                "IPSECKEY",
                "ISDN",
                "KEY",
                "KX",
                "L32",
                "L64",
                "LOC",
                "LP",
                "MB",
                "MD",
                "MF",
                "MG",
                "MINFO",
                "MR",
                "MX",
                "NAPTR",
                "NID",
                "NIMLOC",
                "NINFO",
                "NS",
                "NSAP",
                "NSAP-PTR",
                "NSEC",
                "NSEC3",
                "NSEC3PARAM",
                "NULL",
                "NXT",
                "OPENPGPKEY",
                "PTR",
                "PX",
                "RESINFO",
                "RKEY",
                "RP",
                "RRSIG",
                "RT",
                "SIG",
                "SINK",
                "SMIMEA",
                "SOA",
                "SPF",
                "SRV",
                "SSHFP",
                "SVCB",
                "TA",
                "TALINK",
                "TLSA",
                "TXT",
                "UID",
                "UINFO",
                "UNSPEC",
                "URI",
                "WKS",
                "X25",
                "ZONEMD",
            ],
        ),
        ("-P", []),
    ],
)
def test_rrchecker_list_standard_names(option, expected_result):
    stdout = isctest.run.cmd([os.environ["RRCHECKER"], option]).stdout.decode("utf-8")
    values = [line for line in stdout.split("\n") if line.strip()]

    assert sorted(values) == sorted(expected_result)


def run_rrchecker(option, rr_class, rr_type, rr_rest):
    rrchecker_output = (
        isctest.run.cmd(
            [os.environ["RRCHECKER"], option],
            input_text=f"{rr_class} {rr_type} {rr_rest}".encode("utf-8"),
        )
        .stdout.decode("utf-8")
        .strip()
    )
    return rrchecker_output.split()


@pytest.mark.parametrize("option", ["-p", "-u"])
def test_rrchecker_conversions(option):
    tempzone_file = "tempzone"
    with open(tempzone_file, "w", encoding="utf-8") as file:
        isctest.run.cmd(
            [
                os.environ["SHELL"],
                os.environ["TOP_SRCDIR"] + "/bin/tests/system/genzone.sh",
                "0",
            ],
            stdout=file,
        )
    checkzone_output = isctest.run.cmd(
        [
            os.environ["CHECKZONE"],
            "-D",
            "-q",
            ".",
            tempzone_file,
        ],
    ).stdout.decode("utf-8")
    checkzone_output = [
        line for line in checkzone_output.splitlines() if not line.startswith(";")
    ]

    for rr in checkzone_output:
        rr_parts_orig = rr.split()
        assert len(rr_parts_orig) >= 4, f"invalid rr: {rr}"
        rr_class_orig, rr_type_orig, rr_rest_orig = (
            rr_parts_orig[2],
            rr_parts_orig[3],
            " ".join(rr_parts_orig[4:]),
        )
        rr_class, rr_type, rr_rest = rr_class_orig, rr_type_orig, rr_rest_orig
        if option == "-u":
            rr_class, rr_type, *rr_rest = run_rrchecker(
                "-u", rr_class_orig, rr_type_orig, rr_rest_orig
            )
            rr_rest = " ".join(rr_rest)

        rr_class, rr_type, *rr_rest = run_rrchecker("-p", rr_class, rr_type, rr_rest)

        assert rr_class_orig == rr_class
        assert rr_type_orig == rr_type
        assert rr_rest_orig == " ".join(rr_rest)
