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
            "dig.out*",
            "dnsrps.cache",
            "dnsrps.conf",
            "dnsrps.zones",
            "proto.*",
            "test*",
            "trusted.conf",
            "ns2/K*",
            "ns2/bl.tld2.db",
            "ns2/tld2s.db",
            "ns3/bl-2.db",
            "ns3/bl-cname.db",
            "ns3/bl-disabled.db",
            "ns3/bl-drop.db",
            "ns3/bl-garden.db",
            "ns3/bl-given.db",
            "ns3/bl-no-op.db",
            "ns3/bl-nodata.db",
            "ns3/bl-nxdomain.db",
            "ns3/bl-passthru.db",
            "ns3/bl-tcp-only.db",
            "ns3/bl-wildcname.db",
            "ns3/bl.db",
            "ns3/bl.tld2.db",
            "ns3/evil-cname.db",
            "ns3/fast-expire.db",
            "ns3/include-rpz.db",
            "ns3/include-rpz.inc",
            "ns3/manual-update-rpz.db",
            "ns3/mixed-case-rpz.db",
            "ns3/named.conf.tmp",
            "ns3/named.stats",
            "ns3/slow-rpz.db",
            "ns3/wild-cname.db",
            "ns5/bl.db",
            "ns5/empty.db",
            "ns5/empty.db.jnl",
            "ns5/example.db",
            "ns5/expire.conf",
            "ns5/fast-expire.db",
            "ns5/named.stats",
            "ns5/policy2.db",
            "ns5/policy2.db.jnl",
            "ns5/rpz-switch",
            "ns6/bl.tld2s.db",
            "ns6/empty.db",
            "ns6/named.stats",
            "ns7/policy2.db",
            "ns8/manual-update-rpz.db",
        ]
    ),
]


def test_rpz_dnsrps(run_tests_sh):
    with open("dnsrps.conf", "w", encoding="utf-8") as conf:
        conf.writelines(
            [
                "dnsrps-options { log-level 3 };"
                "dnsrps-enable yes;"
                'dnsrps-library "../../rpz/testlib/.libs/libdummyrpz.so";'
            ]
        )
    run_tests_sh()
