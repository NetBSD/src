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

import dns.message

import isctest


def test_emptyzones(servers, templates):
    # check that switching to automatic empty zones works
    ns1 = servers["ns1"]
    ns1.rndc("reload")
    templates.render("ns1/named.conf", {"automatic_empty_zones": True})
    ns1.rndc("reload")
    msg = dns.message.make_query("version.bind", "TXT", "CH")
    res = isctest.query.tcp(msg, "10.53.0.1")
    isctest.check.noerror(res)

    # check that allow-transfer { none; } works
    msg = dns.message.make_query("10.in-addr.arpa", "AXFR")
    res = isctest.query.tcp(msg, "10.53.0.1")
    isctest.check.refused(res)
