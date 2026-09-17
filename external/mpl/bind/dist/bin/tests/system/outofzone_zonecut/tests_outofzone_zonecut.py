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

"""
A zone database that contains nodes above the zone apex (for example a
secondary backup file written by a version that accepted out-of-zone
data in zone transfers) must not let those nodes act as zone cuts:
queries for names inside the zone have to be answered authoritatively
from the zone, and a recursive server must not follow the bogus
delegation or DNAME.
"""

import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest

ZONES = {
    "nszone.example.": "NS",
    "dnamezone.example.": "DNAME",
}


def check_in_zone(response, zone):
    """Nothing in the response may be owned by a name outside the zone."""
    origin = dns.name.from_text(zone)
    for section in (response.answer, response.authority, response.additional):
        for rrset in section:
            assert rrset.name.is_subdomain(
                origin
            ), f"{rrset.name} {dns.rdatatype.to_text(rrset.rdtype)} leaked into the response:\n{response}"


def check_authoritative_a(response, zone):
    isctest.check.noerror(response)
    isctest.check.aaflag(response)
    check_in_zone(response, zone)
    rrset = response.get_rrset(
        response.answer,
        dns.name.from_text(f"www.{zone}"),
        dns.rdataclass.IN,
        dns.rdatatype.A,
    )
    assert rrset is not None, f"no A record in the answer:\n{response}"
    assert [str(rdata) for rdata in rrset] == ["10.0.0.1"]


def check_authoritative_soa(response, zone):
    isctest.check.noerror(response)
    isctest.check.aaflag(response)
    check_in_zone(response, zone)
    rrset = response.get_rrset(
        response.answer,
        dns.name.from_text(zone),
        dns.rdataclass.IN,
        dns.rdatatype.SOA,
    )
    assert rrset is not None, f"no SOA record in the answer:\n{response}"


@pytest.mark.parametrize("zone", ZONES.keys(), ids=ZONES.values())
@pytest.mark.parametrize("server", ["ns2", "ns3"])
def test_above_apex_node_is_not_a_zone_cut(server, zone, request):
    ns = request.getfixturevalue(server)

    msg = isctest.query.create(f"www.{zone}", "A", rd=False)
    check_authoritative_a(isctest.query.udp(msg, ns.ip), zone)

    msg = isctest.query.create(zone, "SOA", rd=False)
    check_authoritative_soa(isctest.query.udp(msg, ns.ip), zone)


@pytest.mark.parametrize("zone", ZONES.keys(), ids=ZONES.values())
def test_resolver_does_not_follow_above_apex_node(ns3, zone):
    # A recursive query for a name in the zone is answered from the zone
    # itself; the out-of-zone NS/DNAME must not start a recursion towards
    # the nameserver it names.
    msg = isctest.query.create(f"www.{zone}", "A")
    check_authoritative_a(isctest.query.udp(msg, ns3.ip), zone)

    # Had the bogus delegation been followed, the "attacker" server would
    # have supplied an "example." DNAME that is now in the cache and
    # rewrites every sibling name under "example.".
    msg = isctest.query.create("sibling.example.", "A", rd=False)
    response = isctest.query.udp(msg, ns3.ip)
    isctest.check.empty_answer(response)
    for section in (response.answer, response.authority, response.additional):
        for rrset in section:
            assert rrset.rdtype not in (
                dns.rdatatype.DNAME,
                dns.rdatatype.CNAME,
            ), f"poisoned cache entry:\n{response}"
