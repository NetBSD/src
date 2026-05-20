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


def test_resend_loop_badcookie(ns4):
    expected_log = "exceeded max queries resolving 'test.example/A'"

    msg = dns.message.make_query("test.example", "A")
    with ns4.watch_log_from_here() as watcher:
        res = isctest.query.udp(msg, ns4.ip)
        watcher.wait_for_line(expected_log)

    isctest.check.servfail(res)

    prohibited_log = "query failed (timed out) for test.example/IN/A"
    assert prohibited_log not in ns4.log
