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

import os

import isctest
import isctest.mark

pytestmark = [isctest.mark.with_dnstap]


def line_to_dst_ips(line):
    # dnstap-read output line example
    # 05-Feb-2026 11:00:57.853 RQ 10.53.0.6:38507 -> 10.53.0.3:22047 TCP 56b fooXXX.example./IN/NS
    _, _, _, _, _, dst, _, _, _ = line.split(" ", 9)
    ip, _ = dst.split(":", 1)
    return ip


def extract_dnstap(ns):
    ns.rndc("dnstap -roll 1")
    path = os.path.join(ns.identifier, "dnstap.out.0")
    dnstapread = isctest.run.cmd(
        [isctest.vars.ALL["DNSTAPREAD"], path],
    )

    lines = dnstapread.out.splitlines()
    return map(line_to_dst_ips, lines)


def assert_used_auth(ns, authip):
    ips = extract_dnstap(ns)
    queries = 0
    matches = 0
    for ip in ips:
        queries += 1
        if ip == authip:
            matches += 1
    assert matches > 85
    assert queries <= 115


def test_srtt(ns6):
    for i in range(1, 100):
        msg = isctest.query.create(f"foo{i}.example.", "A")
        res = isctest.query.udp(msg, ns6.ip)
        isctest.check.noerror(res)
        assert len(res.answer[0]) == 1
        res.answer[0].ttl = 300
        assert str(res.answer[0]) == f"foo{i}.example. 300 IN A 10.53.9.9"

    assert_used_auth(ns6, "10.53.0.2")

    for i in range(100, 200):
        msg = isctest.query.create(f"foo{i}.example.", "A")
        res = isctest.query.udp(msg, ns6.ip)
        isctest.check.noerror(res)
        assert len(res.answer[0]) == 1
        res.answer[0].ttl = 300
        assert str(res.answer[0]) == f"foo{i}.example. 300 IN A 10.53.9.9"

    assert_used_auth(ns6, "10.53.0.3")

    for i in range(200, 300):
        msg = isctest.query.create(f"foo{i}.example.", "A")
        res = isctest.query.udp(msg, ns6.ip)
        isctest.check.noerror(res)
        assert len(res.answer[0]) == 1
        res.answer[0].ttl = 300
        assert str(res.answer[0]) == f"foo{i}.example. 300 IN A 10.53.9.9"

    assert_used_auth(ns6, "10.53.0.4")

    for i in range(300, 400):
        msg = isctest.query.create(f"foo{i}.example.", "A")
        res = isctest.query.udp(msg, ns6.ip)
        isctest.check.noerror(res)
        assert len(res.answer[0]) == 1
        res.answer[0].ttl = 300
        assert str(res.answer[0]) == f"foo{i}.example. 300 IN A 10.53.9.9"
    assert_used_auth(ns6, "10.53.0.5")
