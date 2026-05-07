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

"""
Regression test for GL#5006: TOCTOU race in DNS UPDATE SSU table handling.

send_update() and update_action() used to independently read the zone's
SSU table.  If rndc reconfig changed the zone's update policy between
these two reads, the values could diverge, causing an assertion failure.

This test races rndc reconfig (toggling between allow-update and
update-policy) against a stream of DNS UPDATEs to verify that named
survives without crashing.
"""

import threading
import time

import dns.query
import dns.rdatatype
import dns.update
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/*.db",
        "*/*.jnl",
    ]
)


def send_updates(ip, port, stop_event):
    """Send DNS UPDATEs in a tight loop until stopped."""
    n = 0
    while not stop_event.is_set():
        n += 1
        try:
            up = dns.update.UpdateMessage("example.")
            up.add(
                f"test{n}.example.",
                300,
                dns.rdatatype.A,
                f"10.0.0.{n % 256}",
            )
            dns.query.tcp(up, ip, port=port, timeout=2)
        except Exception:  # pylint: disable=broad-exception-caught
            pass


def toggle_config(ns1, templates, stop_event):
    """Toggle zone config between allow-update and update-policy."""
    use_ssu = False
    while not stop_event.is_set():
        use_ssu = not use_ssu
        try:
            templates.render("ns1/named.conf", {"use_ssu": use_ssu})
            ns1.rndc("reconfig")
        except Exception:  # pylint: disable=broad-exception-caught
            pass
        time.sleep(0.01)


def test_ssu_toctou_race(ns1, templates):
    """Race rndc reconfig against DNS UPDATEs -- named must not crash."""
    port = int(isctest.vars.ALL["PORT"])
    stop = threading.Event()

    update_thread = threading.Thread(
        target=send_updates,
        args=("10.53.0.1", port, stop),
    )
    reconfig_thread = threading.Thread(
        target=toggle_config,
        args=(ns1, templates, stop),
    )

    update_thread.start()
    reconfig_thread.start()

    # Let them race for a few seconds
    time.sleep(5)

    stop.set()
    update_thread.join(timeout=10)
    reconfig_thread.join(timeout=10)

    # Restore original config
    templates.render("ns1/named.conf", {"use_ssu": False})
    ns1.rndc("reconfig")

    # Verify named is still alive
    msg = isctest.query.create("ns.example.", "A")
    res = isctest.query.udp(msg, "10.53.0.1")
    isctest.check.noerror(res)
