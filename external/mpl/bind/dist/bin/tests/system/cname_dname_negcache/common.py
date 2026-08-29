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

from re import compile as Re
from re import escape
from socket import AF_INET, SOCK_DGRAM, socket

import isctest


def run_attack(ns, name1, type1, name2, type2):
    msg1 = isctest.query.create(name1, type1, cd=True)
    msg2 = isctest.query.create(name2, type2, cd=True)
    port = int(isctest.vars.ALL["PORT"])

    with socket(AF_INET, SOCK_DGRAM) as sock:
        # The order the requests go out doesn't matter. What is important is
        # that the first query starts recursion before the second query returns
        # the answer, and the second query returns the answer before the first
        # query returns the answer. (So, when the NOERROR/NODATA comes back from
        # the first query, the cache is queried and we get the positive response
        # cached from the second query attached to the fresp rdataset of the
        # response of the first query.)
        # That ordering is enforced by ans2, which holds back the negative
        # answer to the first query until it has answered the second one (see
        # ans2/ans.py); the resolver must not crash while reconciling them.
        sock.sendto(msg1.to_wire(), (ns.ip, port))
        sock.sendto(msg2.to_wire(), (ns.ip, port))

    # The second query comes back immediately, the resolver caches the DNAME.
    # The first query comes back shortly after, once ans2 has released the
    # negative answer, and should not crash the server.  Wait for the negative
    # SOA for this specific name (not just any foo.test. one) so the test cannot
    # pass on an unrelated record.
    soa = Re(rf"(?<![\w.]){escape(name1)}.*IN\s+SOA\s+ns\.test\.\s+op\.ns\.test\.")
    with ns.watch_log_from_start(timeout=15) as watcher:
        watcher.wait_for_sequence([soa])
