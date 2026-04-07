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

import dns.rrset
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "ns3/child.example.st",
        "ns5/example.db",
    ]
)


def test_stub_zones_availability(ns3):
    # check that the stub zone has been saved to disk
    assert os.path.exists("ns3/child.example.st")

    # try an AXFR that should be denied (NOTAUTH)
    def axfr_denied():
        msg = isctest.query.create("child.example.", "AXFR")
        res = isctest.query.tcp(msg, "10.53.0.3")
        isctest.check.notauth(res)

    # look for stub zone data without recursion (should not be found)
    def stub_zone_lookout_without_recursion():
        # drop all flags (dns.flags.RD is set by default)
        msg = isctest.query.create("data.child.example.", "TXT")
        msg.flags = 0
        res = isctest.query.tcp(msg, "10.53.0.3")
        isctest.check.noerror(res)
        assert not res.answer
        assert res.authority[0] == dns.rrset.from_text(
            "child.example.", "300", "IN", "NS", "ns2.child.example."
        )
        assert res.additional[0] == dns.rrset.from_text(
            "ns2.child.example.", "300", "IN", "A", "10.53.0.2"
        )

    # look for stub zone data with recursion (should be found)
    def stub_zone_lookout_with_recursion():
        # dns.flags.RD is set by default
        msg = isctest.query.create("data.child.example.", "TXT")
        res = isctest.query.tcp(msg, "10.53.0.3")
        isctest.check.noerror(res)
        assert res.answer[0] == dns.rrset.from_text(
            "data.child.example.", "300", "IN", "TXT", '"some" "test" "data"'
        )

    axfr_denied()
    stub_zone_lookout_without_recursion()
    stub_zone_lookout_with_recursion()

    ns3.stop()
    ns3.start(["--noclean", "--restart", "--port", os.environ["PORT"]])

    axfr_denied()
    stub_zone_lookout_without_recursion()
    stub_zone_lookout_with_recursion()


# check that glue record is correctly transferred from primary when the "minimal-responses" option is on
def test_stub_glue_record_with_minimal_response():
    # ensure zone data were transfered
    assert os.path.exists("ns5/example.db")

    # this query would fail if NS glue wasn't transferred
    msg_txt = isctest.query.create("target.example.", "TXT", dnssec=False)
    res_txt = isctest.query.tcp(msg_txt, "10.53.0.5")
    isctest.check.noerror(res_txt)
    assert res_txt.answer[0] == dns.rrset.from_text(
        "target.example.", "300", "IN", "TXT", '"test"'
    )

    # ensure both IPv4 and IPv6 glue records were transferred
    msg_a = isctest.query.create("ns4.example.", "A")
    res_a = isctest.query.tcp(msg_a, "10.53.0.5")
    assert res_a.answer[0] == dns.rrset.from_text(
        "ns4.example.", "300", "IN", "A", "10.53.0.4"
    )

    msg_aaaa = isctest.query.create("ns4.example.", "AAAA")
    res_aaaa = isctest.query.tcp(msg_aaaa, "10.53.0.5")
    assert res_aaaa.answer[0] == dns.rrset.from_text(
        "ns4.example.", "300", "IN", "AAAA", "fd92:7065:b8e:ffff::4"
    )
