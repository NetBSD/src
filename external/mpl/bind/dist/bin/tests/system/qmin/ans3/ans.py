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

from isctest.asyncserver import AsyncDnsServer

from qmin_ans import DelayedResponseHandler, EntRcodeChanger, QueryLogHandler


class QueryLogger(QueryLogHandler):
    domains = ["8.2.6.0.1.0.0.2.ip6.arpa.", "a.b.stale.", "zoop.boing.good."]


class ZoopBoingBadHandler(EntRcodeChanger):
    domains = ["zoop.boing.bad."]
    rcode = dns.rcode.NXDOMAIN


class ZoopBoingUglyHandler(EntRcodeChanger):
    domains = ["zoop.boing.ugly."]
    rcode = dns.rcode.FORMERR


class ZoopBoingSlowHandler(DelayedResponseHandler):
    domains = ["zoop.boing.slow."]
    delay = 0.4


def main() -> None:
    server = AsyncDnsServer()
    server.install_response_handlers(
        [
            QueryLogger(),
            ZoopBoingBadHandler(),
            ZoopBoingUglyHandler(),
            ZoopBoingSlowHandler(),
        ]
    )
    server.run()


if __name__ == "__main__":
    main()
