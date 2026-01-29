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

from datetime import datetime

import pytest

import isctest.mark

pytest.register_assert_rewrite("generic")
import generic

requests = pytest.importorskip("requests")

pytestmark = [
    isctest.mark.with_json_c,
    pytest.mark.extra_artifacts(
        [
            "ns2/*.jnl",
            "ns2/*.signed",
            "ns2/dsset-*",
            "ns2/K*",
            "ns2/dnssec.db.signed",
            "ns2/dnssec.db",
            "ns2/dnssec.*.id",
            "ns2/manykeys.db",
            "ns2/manykeys.*.id",
            "ns2/settime.out.*",
            "ns2/signzone.out.*",
            "ns3/_default.nzd",
            "ns3/example-tcp.db",
            "ns3/example-tls.db",
            "ns3/example.db",
        ]
    ),
]


# JSON helper functions
def fetch_zones_json(statsip, statsport):
    r = requests.get(
        "http://{}:{}/json/v1/zones".format(statsip, statsport), timeout=600
    )
    assert r.status_code == 200

    data = r.json()
    return data["views"]["_default"]["zones"]


def fetch_traffic_json(statsip, statsport):
    r = requests.get(
        "http://{}:{}/json/v1/traffic".format(statsip, statsport), timeout=600
    )
    assert r.status_code == 200

    data = r.json()

    return data["traffic"]


def load_timers_json(zone, primary=True):
    name = zone["name"]

    # Check if the primary zone timer exists
    assert "loaded" in zone
    loaded = datetime.strptime(zone["loaded"], generic.fmt)

    if primary:
        # Check if the secondary zone timers does not exist
        assert "expires" not in zone
        assert "refresh" not in zone
        expires = None
        refresh = None
    else:
        assert "expires" in zone
        assert "refresh" in zone
        expires = datetime.strptime(zone["expires"], generic.fmt)
        refresh = datetime.strptime(zone["refresh"], generic.fmt)

    return (name, loaded, expires, refresh)


def load_zone_json(zone):
    name = zone["name"]

    return name


def test_zone_timers_primary_json(statsport):
    generic.test_zone_timers_primary(
        fetch_zones_json,
        load_timers_json,
        statsip="10.53.0.1",
        statsport=statsport,
        zonedir="ns1",
    )


def test_zone_timers_secondary_json(statsport):
    generic.test_zone_timers_secondary(
        fetch_zones_json,
        load_timers_json,
        statsip="10.53.0.3",
        statsport=statsport,
        zonedir="ns3",
    )


def test_zone_with_many_keys_json(statsport):
    generic.test_zone_with_many_keys(
        fetch_zones_json, load_zone_json, statsip="10.53.0.2", statsport=statsport
    )


@pytest.mark.flaky(max_runs=2)
def test_traffic_json(statsport):
    generic.test_traffic(fetch_traffic_json, statsip="10.53.0.2", statsport=statsport)
