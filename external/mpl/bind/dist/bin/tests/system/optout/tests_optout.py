#!/usr/bin/python3

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


import os
import re
import sys

import isctest
import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.exception
import dns.message
import dns.name
import dns.query
import dns.rcode
import dns.rdataclass
import dns.rdatatype


pytestmark = [
    pytest.mark.skipif(
        sys.version_info < (3, 7), reason="Python >= 3.7 required [GL #3001]"
    ),
    pytest.mark.extra_artifacts(
        [
            "*.out",
            "ns2/*.infile",
            "ns2/*.signed",
            "ns2/*.jnl",
            "ns2/*.jbk",
            "ns2/controls.conf",
            "ns2/dsset-*",
            "ns2/K*",
        ]
    ),
]


def has_nsec3param(zone, response):
    match = rf"{re.escape(zone)}\.\s+\d+\s+IN\s+NSEC3PARAM\s+1\s+0\s+0\s+-"

    for rr in response.answer:
        if re.search(match, rr.to_text()):
            return True

    return False


def do_query(server, qname, qtype, tcp=False):
    msg = isctest.query.create(qname, qtype)
    query_func = isctest.query.tcp if tcp else isctest.query.udp
    response = query_func(msg, server.ip, expected_rcode=dns.rcode.NOERROR)
    return response


def do_xfr(server, qname):
    xfr = dns.zone.Zone(origin=f"{qname}.", relativize=False)
    dns.query.inbound_xfr(
        where=server.ip, txn_manager=xfr, port=int(os.environ["PORT"])
    )
    return xfr


def verify_zone(zone, transfer):
    verify = os.getenv("VERIFY")
    assert verify is not None

    filename = f"{zone}.out"
    with open(filename, "w", encoding="utf-8") as file:
        file.write(transfer.to_text())

    # dnssec-verify command with default arguments.
    verify_cmd = [verify, "-z", "-o", zone, filename]

    verifier = isctest.run.cmd(verify_cmd)

    if verifier.rc != 0:
        isctest.log.error(f"dnssec-verify {zone} failed")

    return verifier.rc == 0


def test_optout(ns2):
    zone = "test"
    expect_nsec3param = True

    # Wait until the provided zone is signed and then verify its DNSSEC data.
    def check_nsec3param():
        response = do_query(ns2, zone, "NSEC3PARAM")
        if expect_nsec3param:
            return has_nsec3param(zone, response)
        return not has_nsec3param(zone, response)

    # check zone is fully signed.
    isctest.run.retry_with_timeout(check_nsec3param, timeout=100)

    # check if zone if DNSSEC valid.
    transfer = do_xfr(ns2, zone)
    assert verify_zone(zone, transfer)


def test_optout_to_nsec(ns2, templates):
    zone = "small.test"
    expect_nsec3param = True

    # Wait until the provided zone is signed and then verify its DNSSEC data.
    def check_nsec3param():
        response = do_query(ns2, zone, "NSEC3PARAM")
        if expect_nsec3param:
            return has_nsec3param(zone, response)
        return not has_nsec3param(zone, response)

    # check zone is fully signed.
    isctest.run.retry_with_timeout(check_nsec3param, timeout=100)

    # check if zone if DNSSEC valid.
    transfer = do_xfr(ns2, zone)
    assert verify_zone(zone, transfer)

    # reconfigure to NSEC.
    data = {
        "reconfiged": True,
    }
    templates.render(f"{ns2.identifier}/named.conf", data)
    ns2.reconfigure()

    # wait until NSEC3PARAM is removed.
    expect_nsec3param = False
    isctest.run.retry_with_timeout(check_nsec3param, timeout=100)

    # check if zone if DNSSEC valid.
    transfer = do_xfr(ns2, zone)
    assert verify_zone(zone, transfer)
