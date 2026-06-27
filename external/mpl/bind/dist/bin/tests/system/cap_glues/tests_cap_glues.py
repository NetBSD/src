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


def test_cap_glues(ns3):
    msg = isctest.query.create("example.tld.", "A")
    isctest.query.udp(msg, ns3.ip)

    with ns3.watch_log_from_here() as watcher:
        ns3.rndc("dumpdb -cache")
        watcher.wait_for_line("dumpdb complete")
    db = isctest.text.TextFile(f"{ns3.identifier}/named_dump.db")

    allowed_suffixes = range(20, 40)
    skipped_suffixes = range(40, 44)

    for n in allowed_suffixes:
        assert len(db.grep(f"10.53.0.{n}")) >= 1
        assert len(db.grep(f"2001:db8::{n}")) >= 1

    for n in skipped_suffixes:
        assert len(db.grep(f"10.53.0.{n}")) == 0
        assert len(db.grep(f"2001:db8::{n}")) == 0
