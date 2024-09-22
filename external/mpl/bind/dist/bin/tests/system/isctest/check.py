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

import shutil
from typing import Any, Optional

import dns.rcode
import dns.message
import dns.zone

import isctest.log

# compatiblity with dnspython<2.0.0
try:
    # In dnspython>=2.0.0, dns.rcode.Rcode class is available
    # pylint: disable=invalid-name
    dns_rcode = dns.rcode.Rcode  # type: Any
except AttributeError:
    # In dnspython<2.0.0, selected rcodes are available as integers directly
    # from dns.rcode
    dns_rcode = dns.rcode


def rcode(message: dns.message.Message, expected_rcode) -> None:
    assert message.rcode() == expected_rcode, str(message)


def noerror(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.NOERROR)


def servfail(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.SERVFAIL)


def rrsets_equal(
    first_rrset: dns.rrset.RRset,
    second_rrset: dns.rrset.RRset,
    compare_ttl: Optional[bool] = False,
) -> None:
    """Compare two RRset (optionally including TTL)"""

    def compare_rrs(rr1, rrset):
        rr2 = next((other_rr for other_rr in rrset if rr1 == other_rr), None)
        assert rr2 is not None, f"No corresponding RR found for: {rr1}"
        if compare_ttl:
            assert rr1.ttl == rr2.ttl

    isctest.log.debug(
        "%s() first RRset:\n%s",
        rrsets_equal.__name__,
        "\n".join([str(rr) for rr in first_rrset]),
    )
    isctest.log.debug(
        "%s() second RRset:\n%s",
        rrsets_equal.__name__,
        "\n".join([str(rr) for rr in second_rrset]),
    )
    for rr in first_rrset:
        compare_rrs(rr, second_rrset)
    for rr in second_rrset:
        compare_rrs(rr, first_rrset)


def zones_equal(
    first_zone: dns.zone.Zone,
    second_zone: dns.zone.Zone,
    compare_ttl: Optional[bool] = False,
) -> None:
    """Compare two zones (optionally including TTL)"""

    isctest.log.debug(
        "%s() first zone:\n%s",
        zones_equal.__name__,
        first_zone.to_text(relativize=False),
    )
    isctest.log.debug(
        "%s() second zone:\n%s",
        zones_equal.__name__,
        second_zone.to_text(relativize=False),
    )
    assert first_zone == second_zone
    if compare_ttl:
        for name, node in first_zone.nodes.items():
            for rdataset in node:
                found_rdataset = second_zone.find_rdataset(
                    name=name, rdtype=rdataset.rdtype
                )
                assert found_rdataset
                assert found_rdataset.ttl == rdataset.ttl


def is_executable(cmd: str, errmsg: str) -> None:
    executable = shutil.which(cmd)
    assert executable is not None, errmsg


def nxdomain(message: dns.message.Message) -> None:
    rcode(message, dns.rcode.NXDOMAIN)


def single_question(message: dns.message.Message) -> None:
    assert len(message.question) == 1, str(message)


def empty_answer(message: dns.message.Message) -> None:
    assert not message.answer, str(message)


def is_response_to(response: dns.message.Message, query: dns.message.Message) -> None:
    single_question(response)
    single_question(query)
    assert query.is_response(response), str(response)
