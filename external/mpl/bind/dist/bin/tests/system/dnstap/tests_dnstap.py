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

import isctest
import isctest.mark
import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns.rrset

pytestmark = [
    isctest.mark.with_dnstap,
    pytest.mark.extra_artifacts(
        [
            "dnstap.out.*",
            "ns*/dnstap.out*",
            "ns2/example.db",
        ]
    ),
]


def test_dnstap_dispatch_socket_addresses(ns3):
    # Send some query to ns3 so that it records something in its dnstap file.
    msg = isctest.query.create("mail.example.", "A")
    res = isctest.query.tcp(msg, "10.53.0.2", expected_rcode=dns.rcode.NOERROR)
    assert res.answer == [
        dns.rrset.from_text("mail.example.", 300, "IN", "A", "10.0.0.2")
    ]

    # Before continuing, roll dnstap file to ensure it is flushed to disk.
    ns3.rndc("dnstap -roll 1")

    # Move the dnstap file aside so that it is retained for troubleshooting.
    os.rename(os.path.join("ns3", "dnstap.out.0"), "dnstap.out.resolver_addresses")

    # Read the contents of the dnstap file using dnstap-read.
    dnstapread = isctest.run.cmd(
        [isctest.vars.ALL["DNSTAPREAD"], "dnstap.out.resolver_addresses"],
    )

    # Check whether all frames contain the expected addresses.
    #
    # Expected dnstap-read output format:
    #
    #     22-Jun-2022 12:09:06.168 RR 10.53.0.3:0 -> 10.53.0.1:7523 TCP ...
    #     22-Jun-2022 12:09:06.168 RR 10.53.0.3:0 <- 10.53.0.1:7523 TCP ...
    #     22-Jun-2022 12:09:06.168 RQ 10.53.0.3:56306 -> 10.53.0.2:7523 UDP ...
    #     22-Jun-2022 12:09:06.168 RQ 10.53.0.3:56306 <- 10.53.0.2:7523 UDP ...
    #
    bad_frames = []
    inspected_frames = 0
    addr_regex = r"^10\.53\.0\.[0-9]+:[0-9]{1,5}$"
    for line in dnstapread.out.splitlines():
        _, _, frame_type, addr1, _, addr2, _ = line.split(" ", 6)
        # Only inspect RESOLVER_QUERY and RESOLVER_RESPONSE frames.
        if frame_type not in ("RQ", "RR"):
            continue
        inspected_frames += 1
        if not re.match(addr_regex, addr1) or not re.match(addr_regex, addr2):
            bad_frames.append(line)

    assert (
        len(bad_frames) == 0
    ), "{} out of {} inspected frames contain unexpected addresses:\n\n{}".format(
        len(bad_frames), inspected_frames, "\n".join(bad_frames)
    )
