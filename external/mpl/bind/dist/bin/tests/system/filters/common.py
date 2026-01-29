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

import dns
from dns import rdataclass, rdatatype

import isctest


ARTIFACTS = [
    "conf/*.conf",
    "ns*/trusted.conf",
    "ns*/*.signed",
    "ns*/K*",
    "ns*/dsset-*",
    "ns*/signer.err",
]


def check_filtertype_only(dest, source, qname, ftype, expected, adflag):
    qname = dns.name.from_text(qname)

    msg = isctest.query.create(qname, ftype)
    res = isctest.query.tcp(msg, dest, source=source)
    isctest.check.noerror(res)

    if adflag:
        isctest.check.adflag(res)
    else:
        isctest.check.noadflag(res)
    a_record = res.get_rrset(res.answer, qname, rdataclass.IN, rdatatype.A)
    aaaa_record = res.get_rrset(res.answer, qname, rdataclass.IN, rdatatype.AAAA)
    if ftype == "aaaa":
        assert not a_record
        if expected:
            assert (
                aaaa_record and aaaa_record[0].address == expected
            ), f"expected AAAA {expected} in ANSWER: {res}"
    else:
        assert not aaaa_record
        if expected:
            assert (
                a_record and a_record[0].address == expected
            ), f"expected A {expected} in ANSWER: {res}"


def check_any(dest, source, qname, expected4, expected6, do):
    qname = dns.name.from_text(qname)
    msg = isctest.query.create(qname, "any", dnssec=do)
    res = isctest.query.tcp(msg, dest, source=source)
    isctest.check.noerror(res)
    a_record = res.get_rrset(res.answer, qname, rdataclass.IN, rdatatype.A)
    if expected4:
        assert (
            a_record and a_record[0].address == expected4
        ), f"expected A {expected4} in ANSWER: {res}"
    else:
        assert not a_record
    aaaa_record = res.get_rrset(res.answer, qname, rdataclass.IN, rdatatype.AAAA)
    if expected6:
        assert (
            aaaa_record and aaaa_record[0].address == expected6
        ), f"expected AAAA {expected6} in ANSWER: {res}"
    else:
        assert not aaaa_record


def check_nodata(dest, source, qname, qtype, do, adflag):
    msg = isctest.query.create(qname, qtype, dnssec=do)
    res = isctest.query.tcp(msg, dest, source=source)
    isctest.check.noerror(res)
    isctest.check.empty_answer(res)
    if adflag:
        isctest.check.adflag(res)
    else:
        isctest.check.noadflag(res)


def check_additional(dest, source, qname, qtype, ftype, expected, adcount):
    msg = isctest.query.create(qname, qtype)
    res = isctest.query.tcp(msg, dest, source=source)
    isctest.check.noerror(res)
    isctest.check.rr_count_eq(res.additional, adcount)
    t = rdatatype.A if ftype == "a" else rdatatype.AAAA
    if expected:
        assert [a for a in res.additional if a.rdtype == t]
    else:
        assert not [a for a in res.additional if a.rdtype == t]


def prime_cache(addr):
    isctest.log.debug("prime cache for recursive testing:")
    # (when testing recursive, we need to prime the cache first with
    # the MX addresses, since additional section data isn't included
    # unless it's been validated.)
    for name in ["mx", "ns"]:
        for zone in ["signed", "unsigned"]:
            for qtype in ["a", "aaaa"]:
                isctest.log.debug(f"{addr}: {name}.{zone}/{qtype}")
                isctest.query.tcp(isctest.query.create(f"{name}.{zone}", qtype), addr)


