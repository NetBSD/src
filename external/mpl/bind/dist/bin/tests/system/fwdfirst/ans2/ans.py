"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import AsyncDnsServer, DomainHandler, StaticResponseHandler


class PoisonReferralForwarder(DomainHandler, StaticResponseHandler):
    domains = ["fwd.hack."]
    authority = [
        dns.rrset.from_text(
            "hack.", 300, dns.rdataclass.IN, dns.rdatatype.NS, "ns.fwd.hack."
        )
    ]
    additional = [
        dns.rrset.from_text(
            "ns.fwd.hack.", 300, dns.rdataclass.IN, dns.rdatatype.A, "10.53.0.3"
        )
    ]
    rcode = dns.rcode.NOERROR
    authoritative = False


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handler(PoisonReferralForwarder())
    server.run()


if __name__ == "__main__":
    main()
