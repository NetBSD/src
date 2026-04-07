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

import dns.message
import dns.rrset
import pytest

from isctest.instance import NamedInstance

import isctest


@pytest.fixture(autouse=True)
def autouse_flush_resolver_cache(servers: dict[str, NamedInstance]) -> None:
    servers["ns4"].rndc("flush")


def set_spoofing_mode(ans1: str, ans2: str) -> None:
    for ip, mode in (("10.53.0.1", ans1), ("10.53.0.2", ans2)):
        msg = dns.message.make_query(f"{mode}.set-spoofing-mode._control.", "TXT")
        res = isctest.query.tcp(msg, ip)
        isctest.check.noerror(res)


def prime_cache(ns4: NamedInstance) -> None:
    msg = dns.message.make_query("prime.victim.", "TXT")
    res = isctest.query.tcp(msg, ns4.ip)
    isctest.check.noerror(res)

    assert res.answer[0] == dns.rrset.from_text(
        "prime.victim.",
        0,
        "IN",
        "TXT",
        '"this record is used for priming the cache of the targeted resolver"',
    )


def send_trigger_query(ns4: NamedInstance, qname: str) -> None:
    msg = dns.message.make_query(qname, "TXT")
    isctest.query.tcp(msg, ns4.ip)
    # The contents of the resolver's response to the trigger query do not
    # matter, so they are not checked in any way; what matters is whether the
    # spoofed response succeeded in hijacking the "victim." domain, which is
    # checked below.


def check_domain_hijack(ns4: NamedInstance) -> None:
    # Not necessary for triggering bugs, but useful for troubleshooting test
    # behavior.
    ns4.rndc("dumpdb -cache")

    msg = dns.message.make_query("only-if-hijacked.victim.", "TXT")
    res = isctest.query.tcp(msg, ns4.ip)
    isctest.check.nxdomain(res)

    msg = dns.message.make_query("canary.victim.", "TXT")
    res = isctest.query.tcp(msg, ns4.ip)
    isctest.check.noerror(res)

    assert res.answer[0] == dns.rrset.from_text(
        "canary.victim.",
        0,
        "IN",
        "TXT",
        '"correct answer from the domain under attack"',
    )


def test_bailiwick_sibling_ns_referral(servers: dict[str, NamedInstance]) -> None:
    set_spoofing_mode(ans1="sibling-ns", ans2="none")

    ns4 = servers["ns4"]
    send_trigger_query(ns4, "trigger.")
    check_domain_hijack(ns4)


def test_bailiwick_unsolicited_authority(servers: dict[str, NamedInstance]) -> None:
    set_spoofing_mode(ans1="none", ans2="unsolicited-ns")

    ns4 = servers["ns4"]
    prime_cache(ns4)
    send_trigger_query(ns4, "trigger.victim.")
    check_domain_hijack(ns4)


def test_bailiwick_parent_glue(servers: dict[str, NamedInstance]) -> None:
    set_spoofing_mode(ans1="none", ans2="parent-glue")

    ns4 = servers["ns4"]
    prime_cache(ns4)
    send_trigger_query(ns4, "trigger.victim.")

    isctest.log.info("Waiting 61 seconds for the ns.victim. ADB entry to expire")
    time.sleep(61)

    check_domain_hijack(ns4)


def test_bailiwick_spoofed_dname(servers: dict[str, NamedInstance]) -> None:
    set_spoofing_mode(ans1="none", ans2="dname")

    ns4 = servers["ns4"]
    send_trigger_query(ns4, "trigger.victim.")
    check_domain_hijack(ns4)
