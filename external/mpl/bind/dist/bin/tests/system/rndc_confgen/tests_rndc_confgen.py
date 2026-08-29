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

import base64
import os
import re
import subprocess

import pytest

import isctest

INJECTION = 'backdoor"{algorithm hmac-sha256;secret "AAAA";};key "rndc-key'

ESCAPED_KEY = (
    b'key "backdoor\\"{algorithm hmac-sha256;secret \\"AAAA\\";};key \\"rndc-key"'
)

TOO_BIG_LABEL = "key012345678901234567890123456789012345678901234567890123456789X"


def test_rndc_confgen_default():
    cmd = isctest.run.cmd([os.environ["RNDCCONFGEN"]])
    assert b'key "rndc-key" {' in cmd.proc.stdout


def test_rndc_confgen_keyname_with_dots():
    cmd = isctest.run.cmd([os.environ["RNDCCONFGEN"], "-k", "key.example.com"])
    assert b'key "key.example.com" {' in cmd.proc.stdout


def test_rndc_confgen_escapes_injection():
    cmd = isctest.run.cmd([os.environ["RNDCCONFGEN"], "-k", INJECTION])
    assert ESCAPED_KEY in cmd.proc.stdout


def test_rndc_confgen_rejects_to_big_label():
    with pytest.raises(subprocess.CalledProcessError):
        isctest.run.cmd([os.environ["RNDCCONFGEN"], "-k", TOO_BIG_LABEL])


def test_tsig_keygen_default():
    cmd = isctest.run.cmd([os.environ["TSIGKEYGEN"]])
    assert b'key "tsig-key" {' in cmd.proc.stdout


def test_tsig_keygen_escapes_injection():
    cmd = isctest.run.cmd([os.environ["TSIGKEYGEN"], INJECTION])
    assert ESCAPED_KEY in cmd.proc.stdout


def _extract_secret(stdout: bytes) -> bytes:
    match = re.search(rb'secret\s+"([^"]+)"', stdout)
    assert match is not None, f"no secret in output: {stdout!r}"
    return base64.b64decode(match.group(1))


@pytest.mark.parametrize(
    "algorithm,bits",
    [
        ("hmac-sha256", 1),
        ("hmac-sha256", 256),
        ("hmac-sha256", 512),
        ("hmac-sha384", 1),
        ("hmac-sha384", 384),
        ("hmac-sha384", 513),
        ("hmac-sha384", 768),
        ("hmac-sha384", 1024),
        ("hmac-sha512", 1),
        ("hmac-sha512", 512),
        ("hmac-sha512", 513),
        ("hmac-sha512", 1024),
    ],
)
def test_rndc_confgen_hmac_keysize(algorithm, bits):
    cmd = isctest.run.cmd([os.environ["RNDCCONFGEN"], "-A", algorithm, "-b", str(bits)])
    secret = _extract_secret(cmd.proc.stdout)
    assert len(secret) == (bits + 7) // 8
    assert f"algorithm {algorithm};".encode() in cmd.proc.stdout
