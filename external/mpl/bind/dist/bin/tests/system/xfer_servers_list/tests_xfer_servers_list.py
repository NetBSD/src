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

import isctest


def check_soa(ns, serial):
    msg = isctest.query.create("test.", "SOA")
    res = isctest.query.udp(msg, ns.ip)
    isctest.check.noerror(res)
    assert len(res.answer) == 1
    assert (
        res.answer[0].to_text()
        == f"test. 60 IN SOA ns.test. op.test. {serial} 100 100 300 60"
    )


def wait_for_initial_xfrin(ns):
    with ns.watch_log_from_start() as watcher:
        watcher.wait_for_line("Transfer status: success")
    check_soa(ns, 1)


def wait_for_sending_notify(ns1, ns, key_name):
    pattern = Re(
        f"zone test/IN: sending notify to {ns.ip}#[0-9]+ : TSIG \\({key_name}\\)"
    )
    with ns1.watch_log_from_start() as watcher:
        watcher.wait_for_line(pattern)


def test_xfer_servers_list(ns1, ns2, ns3, ns4, templates):
    # First, wait for ns2, ns3 and ns4 to xfrin foo.fr and answer it
    wait_for_initial_xfrin(ns2)
    wait_for_initial_xfrin(ns3)
    wait_for_initial_xfrin(ns4)

    # ns1 initially notifies the secondaries using the respectively configured keys
    # - 10.53.0.2 has the key defined where `secondaries` is used
    # - 10.53.0.3 has the key directly after its IP address
    # - 10.53.0.4 has the key defined where `secondariesbis` is used
    #   (inside `secondaries`), so it uses this one instead of the one
    #   defined where `secondaries` is used.
    # Because the order notification are sent doesn't matter here, we can't use wait_for_sequence
    seq = [(ns2, "notifykey2"), (ns3, "notifykey3"), (ns4, "notifykey4")]
    for ns, key_name in seq:
        wait_for_sending_notify(ns1, ns, key_name)

    # Then, ns1 update foo.fr. It notifies ns2, ns3 and ns4 about it
    templates.render("ns1/test.db", {"serial": 2})
    with ns2.watch_log_from_here() as ns2_watcher, ns3.watch_log_from_here() as ns3_watcher, ns4.watch_log_from_here() as ns4_watcher:
        ns1.rndc("reload")
        ns2_watcher.wait_for_line("Transfer status: success")
        ns3_watcher.wait_for_line("Transfer status: success")
        ns4_watcher.wait_for_line("Transfer status: success")
    check_soa(ns2, 2)
    check_soa(ns3, 2)
    check_soa(ns4, 2)
