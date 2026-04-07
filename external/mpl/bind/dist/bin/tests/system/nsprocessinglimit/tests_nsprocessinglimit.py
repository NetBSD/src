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


def line_to_ips_and_queries(line):
    # dnstap-read output line example
    # 05-Feb-2026 11:00:57.853 RQ 10.53.0.4:38507 -> 10.53.0.3:22047 TCP 56b sub.example.tld/IN/NS
    _, _, _, _, _, dst, _, _, query = line.split(" ", 9)
    ip, _ = dst.split(":", 1)
    return (ip, query)


def extract_dnstap(ns, expectedlen):
    ns.rndc("dnstap -roll 1")
    path = os.path.join(ns.identifier, "dnstap.out.0")
    dnstapread = isctest.run.cmd(
        [isctest.vars.ALL["DNSTAPREAD"], path],
    )

    lines = dnstapread.out.splitlines()
    assert expectedlen == len(lines)
    return map(line_to_ips_and_queries, lines)


def expect_query(expected_query, expected_query_count, ips_and_queries):
    count = 0
    for _, query in ips_and_queries:
        if query == expected_query:
            count += 1
    assert count == expected_query_count


def expect_next_ip_and_query(expected_ips_and_queries, ips_and_queries):
    for expected_ip, expected_query in expected_ips_and_queries:
        ip, query = next(ips_and_queries)
        assert ip == expected_ip
        assert query == expected_query


def test_selfpointedglue_nslimit(ns4):
    msg = isctest.query.create("a.sub.example.tld.", "A")
    res = isctest.query.tcp(msg, ns4.ip)
    isctest.check.servfail(res)

    # The 4 formers lines are request to find sub.example2.tld NSs.
    # The latest 20 are queries to sub.example2.tld NSs.
    ips_and_queries = extract_dnstap(ns4, 24)

    # Checking the begining of the resulution
    expect_next_ip_and_query(
        [
            ("10.53.0.1", "./IN/NS"),
            ("10.53.0.1", "tld/IN/NS"),
            ("10.53.0.2", "example.tld/IN/NS"),
            ("10.53.0.3", "sub.example.tld/IN/NS"),
        ],
        ips_and_queries,
    )
    expect_query("a.sub.example.tld/IN/A", 20, ips_and_queries)
