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

import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "black.out",
        "dig.out.*",
        "insecure.nodata.out",
        "insecure.nxdomain.out",
        "insecure.wild.out",
        "insecure.wildcname.out",
        "insecure.wildnodata1nsec.out",
        "insecure.wildnodata2nsec.out",
        "insecure.wildnodata2nsecafterdata.out",
        "json.out*",
        "minimal.nxdomain.out",
        "no-apex-covering.out",
        "nodata.out",
        "nxdomain.out",
        "wild.out",
        "wildcname.out",
        "wildnodata1nsec.out",
        "wildnodata2nsec.out",
        "wildnodata2nsecafterdata.out",
        "xml.out*",
        "ns*/named.stats",
        "ns*/statistics-channels.conf",
        "ns1/K*+*+*.key",
        "ns1/K*+*+*.private",
        "ns1/dnamed.db",
        "ns1/dnamed.db.signed",
        "ns1/dsset-*",
        "ns1/example.db",
        "ns1/example.db.signed",
        "ns1/insecure.example.db",
        "ns1/insecure.example.db.signed",
        "ns1/minimal.db",
        "ns1/minimal.db.signed",
        "ns1/no-apex-covering.db",
        "ns1/no-apex-covering.db.signed",
        "ns1/root.db",
        "ns1/root.db.signed",
        "ns1/soa-without-dnskey.db",
        "ns1/soa-without-dnskey.db.signed",
        "ns1/trusted.conf",
    ]
)


def test_synthfromdnssec(run_tests_sh):
    run_tests_sh()
