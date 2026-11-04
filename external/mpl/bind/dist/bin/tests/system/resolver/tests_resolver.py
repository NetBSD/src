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

import time

import dns.message

import isctest


def test_resolver_cache_reloadfails(ns1, templates):
    ns1.rndc("flush")
    msg = isctest.query.create("www.example.org.", "A")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.noerror(res)
    assert res.answer[0].ttl == 300
    templates.render(
        "ns1/named.conf", {"wrongoption": True}, template="ns1/named2.conf.j2"
    )

    # The first reload fails, and the old cache list will be preserved
    cmd = ns1.rndc("reload", raise_on_exception=False)
    assert cmd.rc != 0

    templates.render("ns1/named.conf", {"wrongoption": False})
    # The second reload succeed, and the cache is still there, as preserved
    # from the old cache list
    ns1.rndc("reload")
    time.sleep(3)
    msg = isctest.query.create("www.example.org.", "A")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.noerror(res)

    # The ttl being lower than 300 (provided by fake authoritative) proves
    # the cache is still in use
    assert res.answer[0].ttl < 300


# GL#5930
def test_resolver_dname_target_filter_attack():
    # Control check - this should return 'attack.example.net. DNAME org.',
    # which then should result in resolving 'www.example.org. AAAA', which
    # should be SERVAIL because example.org is in 'deny-answer-aliases'.
    msg = isctest.query.create("www.example.attack.example.net.", "AAAA")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.servfail(res)

    # Execute the attack - this should return 'attack.example.net. DNAME org.',
    # which then should result in resolving isc.org and caching the DNAME.
    msg = isctest.query.create("isc.attack.example.net.", "A")
    res = isctest.query.udp(msg, "10.53.0.1")
    answer = """;ANSWER
attack.example.net. 300 IN DNAME org.
isc.attack.example.net. 300 IN CNAME isc.org.
isc.org. 300 IN A 1.2.3.4
;AUTHORITY
;ADDITIONAL
"""
    expected_answer = dns.message.from_text(answer)
    isctest.check.noerror(res)
    isctest.check.rrsets_equal(res.answer, expected_answer.answer)
    isctest.check.rrsets_equal(res.authority, expected_answer.authority)
    isctest.check.rrsets_equal(res.additional, expected_answer.additional)

    # Vulnerability check - this should return 'attack.example.net. DNAME org.'
    # which then should result in resolving 'www.example.org. A', which
    # should still be SERVAIL because example.org is in 'deny-answer-aliases',
    # unless the attack on the previous step was successful.
    msg = isctest.query.create("www.example.attack.example.net.", "A")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.servfail(res)

    # Exception check - this should return 'gooddname.example.net. DNAME org.'
    # which then should result in resolving 'www.example.org. A', which
    # should be NOERROR because while example.org is in 'deny-answer-aliases',
    # gooddname.example.net is in the exceptions list.
    msg = isctest.query.create("www.example.gooddname.example.net.", "A")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.noerror(res)
