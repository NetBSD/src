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

import isctest

import dns.message


def test_database(servers, templates):
    msg = dns.message.make_query("database.", "SOA")

    # checking pre reload zone
    res = isctest.query.tcp(msg, "10.53.0.1")
    assert res.answer[0] == dns.rrset.from_text(
        "database.",
        86400,
        "IN",
        "SOA",
        "localhost. hostmaster.isc.org. 0 28800 7200 604800 86400",
    )

    templates.render("ns1/named.conf", {"rname": "marka.isc.org."})
    servers["ns1"].rndc("reload")

    # checking post reload zone
    res = isctest.query.tcp(msg, "10.53.0.1")
    assert res.answer[0] == dns.rrset.from_text(
        "database.",
        86400,
        "IN",
        "SOA",
        "localhost. marka.isc.org. 0 28800 7200 604800 86400",
    )
