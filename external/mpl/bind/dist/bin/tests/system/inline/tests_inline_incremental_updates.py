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

import random
import threading

import dns.update
import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "K*",
        "*.out*",
        "*/*.out*",
        "ns*/K*",
        "ns*/dsset-*",
        "ns*/*.bk",
        "ns*/*.db",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/*.nzd",
        "ns*/*.signed",
        "ns*/trusted.conf",
        "ns3/delayedkeys.conf",
        "ns3/removedkeys",
    ]
)


def worker(server, count) -> None:
    zone = "incremental-updates"
    for i in range(count):
        try:
            sub = random.randrange(1000000)
            update_msg = dns.update.UpdateMessage(zone)
            update_msg.add(f"a-{sub}-{i}.{zone}.", 300, "A", "10.0.0.1")
            server.nsupdate(update_msg)
        except Exception:  # pylint: disable=broad-exception-caught
            break


def test_inline_incremental_updates(ns3):
    """
    Flood the server with updates to check how 'receive secure serial'
    is coping with quick incremental updates.
    """
    threads_n = 10
    updates_n = 10
    threads = [
        threading.Thread(target=worker, args=(ns3, updates_n), daemon=True)
        for _ in range(threads_n)
    ]

    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
