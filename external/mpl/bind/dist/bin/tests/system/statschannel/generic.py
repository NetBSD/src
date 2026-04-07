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

from collections import defaultdict
from datetime import datetime, timedelta
from time import sleep

import os

import dns.message

import isctest

# ISO datetime format without msec
FMT = "%Y-%m-%dT%H:%M:%SZ"

# The constants were taken from BIND 9 source code (lib/dns/zone.c)
max_refresh = timedelta(seconds=2419200)  # 4 weeks
max_expires = timedelta(seconds=14515200)  # 24 weeks
dayzero = datetime.utcfromtimestamp(0).replace(microsecond=0)

# Wait for the secondary zone files to appear to extract their mtime
MAX_SECONDARY_ZONE_WAITTIME_SEC = 5


# Generic helper functions
def check_expires(expires, min_time, max_time):
    assert expires >= min_time
    assert expires <= max_time


def check_refresh(refresh, min_time, max_time):
    assert refresh >= min_time
    assert refresh <= max_time


def check_loaded(loaded, expected, now):
    # Sanity check the zone timers values
    assert (loaded - expected).total_seconds() < MAX_SECONDARY_ZONE_WAITTIME_SEC
    assert loaded <= now


def check_zone_timers(loaded, expires, refresh, loaded_exp):
    now = datetime.utcnow().replace(microsecond=0)
    # Sanity checks the zone timers values
    if expires is not None:
        check_expires(expires, now, now + max_expires)
    if refresh is not None:
        check_refresh(refresh, now, now + max_refresh)
    check_loaded(loaded, loaded_exp, now)


#
# The output is gibberish, but at least make sure it does not crash.
#
def check_manykeys(name, zone=None):
    # pylint: disable=unused-argument
    assert name == "manykeys"


def zone_mtime(zonedir, name):
    try:
        si = os.stat(os.path.join(zonedir, f"{name}.db"))
    except FileNotFoundError:
        return dayzero

    mtime = datetime.utcfromtimestamp(si.st_mtime).replace(microsecond=0)

    return mtime


def test_zone_timers_primary(fetch_zones, load_timers, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]
    zonedir = kwargs["zonedir"]

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        name, loaded, expires, refresh = load_timers(zone, True)
        mtime = zone_mtime(zonedir, name)
        check_zone_timers(loaded, expires, refresh, mtime)


def test_zone_timers_secondary(fetch_zones, load_timers, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]
    zonedir = kwargs["zonedir"]

    # If any one of the zone files isn't ready, then retry until timeout.
    tries = MAX_SECONDARY_ZONE_WAITTIME_SEC
    while tries >= 0:
        zones = fetch_zones(statsip, statsport)
        again = False
        for zone in zones:
            name, loaded, expires, refresh = load_timers(zone, False)
            mtime = zone_mtime(zonedir, name)
            if (mtime != dayzero) or (tries == 0):
                # mtime was either retrieved successfully or no tries were
                # left, run the check anyway.
                check_zone_timers(loaded, expires, refresh, mtime)
            else:
                tries = tries - 1
                again = True
                break
        if again:
            sleep(1)
        else:
            break


def test_zone_with_many_keys(fetch_zones, load_zone, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        name = load_zone(zone)
        if name == "manykeys":
            check_manykeys(name)


def create_msg(qname, qtype):
    msg = dns.message.make_query(
        qname, qtype, want_dnssec=True, use_edns=0, payload=4096
    )

    return msg


def create_expected(data):
    expected = {
        "dns-tcp-requests-sizes-received-ipv4": defaultdict(int),
        "dns-tcp-responses-sizes-sent-ipv4": defaultdict(int),
        "dns-tcp-requests-sizes-received-ipv6": defaultdict(int),
        "dns-tcp-responses-sizes-sent-ipv6": defaultdict(int),
        "dns-udp-requests-sizes-received-ipv4": defaultdict(int),
        "dns-udp-requests-sizes-received-ipv6": defaultdict(int),
        "dns-udp-responses-sizes-sent-ipv4": defaultdict(int),
        "dns-udp-responses-sizes-sent-ipv6": defaultdict(int),
    }

    for k, v in data.items():
        for kk, vv in v.items():
            expected[k][kk] += vv

    return expected


def update_expected(expected, key, msg):
    msg_len = len(msg.to_wire())
    bucket_num = (msg_len // 16) * 16
    bucket = f"{bucket_num}-{bucket_num + 15}"

    expected[key][bucket] += 1


def check_traffic(data, expected):
    def ordered(obj):
        if isinstance(obj, dict):
            return sorted((k, ordered(v)) for k, v in obj.items())
        if isinstance(obj, list):
            return sorted(ordered(x) for x in obj)
        return obj

    ordered_data = ordered(data)
    ordered_expected = ordered(expected)

    assert len(ordered_data) == 8
    assert len(ordered_expected) == 8
    assert len(data) == len(ordered_data)
    assert len(expected) == len(ordered_expected)

    assert ordered_data == ordered_expected


def test_traffic(fetch_traffic, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]

    data = fetch_traffic(statsip, statsport)
    exp = create_expected(data)

    msg = create_msg("short.example.", "TXT")
    update_expected(exp, "dns-udp-requests-sizes-received-ipv4", msg)
    ans = isctest.query.udp(msg, statsip, attempts=1)
    isctest.check.noerror(ans)
    update_expected(exp, "dns-udp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)

    msg = create_msg("long.example.", "TXT")
    update_expected(exp, "dns-udp-requests-sizes-received-ipv4", msg)
    ans = isctest.query.udp(msg, statsip, attempts=1)
    isctest.check.noerror(ans)
    update_expected(exp, "dns-udp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)

    msg = create_msg("short.example.", "TXT")
    update_expected(exp, "dns-tcp-requests-sizes-received-ipv4", msg)
    ans = isctest.query.tcp(msg, statsip, attempts=1)
    isctest.check.noerror(ans)
    update_expected(exp, "dns-tcp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)

    msg = create_msg("long.example.", "TXT")
    update_expected(exp, "dns-tcp-requests-sizes-received-ipv4", msg)
    ans = isctest.query.tcp(msg, statsip, attempts=1)
    isctest.check.noerror(ans)
    update_expected(exp, "dns-tcp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)
