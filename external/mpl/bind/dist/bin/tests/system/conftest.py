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

import pytest


@pytest.fixture(scope="session")
def named_port():
    return int(os.environ.get("PORT", default=5300))


@pytest.fixture(scope="session")
def named_tlsport():
    return int(os.environ.get("TLSPORT", default=8853))


@pytest.fixture(scope="session")
def control_port():
    return int(os.environ.get("CONTROLPORT", default=9953))
