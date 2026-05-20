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

from isctest.asyncserver import AsyncDnsServer, IgnoreAllQueries

from ..srtt_ans import DelayedQnameRangeHandler


class Foo1ToFoo299Handler(DelayedQnameRangeHandler):
    max_qname = 299
    delay = 0.08


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=dns.rcode.NOERROR)
    server.install_response_handlers(
        Foo1ToFoo299Handler(),
        IgnoreAllQueries(),
    )
    server.run()


if __name__ == "__main__":
    main()
