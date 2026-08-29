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

import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

from isctest.instance import NamedInstance

import isctest


def _a_records(response, qname: str) -> set[str]:
    rrset = response.get_rrset(
        response.answer,
        dns.name.from_text(qname),
        dns.rdataclass.IN,
        dns.rdatatype.A,
    )
    if rrset is None:
        return set()
    return {rdata.address for rdata in rrset}


@pytest.mark.parametrize("resolver_name", ["ns4", "ns5"])
def test_forward_first_referral_ns_above_forward_zone_not_cached(
    servers: dict[str, NamedInstance],
    resolver_name: str,
) -> None:
    resolver = servers[resolver_name]
    resolver.rndc("flush")

    trigger = isctest.query.create("trigger.fwd.hack.", "A", dnssec=False)
    isctest.query.tcp(trigger, resolver.ip)

    sibling = isctest.query.create("victim.sibling.hack.", "A", dnssec=False)
    response = isctest.query.tcp(sibling, resolver.ip)
    isctest.check.noerror(response)

    assert _a_records(response, "victim.sibling.hack.") == {"203.0.113.42"}
