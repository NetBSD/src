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

import dns.rrset
import dns.zone
import pytest


def zone_contains(
    zone: dns.zone.Zone, rrset: dns.rrset.RRset, compare_ttl=False
) -> bool:
    """Check if a zone contains RRset"""

    def compare_rrs(rr1, rrset):
        rr2 = next((other_rr for other_rr in rrset if rr1 == other_rr), None)
        if rr2 is None:
            return False
        if compare_ttl:
            return rr1.ttl == rr2.ttl
        return True

    for _, node in zone.nodes.items():
        for rdataset in node:
            for rr in rdataset:
                if compare_rrs(rr, rrset):
                    return True

    return False


def file_contents_contain(file, substr):
    with open(file, "r", encoding="utf-8") as fp:
        for line in fp:
            if f"{substr}" in line:
                return True
    return False


def param(*args, **kwargs):
    if "id" not in kwargs:
        kwargs["id"] = args[0]  # use first argument  as test ID
    return pytest.param(*args, **kwargs)
