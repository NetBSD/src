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

import concurrent.futures
import time

import dns.exception
import dns.rcode
import dns.update
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns2/zone0*.db",
        "ns2/zone0*.jnl",
    ]
)


def rndc_loop(test_state, server):
    while not test_state["finished"]:
        server.rndc("reload", raise_on_exception=False)
        time.sleep(1)


def update_zone(test_state, zone):
    server = "10.53.0.2"
    for i in range(1000):
        if test_state["finished"]:
            return
        update = dns.update.UpdateMessage(zone)
        update.add(f"dynamic-{i}.{zone}", 300, "TXT", f"txt-{i}")
        try:
            response = isctest.query.udp(
                update, server, log_query=False, log_response=False
            )
            assert response.rcode() == dns.rcode.NOERROR
        except dns.exception.Timeout:
            isctest.log.info(f"error: query timeout for {zone}")

    isctest.log.info(f"Update of {server} zone {zone} successful")


# If the test has run to completion without named crashing, it has succeeded.
def test_update_stress():
    test_state = {"finished": False}

    with concurrent.futures.ThreadPoolExecutor() as executor:
        executor.submit(rndc_loop, test_state, "10.53.0.3")

        updaters = []
        for i in range(5):
            zone = f"zone00000{i}.example."
            updaters.append(executor.submit(update_zone, test_state, zone))

        # All the update_zone() tasks are expected to complete within 5
        # minutes.  If they do not, we cannot assert immediately as that will
        # cause the ThreadPoolExecutor context manager to wait indefinitely;
        # instead, we first signal all tasks that it is time to exit and only
        # check whether any task failed to finish within 5 minutes outside of
        # the ThreadPoolExecutor context manager.
        unfinished_tasks = concurrent.futures.wait(updaters, timeout=300).not_done
        test_state["finished"] = True

    assert not unfinished_tasks
