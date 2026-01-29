#!/usr/bin/python3

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
from pathlib import Path
import platform
import socket
import shutil
import subprocess

import pytest

long_test = pytest.mark.skipif(
    not os.environ.get("CI_ENABLE_LONG_TESTS"), reason="CI_ENABLE_LONG_TESTS not set"
)

live_internet_test = pytest.mark.skipif(
    not os.environ.get("CI_ENABLE_LIVE_INTERNET_TESTS"),
    reason="CI_ENABLE_LIVE_INTERNET_TESTS not set",
)


DNSRPS_BIN = Path(os.environ["TOP_BUILDDIR"]) / "bin/tests/system/rpz/dnsrps"


def is_dnsrps_available():
    if os.getenv("FEATURE_DNSRPS") != "1":
        return False
    try:
        subprocess.run([DNSRPS_BIN, "-a"], check=True)
    except subprocess.CalledProcessError:
        return False
    return True


def is_host_freebsd_13(*_):
    return platform.system() == "FreeBSD" and platform.release().startswith("13")


def with_algorithm(name: str):
    key = f"{name}_SUPPORTED"
    assert key in os.environ, f"{key} env variable undefined"
    return pytest.mark.skipif(os.getenv(key) != "1", reason=f"{name} is not supported")


with_dnstap = pytest.mark.skipif(
    os.getenv("FEATURE_DNSTAP") != "1", reason="DNSTAP support disabled in the build"
)


without_fips = pytest.mark.skipif(
    os.getenv("FEATURE_FIPS_MODE") == "1", reason="FIPS support enabled in the build"
)

with_libxml2 = pytest.mark.skipif(
    os.getenv("FEATURE_LIBXML2") != "1", reason="libxml2 support disabled in the build"
)

with_lmdb = pytest.mark.skipif(
    os.getenv("FEATURE_LMDB") != "1", reason="LMDB support disabled in the build"
)

with_json_c = pytest.mark.skipif(
    os.getenv("FEATURE_JSON_C") != "1", reason="json-c support disabled in the build"
)

dnsrps_enabled = pytest.mark.skipif(
    not is_dnsrps_available(), reason="dnsrps disabled in the build"
)


softhsm2_environment = pytest.mark.skipif(
    not (
        os.getenv("SOFTHSM2_CONF")
        and os.getenv("SOFTHSM2_MODULE")
        and shutil.which("pkcs11-tool")
        and shutil.which("softhsm2-util")
    ),
    reason="SOFTHSM2_CONF and SOFTHSM2_MODULE environmental variables must be set and pkcs11-tool and softhsm2-util tools present",
)


def have_ipv6():
    sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    try:
        sock.bind(("fd92:7065:b8e:ffff::1", 0))
    except OSError:
        return False
    return True


with_ipv6 = pytest.mark.skipif(not have_ipv6(), reason="IPv6 not available")
