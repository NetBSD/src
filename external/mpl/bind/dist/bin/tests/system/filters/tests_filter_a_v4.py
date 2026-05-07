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

import pytest

from filters.common import (
    ARTIFACTS,
    check_filter,
    check_filter_other_family,
    prime_cache,
)

import isctest.mark

pytestmark = pytest.mark.extra_artifacts(ARTIFACTS)


def bootstrap():
    return {
        "family": "v4",
        "filtertype": "a",
    }


@pytest.fixture(scope="module", autouse=True)
def after_servers_start():
    prime_cache("10.53.0.2")
    prime_cache("10.53.0.3")


@pytest.mark.parametrize(
    "addr, altaddr, break_dnssec, recursive",
    [
        pytest.param("10.53.0.1", "10.53.0.2", False, False, id="auth"),
        pytest.param("10.53.0.4", "10.53.0.2", True, False, id="auth-break-dnssec"),
        pytest.param("10.53.0.2", "10.53.0.1", False, True, id="recurs"),
        pytest.param("10.53.0.3", "10.53.0.1", True, True, id="recurs-break-dnssec"),
    ],
)
def test_filter_a_on_v4(addr, altaddr, break_dnssec, recursive):
    check_filter(addr, altaddr, "a", break_dnssec, recursive)


@isctest.mark.with_ipv6
@pytest.mark.parametrize(
    "addr",
    [
        pytest.param("fd92:7065:b8e:ffff::1", id="auth"),
        pytest.param("fd92:7065:b8e:ffff::4", id="auth-break-dnssec"),
        pytest.param("fd92:7065:b8e:ffff::2", id="recurs"),
        pytest.param("fd92:7065:b8e:ffff::3", id="recurs-break-dnssec"),
    ],
)
def test_filter_a_on_v4_via_v6(addr):
    check_filter_other_family(addr, "a")
