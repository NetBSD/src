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

pytestmark = [
    pytest.mark.algorithm_set("ecc_default"),
    pytest.mark.extra_artifacts(
        [
            "delv.*",
            "dig.out.*",
            "dsset-*",
            "rndc.out.*",
            "signer.out.*",
            "ns*/dsset-*",
            "ns*/*.bk",
            "ns*/*.jnl",
            "ns*/K*",
            "ns*/island.conf",
            "ns*/managed.conf",
            "ns*/named.secroots",
            "ns*/private.conf",
            "ns*/signer.out.*",
            "ns*/trusted.conf",
            "ns*/*.signed",
            "ns1/managed.key",
            "ns1/managed.key.id",
            "ns1/root.db.orig",
            "ns1/root.db.tmp",
            "ns1/zone.key",
            "ns2/managed-keys.bind",
            "ns2/managed1.conf",
            "ns3/broken.conf",
            "ns3/managed-keys.bind",
            "ns4/nope",
            "ns5/managed-keys.bind",
            "ns5/named.args",
            "ns7/view1.mkeys",
            "ns7/view2.mkeys",
        ]
    ),
]


@pytest.mark.flaky(max_runs=2)  # GL#3098
def test_mkeys(run_tests_sh):
    run_tests_sh()
