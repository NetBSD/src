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

import dns.message
import dns.zone

import isctest


def test_masterfile_include_semantics():
    """Test master file $INCLUDE semantics"""
    msg_axfr = isctest.query.create("include.", "AXFR")
    res_axfr = isctest.query.tcp(msg_axfr, "10.53.0.1")
    axfr_include_semantics = """;ANSWER
include.			300	IN	SOA	ns.include. hostmaster.include. 1 3600 1800 1814400 3600
include.			300	IN	NS	ns.include.
a.include.			300	IN	A	10.0.0.1
a.include.			300	IN	A	10.0.0.99
a.a.include.		300	IN	A	10.0.1.1
b.foo.a.include.	300	IN	A	10.0.2.2
b.include.			300	IN	A	10.0.0.2
a.b.include.		300	IN	A	10.0.1.1
c.b.include.		300	IN	A	10.0.0.3
b.foo.b.include.	300	IN	A	10.0.2.2
ns.include.			300	IN	A	127.0.0.1
"""
    expected = dns.message.from_text(axfr_include_semantics)
    isctest.check.rrsets_equal(res_axfr.answer, expected.answer, compare_ttl=True)


def test_masterfile_bind_8_compat_semantics():
    """Test master file BIND 8 TTL and $TTL semantics compatibility"""
    msg_axfr = isctest.query.create("ttl1.", "AXFR")
    res_axfr = isctest.query.tcp(msg_axfr, "10.53.0.1")
    axfr_ttl_semantics = """;ANSWER
ttl1.		3	IN	SOA	ns.ttl1. hostmaster.ttl1. 1 3600 1800 1814400 3
ttl1.		3	IN	NS	ns.ttl1.
a.ttl1.		3	IN	TXT	"soa minttl 3"
b.ttl1.		2	IN	TXT	"explicit ttl 2"
c.ttl1.		3	IN	TXT	"soa minttl 3"
d.ttl1.		1	IN	TXT	"default ttl 1"
e.ttl1.		4	IN	TXT	"explicit ttl 4"
f.ttl1.		1	IN	TXT	"default ttl 1"
ns.ttl1.	3	IN	A	10.53.0.1
"""
    expected = dns.message.from_text(axfr_ttl_semantics)
    isctest.check.rrsets_equal(res_axfr.answer, expected.answer, compare_ttl=True)


def test_masterfile_rfc_1035_semantics():
    """Test master file RFC1035 TTL and $TTL semantics"""
    msg_axfr = isctest.query.create("ttl2.", "AXFR")
    res_axfr = isctest.query.tcp(msg_axfr, "10.53.0.1")
    axfr_ttl_semantics = """;ANSWER
ttl2.		1	IN	SOA	ns.ttl2. hostmaster.ttl2. 1 3600 1800 1814400 3
ttl2.		1	IN	NS	ns.ttl2.
a.ttl2.		1	IN	TXT	"inherited ttl 1"
b.ttl2.		2	IN	TXT	"explicit ttl 2"
c.ttl2.		2	IN	TXT	"inherited ttl 2"
d.ttl2.		3	IN	TXT	"default ttl 3"
e.ttl2.		2	IN	TXT	"explicit ttl 2"
f.ttl2.		3	IN	TXT	"default ttl 3"
ns.ttl2.	1	IN	A	10.53.0.1
"""
    expected = dns.message.from_text(axfr_ttl_semantics)
    isctest.check.rrsets_equal(res_axfr.answer, expected.answer, compare_ttl=True)


def test_masterfile_missing_master_file():
    """Test nameserver running with a missing master file"""
    msg_soa = isctest.query.create("example.", "SOA")
    res_soa = isctest.query.tcp(msg_soa, "10.53.0.2")
    expected_soa_rr = """;ANSWER
example.	300	IN	SOA	mname1. . 2010042407 20 20 1814400 3600
"""
    expected = dns.message.from_text(expected_soa_rr)
    isctest.check.rrsets_equal(res_soa.answer, expected.answer, compare_ttl=True)


def test_masterfile_missing_master_file_servfail():
    """Test nameserver returning SERVFAIL for a missing master file"""
    msg_soa = isctest.query.create("missing.", "SOA")
    res_soa = isctest.query.tcp(msg_soa, "10.53.0.2")
    isctest.check.servfail(res_soa)


def test_masterfile_owner_inheritance():
    """Test owner inheritance after $INCLUDE"""
    cmd = isctest.run.cmd(
        [
            os.environ["CHECKZONE"],
            "-D",
            "-q",
            "example",
            "zone/inheritownerafterinclude.db",
        ]
    )
    owner_inheritance_zone = """
example.	0	IN	SOA	. . 0 0 0 0 0
example.	0	IN	TXT	"this should be at the zone apex"
example.	0	IN	NS	.
"""
    checker_zone = dns.zone.from_text(cmd.out, origin="example.")
    expected = dns.zone.from_text(owner_inheritance_zone, origin="example.")
    isctest.check.zones_equal(checker_zone, expected, compare_ttl=True)
