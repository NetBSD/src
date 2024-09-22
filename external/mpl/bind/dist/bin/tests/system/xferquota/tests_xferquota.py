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

import glob
import os
import re
import shutil
import signal

import isctest

import dns.message


def test_xferquota(named_port, servers):
    # Changing test zone
    shutil.copyfile("ns1/changing2.db", "ns1/changing.db")
    with open("ns1/named.pid", "r", encoding="utf-8") as pidfile:
        pid = int(pidfile.read())
    os.kill(pid, signal.SIGHUP)
    with servers["ns1"].watch_log_from_start() as watcher:
        watcher.wait_for_line("received SIGHUP signal to reload zones")

    def check_line_count():
        matching_line_count = 0
        # Iterate through zone files and count matching lines (records)
        for file_path in glob.glob("ns2/zone000*.example.bk"):
            zone = dns.zone.from_file(
                file_path, origin=file_path[4:-2], relativize=False
            )
            for name, _ttl, rdata in zone.iterate_rdatas(rdtype="A"):
                if (
                    re.fullmatch("xyzzy.zone[0-9]+.example.", name.to_text())
                    and rdata.to_text() == "10.0.0.2"
                ):
                    matching_line_count += 1
        return matching_line_count == 300

    isctest.run.retry_with_timeout(check_line_count, timeout=360)

    axfr_msg = dns.message.make_query("zone000099.example.", "AXFR")
    a_msg = dns.message.make_query("a.changing.", "A")

    def query_and_compare(msg):
        ns1response = isctest.query.tcp(msg, "10.53.0.1")
        ns2response = isctest.query.tcp(msg, "10.53.0.2")
        isctest.check.noerror(ns1response)
        isctest.check.noerror(ns2response)
        isctest.check.rrsets_equal(ns1response.answer, ns2response.answer)

    query_and_compare(axfr_msg)
    pattern = re.compile(
        f"transfer of 'changing/IN' from 10.53.0.1#{named_port}: "
        f"Transfer completed: .*\\(serial 2\\)"
    )
    with servers["ns2"].watch_log_from_start() as watcher:
        watcher.wait_for_line(
            pattern,
            timeout=30,
        )
    query_and_compare(a_msg)
