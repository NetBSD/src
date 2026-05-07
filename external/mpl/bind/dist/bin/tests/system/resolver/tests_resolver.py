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
