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
        "ns1/*.signed",
        "ns1/dsset-nsec3.",
        "ns1/dsset-signed.",
        "ns1/nsec3.db",
        "ns1/signed.db",
        "ns2/example.db",
        "ns2/named.stats",
        "ns2/redirect.db",
        "ns3/K*",
        "ns3/*.signed",
        "ns3/dsset-nsec3.",
        "ns3/dsset-signed.",
        "ns3/nsec3.db",
        "ns3/signed.db",
        "ns4/example.db",
        "ns4/named.stats",
        "ns5/K*",
        "ns5/dsset-*",
        "ns5/*.signed",
        "ns5/root.db",
        "ns5/sign.ns5.*",
        "ns5/signed.db",
        "ns6/signed.db.signed",
    ]
)


def test_redirect(run_tests_sh):
    run_tests_sh()
