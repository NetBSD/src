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


def feature_test(feature):
    feature_test_bin = os.environ["FEATURETEST"]
    try:
        subprocess.run([feature_test_bin, feature], check=True)
    except subprocess.CalledProcessError as exc:
        if exc.returncode != 1:
            raise
        return False
    return True


DNSRPS_BIN = Path(os.environ["TOP_BUILDDIR"]) / "bin/tests/system/rpz/dnsrps"


def is_dnsrps_available():
    if not feature_test("--enable-dnsrps"):
        return False
    try:
        subprocess.run([DNSRPS_BIN, "-a"], check=True)
    except subprocess.CalledProcessError:
        return False
    return True


def with_tsan(*args):  # pylint: disable=unused-argument
    return feature_test("--tsan")


def with_algorithm(name: str):
    key = f"{name}_SUPPORTED"
    assert key in os.environ, f"{key} env variable undefined"
    return pytest.mark.skipif(os.getenv(key) != "1", reason=f"{name} is not supported")


with_dnstap = pytest.mark.skipif(
    not feature_test("--enable-dnstap"), reason="DNSTAP support disabled in the build"
)


without_fips = pytest.mark.skipif(
    feature_test("--have-fips-mode"), reason="FIPS support enabled in the build"
)

with_libxml2 = pytest.mark.skipif(
    not feature_test("--have-libxml2"), reason="libxml2 support disabled in the build"
)

with_lmdb = pytest.mark.skipif(
    not feature_test("--with-lmdb"), reason="LMDB support disabled in the build"
)

with_json_c = pytest.mark.skipif(
    not feature_test("--have-json-c"), reason="json-c support disabled in the build"
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

try:
    import flaky as flaky_pkg  # type: ignore
except ModuleNotFoundError:
    # In case the flaky package is not installed, run the tests as usual
    # without any attempts to re-run them.
    # pylint: disable=unused-argument
    def flaky(*args, **kwargs):
        """Mock decorator that doesn't do anything special, just returns the function."""

        def wrapper(wrapped_obj):
            return wrapped_obj

        return wrapper

else:
    flaky = flaky_pkg.flaky
