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

from isctest.asyncserver import AsyncDnsServer, QnameQtypeHandler, StaticResponseHandler

VICTIM = "victim.sibling.hack."
POISON_ADDRESS = "6.6.6.6"


def a(name: str) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        name, 300, dns.rdataclass.IN, dns.rdatatype.A, POISON_ADDRESS
    )


class PoisonedAHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = [VICTIM]
    qtypes = [dns.rdatatype.A]
    answer = [a(VICTIM)]


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handler(PoisonedAHandler())
    server.run()


if __name__ == "__main__":
    main()
