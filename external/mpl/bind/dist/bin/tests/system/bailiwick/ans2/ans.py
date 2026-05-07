"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import AsyncGenerator

import dns.rdatatype
import dns.rrset

from isctest.asyncserver import DnsResponseSend, QueryContext, ResponseAction

from ..bailiwick_ans import ResponseSpoofer, spoofing_server

ATTACKER_IP = "10.53.0.3"
TTL = 3600


class UnsolicitedNsSpoofer(ResponseSpoofer, mode="unsolicited-ns"):

    qname = "trigger.victim."

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        response = qctx.prepare_new_response(with_zone_data=False)

        txt_rrset = dns.rrset.from_text(
            qctx.qname,
            TTL,
            qctx.qclass,
            dns.rdatatype.TXT,
            '"spoofed answer with extra NS"',
        )
        response.answer.append(txt_rrset)

        ns_rrset = dns.rrset.from_text(
            "victim.", TTL, qctx.qclass, dns.rdatatype.NS, "ns.attacker."
        )
        response.authority.append(ns_rrset)

        yield DnsResponseSend(response, authoritative=True)


class ParentGlueSpoofer(ResponseSpoofer, mode="parent-glue"):

    qname = "trigger.victim."

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        response = qctx.prepare_new_response(with_zone_data=False)

        ns_rrset = dns.rrset.from_text(
            "trigger.victim.", TTL, qctx.qclass, dns.rdatatype.NS, "ns.victim."
        )
        response.authority.append(ns_rrset)

        glue_rrset = dns.rrset.from_text(
            "ns.victim.", TTL, qctx.qclass, dns.rdatatype.A, ATTACKER_IP
        )
        response.additional.append(glue_rrset)

        yield DnsResponseSend(response, authoritative=False)


class DnameSpoofer(ResponseSpoofer, mode="dname"):

    qname = "trigger.victim."

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        response = qctx.prepare_new_response(with_zone_data=False)

        cname_rrset = dns.rrset.from_text(
            qctx.qname,
            TTL,
            qctx.qclass,
            dns.rdatatype.CNAME,
            "trigger.attacker.",
        )
        dname_rrset = dns.rrset.from_text(
            "victim.", TTL, qctx.qclass, dns.rdatatype.DNAME, "attacker."
        )
        response.answer.append(cname_rrset)
        response.answer.append(dname_rrset)

        yield DnsResponseSend(response, authoritative=True)


def main() -> None:
    spoofing_server().run()


if __name__ == "__main__":
    main()
