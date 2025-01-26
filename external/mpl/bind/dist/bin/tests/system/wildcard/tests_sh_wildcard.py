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

pytestmark = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "ns1/K*",
        "ns1/dsset-*",
        "ns1/*.signed",
        "ns1/allwild.db",
        "ns1/example.db",
        "ns1/nestedwild.db",
        "ns1/nsec.db",
        "ns1/nsec3.db",
        "ns1/private.nsec.conf",
        "ns1/private.nsec.db",
        "ns1/private.nsec3.conf",
        "ns1/private.nsec3.db",
        "ns1/root.db",
        "ns1/signer.err",
        "ns1/trusted.conf",
    ]
)


def test_wildcard(run_tests_sh):
    run_tests_sh()
