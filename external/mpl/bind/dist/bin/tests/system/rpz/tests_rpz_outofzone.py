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

from isctest.instance import NamedInstance


def test_rpz_out_of_zone_owner_name(ns3: NamedInstance) -> None:
    """
    ns3 loads a policy zone holding "com.", whose two labels are fewer than
    the three of the "outofzone.tld2." origin that gets stripped from an
    owner name to build the trigger name.  That used to underflow an
    unsigned label count and fail an assertion, taking named down as the
    policy zone was loaded - so reaching this test at all is most of the
    check.
    """
    assert 'invalid rpz owner name "com"; not within the policy zone' in ns3.log

    # Only the record above was dropped; the rest of the zone still loads.
    assert (
        "rpz: outofzone.tld2: adding node never-queried.example.outofzone.tld2"
        in ns3.log
    )
