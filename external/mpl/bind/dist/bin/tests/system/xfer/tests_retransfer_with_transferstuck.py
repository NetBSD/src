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


def bootstrap():
    isctest.log.info("Restart ns1 with -T transferstuck")
    with open("ns1/named.args", "w", encoding="utf-8") as argsfile:
        argsfile.write(
            "-D xfer-ns1 -m record -c named.conf -d 99 -g -T maxcachesize=2097152 -T transferinsecs -T transferstuck"
        )
    return {
        "enable_some_zones": False,
        "enable_only_axfr_max_idle_time": True,
    }


def test_retransfer_with_transferstuck(ns6):
    isctest.log.info("Test max-transfer-idle-in with 50 seconds timeout")
    start_time = time.time()
    with ns6.watch_log_from_here(timeout=60) as watcher:
        ns6.rndc("retransfer -force axfr-max-idle-time.")
        watcher.wait_for_line("maximum idle time exceeded: timed out")
    end_time = time.time()
    assert (
        50 <= (end_time - start_time) < 59
    ), "max-transfer-idle-in did not wait for the expected time"
