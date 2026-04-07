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


def bootstrap():
    isctest.log.info("Restart ns1 with -T transferslowly")
    with open("ns1/named.args", "w", encoding="utf-8") as argsfile:
        argsfile.write(
            "-D xfer-ns1 -m record -c named.conf -d 99 -g -T maxcachesize=2097152 -T transferinsecs -T transferslowly"
        )
    return {
        "enable_some_zones": False,
    }


def test_wait_for_zone_retransfer(named_port, ns6):
    isctest.log.info("Wait for at least one message")
    with ns6.watch_log_from_here() as watcher:
        ns6.rndc("retransfer axfr-rndc-retransfer-force.")
        watcher.wait_for_line(
            f"'axfr-rndc-retransfer-force/IN' from 10.53.0.1#{named_port}: received"
        )


def test_cancel_ongoing_retransfer(named_port, ns6):
    isctest.log.info(
        "Issue a retransfer-force command which should cancel the ongoing transfer and start a new one."
    )
    with ns6.watch_log_from_here(timeout=30) as watcher_transfer_success:
        with ns6.watch_log_from_here() as watcher_transfer_shutting_down:
            ns6.rndc("retransfer -force axfr-rndc-retransfer-force.")
            watcher_transfer_shutting_down.wait_for_line(
                f"'axfr-rndc-retransfer-force/IN' from 10.53.0.1#{named_port}: Transfer status: shutting down"
            )
        isctest.log.info("Wait for the new transfer to complete successfully")
        watcher_transfer_success.wait_for_line(
            f"'axfr-rndc-retransfer-force/IN' from 10.53.0.1#{named_port}: Transfer status: success"
        )


def test_min_transfer_rate_in(ns6):
    isctest.log.info("Test min-transfer-rate-in with 5 seconds timeout")
    with ns6.watch_log_from_here() as watcher:
        ns6.rndc("retransfer axfr-min-transfer-rate.")
        watcher.wait_for_line("minimum transfer rate reached: timed out")


def test_max_transfer_time_in(ns6):
    isctest.log.info("Test max-transfer-time-in with 1 second timeout")
    with ns6.watch_log_from_here() as watcher:
        ns6.rndc("retransfer axfr-max-transfer-time.")
        watcher.wait_for_line("maximum transfer time exceeded: timed out")
