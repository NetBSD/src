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

import os
import os.path

from collections import defaultdict
from datetime import datetime, timedelta

import dns.message
import dns.query
import dns.rcode

# ISO datetime format without msec
fmt = '%Y-%m-%dT%H:%M:%SZ'

# The constants were taken from BIND 9 source code (lib/dns/zone.c)
max_refresh = timedelta(seconds=2419200)  # 4 weeks
max_expires = timedelta(seconds=14515200)  # 24 weeks
now = datetime.utcnow().replace(microsecond=0)
dayzero = datetime.utcfromtimestamp(0).replace(microsecond=0)


TIMEOUT = 10


# Generic helper functions
def check_expires(expires, min_time, max_time):
    assert expires >= min_time
    assert expires <= max_time


def check_refresh(refresh, min_time, max_time):
    assert refresh >= min_time
    assert refresh <= max_time


def check_loaded(loaded, expected):
    # Sanity check the zone timers values
    assert loaded == expected
    assert loaded < now


def check_zone_timers(loaded, expires, refresh, loaded_exp):
    # Sanity checks the zone timers values
    if expires is not None:
        check_expires(expires, now, now + max_expires)
    if refresh is not None:
        check_refresh(refresh, now, now + max_refresh)
    check_loaded(loaded, loaded_exp)


#
# The output is gibberish, but at least make sure it does not crash.
#
def check_manykeys(name, zone=None):
    # pylint: disable=unused-argument
    assert name == "manykeys"


def zone_mtime(zonedir, name):

    try:
        si = os.stat(os.path.join(zonedir, "{}.db".format(name)))
    except FileNotFoundError:
        return dayzero

    mtime = datetime.utcfromtimestamp(si.st_mtime).replace(microsecond=0)

    return mtime


def zone_keyid(nameserver, zone, key):
    with open('{}/{}.{}.id'.format(nameserver, zone, key)) as f:
        keyid = f.read().strip()
        print('{}-{} ID: {}'.format(zone, key, keyid))
    return keyid


def create_msg(qname, qtype):
    msg = dns.message.make_query(qname, qtype, want_dnssec=True,
                                 use_edns=0, payload=4096)

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
    expected = {"dns-tcp-requests-sizes-received-ipv4": defaultdict(int),
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