def check_filter(addr, altaddr, ftype, break_dnssec, recursive):
    qtype = ftype.upper()
    isctest.log.debug(
        f"check that {qtype} is returned when only {qtype} record exists, signed"
    )
    expected = "1.0.0.2" if ftype == "a" else "2001:db8::2"
    check_filtertype_only(
        addr, addr, f"{ftype}-only.signed", ftype, expected, recursive
    )

    isctest.log.debug(
        f"check that {qtype} is returned when only {qtype} record exists, unsigned"
    )
    expected = "1.0.0.5" if ftype == "a" else "2001:db8::5"
    check_filtertype_only(addr, addr, f"{ftype}-only.unsigned", ftype, expected, False)

    isctest.log.debug(
        "check that NODATA/NOERROR is returned when both AAAA and A exist, signed, DO=0"
    )
    check_nodata(addr, addr, "dual.signed", ftype, False, False)

    isctest.log.debug(
        "check that NODATA/NOERROR is returned when both AAAA and A exist, unsigned, DO=0"
    )
    check_nodata(addr, addr, "dual.unsigned", ftype, False, False)

    isctest.log.debug(
        f"check that {qtype} is returned when both AAAA and A exist, signed, DO=1, unless break-dnssec is enabled"
    )
    if break_dnssec:
        check_nodata(addr, addr, "dual.signed", ftype, False, False)
    else:
        expected = "1.0.0.3" if ftype == "a" else "2001:db8::3"
        check_filtertype_only(addr, addr, "dual.signed", ftype, expected, recursive)

    isctest.log.debug(
        "check that NODATA/NOERROR is returned when both AAAA and A exist, unsigned, DO=1"
    )
    check_nodata(addr, addr, "dual.unsigned", ftype, recursive, False)

    isctest.log.debug(
        f"check that {qtype} is returned if both AAAA and A exist and the query source doesn't match the ACL"
    )

    expected = "1.0.0.6" if ftype == "a" else "2001:db8::6"
    check_filtertype_only(addr, altaddr, "dual.unsigned", ftype, expected, False)

    isctest.log.debug(
        f"check that A/AAAA (and not {qtype}) is returned if both AAAA and A exist, signed, qtype=ANY, DO=0"
    )
    expected4 = "1.0.0.3" if ftype == "aaaa" else None
    expected6 = "2001:db8::3" if ftype == "a" else None
    check_any(addr, addr, "dual.signed", expected4, expected6, False)

    isctest.log.debug(
        "check that both A and AAAA are returned if both AAAA and A exist, signed, qtype=ANY, DO=1, unless break-dnssec is enabled"
    )
    if break_dnssec:
        if ftype == "a":
            expected4 = None
        else:
            expected6 = None
        check_any(addr, addr, "dual.signed", expected4, expected6, True)
    else:
        check_any(addr, addr, "dual.signed", "1.0.0.3", "2001:db8::3", True)

    expected4 = "1.0.0.6" if ftype == "aaaa" else None
    expected6 = "2001:db8::6" if ftype == "a" else None

    isctest.log.debug(
        f"check that A/AAAA (and not {qtype}) is returned if both AAAA and A exist, unsigned, qtype=ANY, DO=0"
    )
    check_any(addr, addr, "dual.unsigned", expected4, expected6, False)

    isctest.log.debug(
        f"check that A/AAAA (and not {qtype}) is returned if both AAAA and A exist, unsigned, qtype=ANY, DO=1"
    )
    check_any(addr, addr, "dual.unsigned", expected4, expected6, True)

    isctest.log.debug(
        "check that both A and AAAA are returned if both AAAA and A exist, signed, qtype=ANY, query source does not match ACL"
    )
    check_any(addr, altaddr, "dual.unsigned", "1.0.0.6", "2001:db8::6", True)

    isctest.log.debug(
        f"check that {qtype} is omitted from additional section, qtype=NS, unsigned"
    )
    check_additional(addr, addr, "unsigned", "ns", ftype, False, 1)

    isctest.log.debug(
        f"check that {qtype} is omitted from additional section, qtype=MX, unsigned"
    )
    check_additional(addr, addr, "unsigned", "mx", ftype, False, 2)

    isctest.log.debug(
        f"check that {qtype} is included in additional section, qtype=MX, signed, unless break-dnssec is enabled"
    )
    if break_dnssec:
        check_additional(addr, addr, "signed", "mx", ftype, False, 4)
    else:
        check_additional(addr, addr, "signed", "mx", ftype, True, 8)


def check_filter_other_family(addr, ftype):
    isctest.log.debug(
        "check that the filtered type is returned when both AAAA and A record exists, unsigned, over other family"
    )
    check_filtertype_only(addr, addr, "dual.unsigned", ftype, None, False)

    isctest.log.debug(
        "check that the filtered type is included in additional section, qtype=MX, unsigned, over other family"
    )
    check_additional(addr, addr, "unsigned", "mx", ftype, True, 4)
