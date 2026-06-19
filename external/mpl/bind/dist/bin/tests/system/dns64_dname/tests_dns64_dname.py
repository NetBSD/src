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

import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts([])


def _answer_rrset(response, owner, rdtype):
    rrset = response.get_rrset(
        response.answer,
        dns.name.from_text(owner),
        dns.rdataclass.IN,
        dns.rdatatype.from_text(rdtype),
    )
    assert rrset is not None, response
    return rrset


def _rrset_text(rrset):
    return {rdata.to_text() for rdata in rrset}


@pytest.mark.requires_zones_loaded("ns1", "ns2")
def test_dns64_partial_exclude_after_dname_restart(ns2):
    # The query name is strictly below d.hack.; resolving the synthesized
    # DNAME CNAME restarts lookup at d.hack., where DNS64 filters a mixed
    # AAAA RRset containing one default-excluded mapped address.
    msg = isctest.query.create("d.d.hack.", "AAAA")
    response = isctest.query.udp(msg, ns2.ip, attempts=1)
    isctest.check.noerror(response)

    dname = _answer_rrset(response, "d.hack.", "DNAME")
    assert _rrset_text(dname) == {"hack."}, response

    cname = _answer_rrset(response, "d.d.hack.", "CNAME")
    assert _rrset_text(cname) == {"d.hack."}, response

    aaaa = _answer_rrset(response, "d.hack.", "AAAA")
    assert _rrset_text(aaaa) == {"2001:db8::64"}, response
