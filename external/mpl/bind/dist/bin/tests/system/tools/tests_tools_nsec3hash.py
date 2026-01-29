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
import subprocess

import pytest

import isctest
from isctest.hypothesis.strategies import dns_names

from hypothesis import strategies, given, settings

pytest.importorskip("dns.dnssectypes")
from dns.dnssectypes import NSEC3Hash
import dns.dnssec

NSEC3HASH = os.environ.get("NSEC3HASH")


# test cases from RFC 5155, Appendix A
@pytest.mark.parametrize(
    "domain,nsec3hash",
    [
        ("*.w.example", "R53BQ7CC2UVMUBFU5OCMM6PERS9TK9EN"),
        (
            "2t7b4g4vsa5smi47k61mv5bv1a22bojr.example",
            "KOHAR7MBB8DC2CE8A9QVL8HON4K53UHI",
        ),
        ("a.example", "35MTHGPGCU1QG68FAB165KLNSNK3DPVL"),
        ("ai.example", "GJEQE526PLBF1G8MKLP59ENFD789NJGI"),
        ("example", "0P9MHAVEQVM6T7VBL5LOP2U3T2RP3TOM"),
        ("ns1.example", "2T7B4G4VSA5SMI47K61MV5BV1A22BOJR"),
        ("ns2.example", "Q04JKCEVQVMU85R014C7DKBA38O0JI5R"),
        ("w.example", "K8UDEMVP1J2F7EG6JEBPS17VP3N8I58H"),
        ("x.w.example", "B4UM86EGHHDS6NEA196SMVMLO4ORS995"),
        ("x.y.w.example", "2VPTU5TIMAMQTTGL4LUU9KG21E0AOR3S"),
        ("xx.example", "T644EBQK9BIBCNA874GIVR6JOJ62MLHV"),
        ("y.w.example", "JI6NEOAEPV8B5O6K4EV33ABHA8HT9FGC"),
    ],
)
def test_nsec3_hashes(domain, nsec3hash):
    salt = "aabbccdd"
    algorithm = "1"
    iterations = "12"

    cmd = isctest.run.cmd([NSEC3HASH, salt, algorithm, iterations, domain])
    assert nsec3hash in cmd.out

    flags = "0"
    cmd = isctest.run.cmd([NSEC3HASH, "-r", algorithm, flags, iterations, salt, domain])
    assert nsec3hash in cmd.out


@pytest.mark.parametrize(
    "salt_emptiness_args",
    [
        [""],
        ["-"],
        ["--", ""],
        ["--", "-"],
    ],
)
def test_nsec3_empty_salt(salt_emptiness_args):
    algorithm = "1"
    iterations = "0"
    domain = "com"

    cmd = isctest.run.cmd(
        [NSEC3HASH] + salt_emptiness_args + [algorithm, iterations, domain]
    )
    assert "CK0POJMG874LJREF7EFN8430QVIT8BSM" in cmd.out
    assert "salt=-" in cmd.out


@pytest.mark.parametrize(
    "salt_emptiness_arg",
    [
        "",
        "-",
    ],
)
def test_nsec3_empty_salt_r(salt_emptiness_arg):
    algorithm = "1"
    flags = "1"
    iterations = "0"
    domain = "com"

    cmd = isctest.run.cmd(
        [
            NSEC3HASH,
            "-r",
            algorithm,
            flags,
            iterations,
            salt_emptiness_arg,
            domain,
        ]
    )
    assert " - CK0POJMG874LJREF7EFN8430QVIT8BSM" in cmd.out


@pytest.mark.parametrize(
    "args",
    [
        [""],  # missing arg
        ["two", "names"],  # extra arg
    ],
)
def test_nsec3_missing_args(args):
    with pytest.raises(subprocess.CalledProcessError):
        isctest.run.cmd([NSEC3HASH, "00", "1", "0"] + args)


def test_nsec3_bad_option():
    with pytest.raises(subprocess.CalledProcessError):
        isctest.run.cmd([NSEC3HASH, "-?"])


@given(
    domain=dns_names(),
    it=strategies.integers(min_value=0, max_value=65535),
    salt_bytes=strategies.binary(min_size=0, max_size=255),
)
def test_nsec3hash_acceptable_values(domain, it, salt_bytes) -> None:
    if not salt_bytes:
        salt_text = "-"
    else:
        salt_text = salt_bytes.hex()
    # calculate the hash using dnspython:
    hash1 = dns.dnssec.nsec3_hash(
        domain, salt=salt_bytes, iterations=it, algorithm=NSEC3Hash.SHA1
    )

    # calculate the hash using nsec3hash:
    cmd = isctest.run.cmd([NSEC3HASH, salt_text, "1", str(it), str(domain)])
    hash2 = cmd.out.partition(" ")[0]

    assert hash1 == hash2


@settings(max_examples=5)
@given(
    domain=dns_names(),
    it=strategies.integers(min_value=0, max_value=65535),
    salt_bytes=strategies.binary(min_size=256),
)
def test_nsec3hash_salt_too_long(domain, it, salt_bytes) -> None:
    salt_text = salt_bytes.hex()
    with pytest.raises(subprocess.CalledProcessError):
        isctest.run.cmd([NSEC3HASH, salt_text, "1", str(it), str(domain)])


@settings(max_examples=5)
@given(
    domain=dns_names(),
    it=strategies.integers(min_value=65536),
    salt_bytes=strategies.binary(min_size=0, max_size=255),
)
def test_nsec3hash_too_many_iterations(domain, it, salt_bytes) -> None:
    salt_text = salt_bytes.hex()
    with pytest.raises(subprocess.CalledProcessError):
        isctest.run.cmd([NSEC3HASH, salt_text, "1", str(it), str(domain)])
