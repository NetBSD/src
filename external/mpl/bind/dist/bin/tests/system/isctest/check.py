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

import difflib
import shutil
from typing import cast, List, Optional

import dns.edns
import dns.flags
import dns.message
import dns.rcode
import dns.zone

import isctest.log
from isctest.compat import dns_rcode, EDECode, EDEOption


def rcode(message: dns.message.Message, expected_rcode) -> None:
    assert message.rcode() == expected_rcode, str(message)


def noerror(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.NOERROR)


def notimp(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.NOTIMP)


def refused(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.REFUSED)


def servfail(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.SERVFAIL)


def adflag(message: dns.message.Message) -> None:
    assert (message.flags & dns.flags.AD) != 0, str(message)


def noadflag(message: dns.message.Message) -> None:
    assert (message.flags & dns.flags.AD) == 0, str(message)


def rdflag(message: dns.message.Message) -> None:
    assert (message.flags & dns.flags.RD) != 0, str(message)


def nordflag(message: dns.message.Message) -> None:
    assert (message.flags & dns.flags.RD) == 0, str(message)


def raflag(message: dns.message.Message) -> None:
    assert (message.flags & dns.flags.RA) != 0, str(message)


def noraflag(message: dns.message.Message) -> None:
    assert (message.flags & dns.flags.RA) == 0, str(message)


def _extract_ede_options(
    message: dns.message.Message,
) -> List[EDEOption]:
    """Extract EDE options from the DNS message."""
    return cast(
        List[EDEOption],
        [
            option
            for option in message.options
            if option.otype == dns.edns.OptionType.EDE
        ],
    )


def noede(message: dns.message.Message) -> None:
    """Check that message contains no EDE option."""
    if not hasattr(dns.edns, "EDECode"):
        # dnspython<2.2.0 doesn't support EDE, skip check
        return

    ede_options = _extract_ede_options(message)
    assert not ede_options, f"unexpected EDE options {ede_options} in {message}"


def ede(
    message: dns.message.Message, code: EDECode, text: Optional[str] = None
) -> None:
    """Check if message contains expected EDE code (and its text)."""
    if not hasattr(dns.edns, "EDECode"):
        # dnspython<2.2.0 doesn't support EDE, skip check
        return

    msg_opts = _extract_ede_options(message)
    matching_opts = [opt for opt in msg_opts if opt.code == code]

    assert matching_opts, f"missing EDE code {code} in {message}"

    if text is None:
        return

    # check at least one matching EDE option has the required text
    for opt in matching_opts:
        if opt.text == text:
            return
    opt_str = ", ".join([opt.to_text() for opt in matching_opts])
    assert False, f'EDE text "{text}" not found in [{opt_str}]'


def section_equal(first_section: list, second_section: list) -> None:
    for rrset in first_section:
        assert (
            rrset in second_section
        ), f"No corresponding RRset found in second section: {rrset}"
    for rrset in second_section:
        assert (
            rrset in first_section
        ), f"No corresponding RRset found in first section: {rrset}"


def same_data(res1: dns.message.Message, res2: dns.message.Message):
    section_equal(res1.question, res2.question)
    section_equal(res1.answer, res2.answer)
    section_equal(res1.authority, res2.authority)
    section_equal(res1.additional, res2.additional)
    assert res1.rcode() == res2.rcode()


def same_answer(res1: dns.message.Message, res2: dns.message.Message):
    section_equal(res1.question, res2.question)
    section_equal(res1.answer, res2.answer)
    assert res1.rcode() == res2.rcode()


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


def named_alive(named_proc, resolver_ip):
    assert named_proc.poll() is None, "named isn't running"
    msg = isctest.query.create("version.bind", "TXT", "CH")
    isctest.query.tcp(msg, resolver_ip, expected_rcode=dns_rcode.NOERROR)


def notauth(message: dns.message.Message) -> None:
    rcode(message, dns.rcode.NOTAUTH)


def nxdomain(message: dns.message.Message) -> None:
    rcode(message, dns.rcode.NXDOMAIN)


def single_question(message: dns.message.Message) -> None:
    assert len(message.question) == 1, str(message)


def empty_answer(message: dns.message.Message) -> None:
    assert not message.answer, str(message)


def rr_count_eq(section: list, expected: int):
    # NOTE: OPT and TSIG records aren't included in the count for ADDITIONAL section
    count = sum(len(rrset) for rrset in section)
    assert count == expected, str(section)


def is_response_to(response: dns.message.Message, query: dns.message.Message) -> None:
    single_question(response)
    single_question(query)
    assert query.is_response(response), str(response)


def file_contents_equal(file1, file2):
    def normalize_line(line):
        # remove trailing&leading whitespace and replace multiple whitespaces
        return " ".join(line.split())

    def read_lines(file_path):
        with open(file_path, "r", encoding="utf-8") as file:
            return [normalize_line(line) for line in file.readlines()]

    lines1 = read_lines(file1)
    lines2 = read_lines(file2)

    differ = difflib.Differ()
    diff = differ.compare(lines1, lines2)

    for line in diff:
        assert not line.startswith("+ ") and not line.startswith(
            "- "
        ), f'file contents of "{file1}" and "{file2}" differ'
