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

import dns.message
import dns.query
import dns.rcode


TIMEOUT = 10


def create_msg(qname, qtype):
    msg = dns.message.make_query(
        qname, qtype, want_dnssec=True, use_edns=0, payload=4096
    )

    return msg


def udp_query(ip, port, msg):
    ans = dns.query.udp(msg, ip, TIMEOUT, port=port)
    assert ans.rcode() == dns.rcode.NOERROR

    return ans


def tcp_query(ip, port, msg):
    ans = dns.query.tcp(msg, ip, TIMEOUT, port=port)
    assert ans.rcode() == dns.rcode.NOERROR

    return ans


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
    bucket = "{}-{}".format(bucket_num, bucket_num + 15)

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
    port = kwargs["port"]

    data = fetch_traffic(statsip, statsport)
    exp = create_expected(data)

    msg = create_msg("short.example.", "TXT")
    update_expected(exp, "dns-udp-requests-sizes-received-ipv4", msg)
    ans = udp_query(statsip, port, msg)
    update_expected(exp, "dns-udp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)

    msg = create_msg("long.example.", "TXT")
    update_expected(exp, "dns-udp-requests-sizes-received-ipv4", msg)
    ans = udp_query(statsip, port, msg)
    update_expected(exp, "dns-udp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)

    msg = create_msg("short.example.", "TXT")
    update_expected(exp, "dns-tcp-requests-sizes-received-ipv4", msg)
    ans = tcp_query(statsip, port, msg)
    update_expected(exp, "dns-tcp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)

    msg = create_msg("long.example.", "TXT")
    update_expected(exp, "dns-tcp-requests-sizes-received-ipv4", msg)
    ans = tcp_query(statsip, port, msg)
    update_expected(exp, "dns-tcp-responses-sizes-sent-ipv4", ans)
    data = fetch_traffic(statsip, statsport)

    check_traffic(data, exp)
