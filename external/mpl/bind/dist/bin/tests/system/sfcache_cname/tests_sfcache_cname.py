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


# A SERVFAIL produced while following a CNAME must be cached against the
# original query name, not the CNAME target.
#
# ns1 serves "foo.tld1 CNAME tld2" and "tld2 A 1.2.3.4"; ns2 forwards
# both zones to it with "max-query-count 2". Resolving "foo.tld1/A"
# follows the CNAME to "tld2" and then exhausts the query budget, so the
# client gets SERVFAIL. That failure must be cached under the original
# name ("foo.tld1"), so a subsequent direct query for the CNAME target
# ("tld2") is not blocked by the SERVFAIL cache and resolves normally.
def test_sfcache_cname(ns2):
    msg = dns.message.make_query("foo.tld1.", "A")
    res = isctest.query.udp(msg, ns2.ip)
    isctest.check.servfail(res)

    msg = dns.message.make_query("tld2.", "A")
    res = isctest.query.udp(msg, ns2.ip)
    isctest.check.noerror(res)
