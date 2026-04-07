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


def test_expiredglue(ns4):
    msg1 = isctest.query.create("a.example.tld.", "A")
    res1 = isctest.query.udp(msg1, ns4.ip)
    isctest.check.noerror(res1)
    isctest.check.rr_count_eq(res1.answer, 1)

    msg2 = isctest.query.create("a.dnshoster.tld.", "A")
    res2 = isctest.query.udp(msg2, ns4.ip)
    isctest.check.rr_count_eq(res2.answer, 1)

    msg3 = isctest.query.create("ns.dnshoster.tld.", "A")
    res3 = isctest.query.udp(msg3, ns4.ip)
    isctest.check.rr_count_eq(res3.answer, 1)

    time.sleep(3)

    # Even if the glue is expired but the delegation is not, named
    # is able to "recover" by looking up the hints again and does
    # not bails out with a fetch loop detection.
    res1_2 = isctest.query.udp(msg1, ns4.ip)
    isctest.check.same_data(res1_2, res1)

    time.sleep(3)
    res2_2 = isctest.query.udp(msg2, ns4.ip)
    isctest.check.same_data(res2_2, res2)

    time.sleep(3)
    res3_2 = isctest.query.udp(msg3, ns4.ip)
    isctest.check.same_data(res3_2, res3)


def test_loopdetected(ns4):
    msg = isctest.query.create("a.missing.tld.", "A")
    with ns4.watch_log_from_here() as watcher:
        res = isctest.query.udp(msg, ns4.ip)

        # However, this is a valid fetch loop, and named detects it.
        watcher.wait_for_line("loop detected resolving 'ns.missing.tld/A'")
        isctest.check.servfail(res)
