############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

import helper


def test_zone_timers_primary(fetch_zones, load_timers, **kwargs):

    statsip = kwargs['statsip']
    statsport = kwargs['statsport']
    zonedir = kwargs['zonedir']

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        (name, loaded, expires, refresh) = load_timers(zone, True)
        mtime = helper.zone_mtime(zonedir, name)
        helper.check_zone_timers(loaded, expires, refresh, mtime)


def test_zone_timers_secondary(fetch_zones, load_timers, **kwargs):

    statsip = kwargs['statsip']
    statsport = kwargs['statsport']
    zonedir = kwargs['zonedir']

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        (name, loaded, expires, refresh) = load_timers(zone, False)
        mtime = helper.zone_mtime(zonedir, name)
        helper.check_zone_timers(loaded, expires, refresh, mtime)


def test_zone_with_many_keys(fetch_zones, load_zone, **kwargs):

    statsip = kwargs['statsip']
    statsport = kwargs['statsport']

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        name = load_zone(zone)
        if name == 'manykeys':
            helper.check_manykeys(name)


def test_traffic(fetch_traffic, **kwargs):

    statsip = kwargs['statsip']
    statsport = kwargs['statsport']
    port = kwargs['port']

    data = fetch_traffic(statsip, statsport)
    exp = helper.create_expected(data)

    msg = helper.create_msg("short.example.", "TXT")
    helper.update_expected(exp, "dns-udp-requests-sizes-received-ipv4", msg)
    ans = helper.udp_query(statsip, port, msg)
    helper.update_expected(exp, "dns-udp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    helper.check_traffic(data, exp)

    msg = helper.create_msg("long.example.", "TXT")
    helper.update_expected(exp, "dns-udp-requests-sizes-received-ipv4", msg)
    ans = helper.udp_query(statsip, port, msg)
    helper.update_expected(exp, "dns-udp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    helper.check_traffic(data, exp)

    msg = helper.create_msg("short.example.", "TXT")
    helper.update_expected(exp, "dns-tcp-requests-sizes-received-ipv4", msg)
    ans = helper.tcp_query(statsip, port, msg)
    helper.update_expected(exp, "dns-tcp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    helper.check_traffic(data, exp)

    msg = helper.create_msg("long.example.", "TXT")
    helper.update_expected(exp, "dns-tcp-requests-sizes-received-ipv4", msg)
    ans = helper.tcp_query(statsip, port, msg)
    helper.update_expected(exp, "dns-tcp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    helper.check_traffic(data, exp)
