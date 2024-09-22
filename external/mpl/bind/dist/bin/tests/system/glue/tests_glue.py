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

import dns.message

import isctest


def test_glue_full_glue_set():
    """test that a ccTLD referral gets a full glue set from the root zone"""
    msg = dns.message.make_query("foo.bar.fi", "A")
    msg.flags &= ~dns.flags.RD
    res = isctest.query.udp(msg, "10.53.0.1")

    answer = """;ANSWER
;AUTHORITY
fi. 172800 IN NS HYDRA.HELSINKI.fi.
fi. 172800 IN NS NS.EU.NET.
fi. 172800 IN NS NS.UU.NET.
fi. 172800 IN NS NS.TELE.fi.
fi. 172800 IN NS T.NS.VERIO.NET.
fi. 172800 IN NS PRIFI.EUNET.fi.
;ADDITIONAL
NS.TELE.fi. 172800 IN A 193.210.18.18
NS.TELE.fi. 172800 IN A 193.210.19.19
PRIFI.EUNET.fi. 172800 IN A 193.66.1.146
HYDRA.HELSINKI.fi. 172800 IN A 128.214.4.29
NS.EU.NET. 172800 IN A 192.16.202.11
T.NS.VERIO.NET. 172800 IN A 192.67.14.16
NS.UU.NET. 172800 IN A 137.39.1.3
"""
    expected_answer = dns.message.from_text(answer)

    isctest.check.noerror(res)
    isctest.check.rrsets_equal(res.answer, expected_answer.answer)
    isctest.check.rrsets_equal(res.authority, expected_answer.authority)
    isctest.check.rrsets_equal(res.additional, expected_answer.additional)


def test_glue_no_glue_set():
    """test that out-of-zone glue is not found"""
    msg = dns.message.make_query("example.net.", "A")
    msg.flags &= ~dns.flags.RD
    res = isctest.query.udp(msg, "10.53.0.1")

    answer = """;ANSWER
;AUTHORITY
example.net. 300 IN NS ns2.example.
example.net. 300 IN NS ns1.example.
;ADDITIONAL
"""
    expected_answer = dns.message.from_text(answer)

    isctest.check.noerror(res)
    isctest.check.rrsets_equal(res.answer, expected_answer.answer)
    isctest.check.rrsets_equal(res.authority, expected_answer.authority)
    isctest.check.rrsets_equal(res.additional, expected_answer.additional)
