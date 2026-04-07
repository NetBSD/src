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

import platform

import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "Kxxx*",
        "dig.out.*",
        "nextpart.out.*",
        "nsupdate.*out*",
        "perl.update_test.out",
        "typelist.out.*",
        "update.in.*",
        "verylarge",
        "ans*/ans.run",
        "ns*/*.jnl",
        "ns*/*.jnl",
        "ns1/ddns.key",
        "ns1/example.db",
        "ns1/keytests.db",
        "ns1/legacy*.key",
        "ns1/many.test.db",
        "ns1/maxjournal.db",
        "ns1/md5.key",
        "ns1/other.db",
        "ns1/sample.db",
        "ns1/sha*.key",
        "ns1/tls.conf",
        "ns1/tls.options",
        "ns1/unixtime.db",
        "ns1/update.db",
        "ns1/yyyymmddvv.db",
        "ns2/example.bk",
        "ns2/sample.db",
        "ns2/update.alt.bk",
        "ns2/update.bk",
        "ns3/dsset-*",
        "ns3/K*",
        "ns3/*.signed",
        "ns3/delegation.test.db",
        "ns3/dnskey.test.db",
        "ns3/example.db",
        "ns3/multisigner.test.db",
        "ns3/nsec3param.test.db",
        "ns3/relaxed.db",
        "ns3/too-big.test.db",
        "ns5/local.db",
        "ns6/2.0.0.2.ip6.addr.db",
        "ns6/in-addr.db",
        "ns7/_default.tsigkeys",
        "ns7/example.com.db",
        "ns7/in-addr.db",
        "ns8/_default.tsigkeys",
        "ns8/example.com.db",
        "ns8/in-addr.db",
        "ns9/_default.tsigkeys",
        "ns9/denyname.example.db",
        "ns9/example.com.db",
        "ns9/in-addr.db",
        "ns10/_default.tsigkeys",
        "ns10/example.com.db",
        "ns10/in-addr.db",
    ]
)


MAX_RUNS = 2 if platform.system() == "FreeBSD" else 1  # GL#3846


@pytest.mark.flaky(max_runs=MAX_RUNS)
def test_nsupdate(run_tests_sh):
    run_tests_sh()
