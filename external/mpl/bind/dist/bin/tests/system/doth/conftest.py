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

import shutil
import subprocess

import pytest


@pytest.fixture
def gnutls_cli_executable():
    # Ensure gnutls-cli is available.
    executable = shutil.which("gnutls-cli")
    if not executable:
        pytest.skip("gnutls-cli not found in PATH")

    # Ensure gnutls-cli supports the --logfile command-line option.
    output = subprocess.run(
        [executable, "--logfile=/dev/null"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    ).stdout
    if b"illegal option" in output:
        pytest.skip("gnutls-cli does not support the --logfile option")

    return executable


@pytest.fixture
def sslyze_executable():
    # Check whether sslyze is available.
    executable = shutil.which("sslyze")
    if not executable:
        pytest.skip("sslyze not found in PATH")

    return executable
