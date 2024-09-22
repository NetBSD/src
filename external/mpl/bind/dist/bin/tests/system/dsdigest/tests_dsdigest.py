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


def test_dsdigest_good():
    """Check that validation with enabled digest types works"""
    msg = dns.message.make_query("a.good.", "A", want_dnssec=True)
    res = isctest.query.tcp(
        msg,
        "10.53.0.3",
    )
    isctest.check.noerror(res)
    assert res.flags & dns.flags.AD


def test_dsdigest_bad():
    """Check that validation with not supported digest types
    and "dnssec-must-be-secure yes;" results in SERVFAIL"""
    msg = dns.message.make_query("a.bad.", "A", want_dnssec=True)
    res = isctest.query.tcp(
        msg,
        "10.53.0.3",
    )
    isctest.check.servfail(res)


def test_dsdigest_insecure():
    """Check that validation with not supported digest algorithms is insecure"""
    msg_ds = dns.message.make_query("bad.", "DS", want_dnssec=True)
    res_ds = isctest.query.tcp(
        msg_ds,
        "10.53.0.4",
    )
    isctest.check.noerror(res_ds)
    assert res_ds.flags & dns.flags.AD

    msg_a = dns.message.make_query("a.bad.", "A", want_dnssec=True)
    res_a = isctest.query.tcp(
        msg_a,
        "10.53.0.4",
    )
    isctest.check.noerror(res_a)
    assert not res_a.flags & dns.flags.AD
