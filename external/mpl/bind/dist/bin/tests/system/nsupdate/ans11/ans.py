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

"""
GL#5818 Finding 1 regression support — AsyncDnsServer primary.

Serves a minimal zone "sigaxfr.nil." whose AXFR carries two SIG records
at the same owner with different covered types (A and MX) and different
TTLs (600 and 1200).  A buggy secondary running dns_diff_load() with
rdata_covers() that only recognises RRSIG will file both rdatas under
typepair (SIG, 0) with the first tuple's TTL; a fixed secondary keeps
them under (SIG, A) and (SIG, MX) with their distinct TTLs.
"""

from collections.abc import AsyncGenerator

import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)

ZONE = dns.name.from_text("sigaxfr.nil.")
NS_NAME = dns.name.from_text("ns.sigaxfr.nil.")
HOST = dns.name.from_text("host.sigaxfr.nil.")

SOA_TEXT = "ns.sigaxfr.nil. hostmaster.sigaxfr.nil. 1 3600 1200 604800 3600"


def _make_sig_rdata(covered_text):
    """Produce a legacy SIG (24) rdata via RRSIG (46) round-trip."""
    rrsig = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, covered_text)
    wire = rrsig.to_digestable()
    return dns.rdata.from_wire(dns.rdataclass.IN, dns.rdatatype.SIG, wire, 0, len(wire))


class SigAxfrServer(DomainHandler):
    """Serve SOA and AXFR for sigaxfr.nil.; other qtypes get NOERROR/NODATA."""

    domains = ["sigaxfr.nil."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        soa_rrset = dns.rrset.from_text(
            ZONE, 3600, dns.rdataclass.IN, dns.rdatatype.SOA, SOA_TEXT
        )

        if qctx.qtype == dns.rdatatype.SOA:
            resp = qctx.response
            resp.answer.append(soa_rrset)
            yield DnsResponseSend(resp)
            return

        if qctx.qtype != dns.rdatatype.AXFR:
            # Other types: empty NOERROR response.
            yield DnsResponseSend(qctx.response)
            return

        # AXFR: opening SOA, NS, NS's A, two SIG RRs at the same owner
        # with distinct covered types and TTLs, closing SOA.
        resp = qctx.response
        resp.answer.append(soa_rrset)

        ns_rrset = dns.rrset.from_text(
            ZONE, 3600, dns.rdataclass.IN, dns.rdatatype.NS, str(NS_NAME)
        )
        resp.answer.append(ns_rrset)

        a_rrset = dns.rrset.from_text(
            NS_NAME, 3600, dns.rdataclass.IN, dns.rdatatype.A, "10.53.0.11"
        )
        resp.answer.append(a_rrset)

        sig_a = _make_sig_rdata("A 6 2 600 20260331170000 20260318160000 21831 . 0000")
        sig_a_rrset = dns.rrset.RRset(HOST, dns.rdataclass.IN, dns.rdatatype.SIG)
        sig_a_rrset.add(sig_a, ttl=600)
        resp.answer.append(sig_a_rrset)

        sig_mx = _make_sig_rdata(
            "MX 6 2 1200 20260331170000 20260318160000 21831 . 0000"
        )
        sig_mx_rrset = dns.rrset.RRset(HOST, dns.rdataclass.IN, dns.rdatatype.SIG)
        sig_mx_rrset.add(sig_mx, ttl=1200)
        resp.answer.append(sig_mx_rrset)

        # Closing SOA terminates the AXFR.
        resp.answer.append(soa_rrset)

        yield DnsResponseSend(resp)


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handler(SigAxfrServer())
    server.run()


if __name__ == "__main__":
    main()
