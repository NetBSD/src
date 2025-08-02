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

import isctest.mark

pytestmark = [
    isctest.mark.dnsrps_enabled,
    pytest.mark.extra_artifacts(
        [
            "dig.out.*",
            "dnsrps.cache",
            "dnsrps.conf",
            "ans*/ans.run",
            "ns2/*.queries",
            "ns2/*.local",
            "ns2/named.*.conf",
            "ns2/named.conf.header",
        ]
    ),
]


def test_rpzrecurse_dnsrps(run_tests_sh):
    with open("dnsrps.conf", "w", encoding="utf-8") as conf:
        conf.writelines(
            [
                "dnsrps-options { log-level 3 };"
                "dnsrps-enable yes;"
                'dnsrps-library "../../rpz/testlib/.libs/libdummyrpz.so";'
            ]
        )
    run_tests_sh()
