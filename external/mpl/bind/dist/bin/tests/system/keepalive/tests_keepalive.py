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

import isctest
import pytest


pytestmark = pytest.mark.extra_artifacts(
    ["ns2/named.stats"],
)


def test_dig_tcp_keepalive_handling(named_port, ns2):
    def get_keepalive_options_received():
        ns2.rndc("stats")
        options_received = 0
        with open("ns2/named.stats", "r", encoding="utf-8") as ns2_stats_file:
            for line in ns2_stats_file:
                if "EDNS TCP keepalive option received" in line:
                    options_received = line.split()[0]
        return int(options_received)

    dig = isctest.run.EnvCmd("DIG", f"-p {str(named_port)}")

    isctest.log.info("check that dig handles TCP keepalive in query")
    assert "; TCP-KEEPALIVE" in dig("+qr +keepalive foo.example. @10.53.0.2").out

    isctest.log.info("check that dig added TCP keepalive was received")
    assert get_keepalive_options_received() == 1

    isctest.log.info("check that TCP keepalive is added for TCP responses")
    assert "; TCP-KEEPALIVE" in dig("+tcp +keepalive foo.example. @10.53.0.2").out

    isctest.log.info("check that TCP keepalive requires TCP")
    assert "; TCP-KEEPALIVE" not in dig("+keepalive foo.example. @10.53.0.2").out

    isctest.log.info("check the default keepalive value")
    assert (
        "; TCP-KEEPALIVE: 30.0 secs"
        in dig("+tcp +keepalive foo.example. @10.53.0.3").out
    )

    isctest.log.info("check a keepalive configured value")
    assert (
        "; TCP-KEEPALIVE: 15.0 secs"
        in dig("+tcp +keepalive foo.example. @10.53.0.2").out
    )

    isctest.log.info("check a re-configured keepalive value")
    response = ns2.rndc("tcp-timeouts 300 300 300 200")
    assert "tcp-initial-timeout=300" in response.out
    assert "tcp-idle-timeout=300" in response.out
    assert "tcp-keepalive-timeout=300" in response.out
    assert "tcp-advertised-timeout=200" in response.out
    assert (
        "; TCP-KEEPALIVE: 20.0 secs"
        in dig("+tcp +keepalive foo.example. @10.53.0.2").out
    )

    isctest.log.info("check server config entry")
    base_options_received = get_keepalive_options_received()
    dig("bar.example. @10.53.0.3")
    next_options_received = get_keepalive_options_received()
    assert base_options_received < next_options_received
