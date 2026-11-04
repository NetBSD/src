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

from collections.abc import AsyncGenerator

import logging

import dns.rcode
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    CloseConnection,
    ControlCommand,
    ControllableAsyncDnsServer,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
)


class ErraticAxfrHandler(ResponseHandler):
    allowed_actions = ["no-response", "partial-axfr", "complete-axfr"]

    def __init__(self, actions: list[str]) -> None:
        self.actions = actions
        self.counter = 0
        for action in actions:
            assert action in self.allowed_actions

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        action = self.actions[self.counter % len(self.actions)]
        self.counter += 1

        logging.info("current response action: %s", action)

        if action == "no-response":
            yield CloseConnection()
            return

        soa_rr = dns.rrset.from_text(
            qctx.qname, 300, qctx.qclass, dns.rdatatype.SOA, ". . 0 0 0 0 300"
        )
        ns_rr = dns.rrset.from_text(qctx.qname, 300, qctx.qclass, dns.rdatatype.NS, ".")

        qctx.response.answer.append(soa_rr)
        qctx.response.answer.append(ns_rr)

        if action == "partial-axfr":
            yield DnsResponseSend(qctx.response)
        elif action == "complete-axfr":
            qctx.response.answer.append(soa_rr)
            yield DnsResponseSend(qctx.response)
        yield CloseConnection()


class ResponseSequenceCommand(ControlCommand):
    control_subdomain = "response-sequence"

    def __init__(self) -> None:
        self._current_handler: ResponseHandler | None = None

    def handle(
        self, args: list[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> str:
        for action in args:
            if action not in ErraticAxfrHandler.allowed_actions:
                logging.error("invalid %s action '%s'", self, action)
                qctx.response.set_rcode(dns.rcode.SERVFAIL)
                return f"invalid action '{action}'; must be one of {ErraticAxfrHandler.allowed_actions}"

        actions = args

        if self._current_handler is not None:
            server.uninstall_response_handler(self._current_handler)

        self._current_handler = ErraticAxfrHandler(actions)
        server.install_response_handler(self._current_handler)

        msg = f"reponse sequence set to {actions}"
        logging.info(msg)
        return msg


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_control_command(ResponseSequenceCommand())
    server.run()


if __name__ == "__main__":
    main()
