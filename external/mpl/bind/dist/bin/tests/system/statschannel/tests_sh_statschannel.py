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
        "K*",
        "bind9.xsl.1",
        "bind9.xsl.2",
        "compressed.headers",
        "compressed.out",
        "curl.*",
        "dig.out.*",
        "header.in*",
        "json.*",
        "nc.out*",
        "regular.headers",
        "regular.out",
        "xfrins*",
        "xml.*mem",
        "xml.*stats",
        "zones*",
        "ns2/*.jnl",
        "ns2/*.signed",
        "ns2/*.db",
        "ns2/dsset-*",
        "ns2/K*",
        "ns2/dnssec.db",
        "ns2/dnssec.*.id",
        "ns2/manykeys.db",
        "ns2/manykeys.*.id",
        "ns2/named.stats",
        "ns2/settime.out.*",
        "ns2/signzone.out.*",
        "ns3/_default.nzf*",
        "ns3/_default.nzd*",
        "ns3/example-new.db",
        "ns3/example-tcp.db",
        "ns3/example-tls.db",
        "ns3/example.db",
    ]
)


def test_statschannel(run_tests_sh):
    run_tests_sh()
