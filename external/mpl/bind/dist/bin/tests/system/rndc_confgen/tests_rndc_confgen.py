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

import pytest

import isctest


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
