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

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    QnameHandler,
    ResponseHandler,
    StaticResponseHandler,
    SwitchControlCommand,
)

from ..serve_stale_ans import (
    a_handler,
    fallback_handler,
    ns_handler,
    other_types_handler,
    soa_handler,
)

ANS8_ADDR = "10.53.0.8"


class TargetStaleFallbackHandler(QnameHandler, StaticResponseHandler):
    qnames = ["target.stale."]


def handlers(www_address: str) -> list[ResponseHandler]:
    return [
        soa_handler("TargetStale", "target.stale."),
        ns_handler(
            "TargetStale",
            "target.stale.",
            "ns.target.stale.",
            address=ANS8_ADDR,
            in_answer=True,
        ),
        TargetStaleFallbackHandler(),
        a_handler(
            "NsTargetStale",
            "ns.target.stale.",
            address=ANS8_ADDR,
        ),
        a_handler(
            "WwwTargetStale",
            "www.target.stale.",
            ttl=2,
            address=www_address,
        ),
        other_types_handler(
            "TargetStaleNodata",
            ["ns.target.stale.", "www.target.stale."],
            "target.stale.",
        ),
        fallback_handler("target.stale."),
    ]


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )

    restored = handlers("10.0.0.1")
    server.install_response_handlers(*restored)
    switch_command = SwitchControlCommand(
        {
            "restore": restored,
            "update": handlers("10.0.0.2"),
        }
    )
    server.install_control_command(switch_command)
    server.run()


if __name__ == "__main__":
    main()
