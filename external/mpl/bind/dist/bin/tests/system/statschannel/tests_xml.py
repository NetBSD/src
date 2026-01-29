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
import xml.etree.ElementTree as ET

import pytest

import isctest.mark

pytest.register_assert_rewrite("generic")
import generic

requests = pytest.importorskip("requests")

pytestmark = [
    isctest.mark.with_libxml2,
    pytest.mark.extra_artifacts(
        [
            "ns2/K*",
            "ns2/*.jnl",
            "ns2/*.signed",
            "ns2/dsset-*",
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


# XML helper functions
def fetch_zones_xml(statsip, statsport):
    r = requests.get(
        "http://{}:{}/xml/v3/zones".format(statsip, statsport), timeout=600
    )
    assert r.status_code == 200

    root = ET.fromstring(r.text)

    default_view = None
    for view in root.find("views").iter("view"):
        if view.attrib["name"] == "_default":
            default_view = view
            break
    assert default_view is not None

    return default_view.find("zones").findall("zone")


def fetch_traffic_xml(statsip, statsport):
    def load_counters(data):
        out = {}
        for counter in data.findall("counter"):
            out[counter.attrib["name"]] = int(counter.text)

        return out

    r = requests.get(
        "http://{}:{}/xml/v3/traffic".format(statsip, statsport), timeout=600
    )
    assert r.status_code == 200

    root = ET.fromstring(r.text)

    traffic = {}
    for ip in ["ipv4", "ipv6"]:
        for proto in ["udp", "tcp"]:
            proto_root = root.find("traffic").find(ip).find(proto)
            for counters in proto_root.findall("counters"):
                if counters.attrib["type"] == "request-size":
                    key = "dns-{}-requests-sizes-received-{}".format(proto, ip)
                else:
                    key = "dns-{}-responses-sizes-sent-{}".format(proto, ip)

                values = load_counters(counters)
                traffic[key] = values

    return traffic


def load_timers_xml(zone, primary=True):
    name = zone.attrib["name"]

    loaded_el = zone.find("loaded")
    assert loaded_el is not None
    loaded = datetime.strptime(loaded_el.text, generic.fmt)

    expires_el = zone.find("expires")
    refresh_el = zone.find("refresh")
    if primary:
        assert expires_el is None
        assert refresh_el is None
        expires = None
        refresh = None
    else:
        assert expires_el is not None
        assert refresh_el is not None
        expires = datetime.strptime(expires_el.text, generic.fmt)
        refresh = datetime.strptime(refresh_el.text, generic.fmt)

    return (name, loaded, expires, refresh)


def load_zone_xml(zone):
    name = zone.attrib["name"]

    return name


def test_zone_timers_primary_xml(statsport):
    generic.test_zone_timers_primary(
        fetch_zones_xml,
        load_timers_xml,
        statsip="10.53.0.1",
        statsport=statsport,
        zonedir="ns1",
    )


def test_zone_timers_secondary_xml(statsport):
    generic.test_zone_timers_secondary(
        fetch_zones_xml,
        load_timers_xml,
        statsip="10.53.0.3",
        statsport=statsport,
        zonedir="ns3",
    )


def test_zone_with_many_keys_xml(statsport):
    generic.test_zone_with_many_keys(
        fetch_zones_xml, load_zone_xml, statsip="10.53.0.2", statsport=statsport
    )


@pytest.mark.flaky(max_runs=2)
def test_traffic_xml(statsport):
    generic.test_traffic(fetch_traffic_xml, statsip="10.53.0.2", statsport=statsport)
