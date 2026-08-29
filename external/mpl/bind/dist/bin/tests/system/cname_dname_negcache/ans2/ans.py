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

import asyncio

from dns import name, rcode, rdataclass, rdatatype, rrset

from isctest.asyncserver import (
    AsyncDnsServer,
    DnsResponseSend,
    QnameQtypeHandler,
    QueryContext,
    StaticResponseHandler,
)

# The attack relies on the resolver caching the positive CNAME/DNAME answer
# *before* it processes the negative answer for the same name.  The negative
# answer must therefore be held back until the positive one has been sent, but
# released again while the negative fetch is still waiting for it.
#
# Releasing it at a fixed wall-clock delay (the original approach) is racy: the
# delay must be larger than the time it takes the resolver to cache the
# positive answer, yet smaller than the resolver's per-query timeout.  Under
# load -- most notably ThreadSanitizer, which slows down `named` but not this
# (wall-clock) server -- those bounds can be violated in either direction,
# making the test either time out (#5946 CI failures) or, worse, silently stop
# exercising the bug.
#
# Instead, gate the negative answer on an event set right after the positive
# answer is sent.  Both queries traverse the same delegation, so any latency in
# reaching this server shifts the positive send and the negative fetch's
# deadline together and cancels out; only the small settle below has to fit
# inside the per-query timeout.
#
# _SETTLE must be longer than the few milliseconds the resolver needs to cache
# the positive answer, and shorter than MINIMUM_QUERY_TIMEOUT (301 ms in
# lib/dns/resolver.c) so the in-flight negative fetch has not given up yet.
_SETTLE = 0.1

_dname_positive_sent = asyncio.Event()
_cname_positive_sent = asyncio.Event()


async def _hold_until_positive_cached(positive_sent: asyncio.Event) -> None:
    await positive_sent.wait()
    await asyncio.sleep(_SETTLE)


def build_rrset(
    qname: name.Name | str,
    rtype: rdatatype.RdataType,
    rdata: str,
    ttl: int = 300,
) -> rrset.RRset:
    return rrset.from_text(qname, ttl, rdataclass.IN, rtype, rdata)


class FooTestNsHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = ["foo.test."]
    qtypes = [rdatatype.NS]
    answer = [build_rrset("foo.test.", rdatatype.NS, "ns.foo.test.")]
    additional = [build_rrset("ns.foo.test.", rdatatype.A, "10.53.0.2")]


class DelayedDnameNegHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = ["foo.test."]
    qtypes = [rdatatype.DNAME]
    authority = [
        build_rrset(
            "foo.test.",
            rdatatype.SOA,
            "ns.test. op.ns.test. 2081509183 86400 3600 3600000 300",
        )
    ]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        await _hold_until_positive_cached(_dname_positive_sent)
        async for response in super().get_responses(qctx):
            yield response


class DnamePosHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = ["a.foo.test."]
    qtypes = [rdatatype.A]
    answer = [
        build_rrset("foo.test.", rdatatype.DNAME, "bar.test."),
        build_rrset("a.foo.test.", rdatatype.CNAME, "a.bar.test."),
    ]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        async for response in super().get_responses(qctx):
            yield response
        _dname_positive_sent.set()


class CnameHandler(QnameQtypeHandler):
    qnames = ["cname.foo.test."]
    qtypes = [rdatatype.CNAME, rdatatype.A]
    answer = [build_rrset("cname.foo.test.", rdatatype.CNAME, "cname.foo.test.")]
    authority = [
        build_rrset(
            "cname.foo.test.",
            rdatatype.SOA,
            "ns.test. op.ns.test. 2081509183 86400 3600 3600000 300",
        )
    ]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        if qctx.qtype == rdatatype.CNAME:
            await _hold_until_positive_cached(_cname_positive_sent)
            qctx.prepare_new_response(with_zone_data=False)
            qctx.response.authority.extend(self.authority)
            yield DnsResponseSend(qctx.response, authoritative=True)
        else:
            qctx.prepare_new_response(with_zone_data=False)
            qctx.response.answer.extend(self.answer)
            yield DnsResponseSend(qctx.response, authoritative=True)
            _cname_positive_sent.set()


def main() -> None:
    server = AsyncDnsServer(default_aa=True, default_rcode=rcode.NOERROR)
    server.install_response_handlers(
        FooTestNsHandler(), DelayedDnameNegHandler(), DnamePosHandler(), CnameHandler()
    )
    server.run()


if __name__ == "__main__":
    main()
