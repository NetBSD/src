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
import dns.rdatatype

from isctest.asyncserver import (
    ControllableAsyncDnsServer,
    QnameHandler,
    QnameQtypeHandler,
    ResponseHandler,
    StaticResponseHandler,
    ToggleResponsesCommand,
)

from ..serve_stale_ans import (
    a_handler,
    cname_a_handler,
    cname_handler,
    fallback_handler,
    ns_handler,
    other_types_handler,
    rrset,
    soa,
    soa_handler,
    txt_handler,
)


class NxdomainExampleHandler(QnameHandler, StaticResponseHandler):
    qnames = ["nxdomain.example."]
    rcode = dns.rcode.NXDOMAIN
    authority = [soa("example.", ttl=2)]


# A negative answer that stays fresh for the whole run of a test, so that a
# resolver refreshing it can only be doing so because it wrongly considers
# the cached entry stale.
class LongttlNodataExampleHandler(QnameHandler, StaticResponseHandler):
    qnames = ["longttl-nodata.example."]
    authority = [soa("example.", ttl=600, minimum=600)]


class LongttlNxdomainExampleHandler(QnameHandler, StaticResponseHandler):
    qnames = ["longttl-nxdomain.example."]
    rcode = dns.rcode.NXDOMAIN
    authority = [soa("example.", ttl=600, minimum=600)]


class OthertypeExampleCaaHandler(QnameQtypeHandler, StaticResponseHandler):
    qnames = ["othertype.example."]
    qtypes = [dns.rdatatype.CAA]
    answer = [
        rrset(
            "othertype.example.",
            2,
            dns.rdatatype.CAA,
            '0 issue "ca1.example.net"',
        )
    ]


class SourceStaleFallbackHandler(QnameHandler, StaticResponseHandler):
    qnames = ["source.stale."]


def handlers() -> list[ResponseHandler]:
    return [
        a_handler("NsExample", "ns.example."),
        ns_handler("Example", "example.", "ns.example."),
        soa_handler("Example", "example."),
        other_types_handler("Example", ["example.", "ns.example."], "example."),
        other_types_handler("NodataExample", "nodata.example.", "example.", ttl=2),
        LongttlNodataExampleHandler(),
        LongttlNxdomainExampleHandler(),
        txt_handler(
            "DataExample",
            "data.example.",
            "A text record with a 2 second ttl",
            ttl=2,
        ),
        other_types_handler("DataExample", "data.example.", "example.", ttl=2),
        a_handler("AOnlyExample", "a-only.example.", ttl=2),
        other_types_handler("AOnlyExample", "a-only.example.", "example.", ttl=2),
        cname_a_handler("CnameExample", "cname.example.", "target.example.", ttl=7),
        other_types_handler("CnameExample", "cname.example.", "example.", ttl=2),
        a_handler("TargetExample", "target.example.", ttl=9),
        other_types_handler("TargetExample", "target.example.", "example.", ttl=2),
        cname_handler(
            "ShortTtlCnameExample",
            "shortttl.cname.example.",
            "longttl.target.example.",
            ttl=1,
        ),
        a_handler("LongTtlTargetExample", "longttl.target.example.", ttl=600),
        other_types_handler(
            "LongTtlTargetExample", "longttl.target.example.", "example.", ttl=2
        ),
        txt_handler(
            "LongTtlExample",
            "longttl.example.",
            "A text record with a 600 second ttl",
            ttl=600,
        ),
        other_types_handler("LongTtlExample", "longttl.example.", "example.", ttl=2),
        NxdomainExampleHandler(),
        OthertypeExampleCaaHandler(),
        other_types_handler(
            "OthertypeExample", "othertype.example.", "example.", ttl=2
        ),
        a_handler("NsDelegatedServeStale", "ns.delegated.serve.stale."),
        other_types_handler(
            "NsDelegatedServeStale",
            "ns.delegated.serve.stale.",
            "delegated.serve.stale.",
        ),
        ns_handler(
            "DelegatedServeStaleZone",
            "delegated.serve.stale.",
            "ns.delegated.serve.stale.",
        ),
        soa_handler("DelegatedServeStaleZone", "delegated.serve.stale."),
        other_types_handler(
            "DelegatedServeStaleZone",
            "delegated.serve.stale.",
            "delegated.serve.stale.",
        ),
        a_handler(
            "WwwDelegatedServeStale",
            "www.delegated.serve.stale.",
            ttl=2,
            address="10.53.0.99",
        ),
        other_types_handler(
            "WwwDelegatedServeStale",
            "www.delegated.serve.stale.",
            "delegated.serve.stale.",
            ttl=2,
        ),
        cname_a_handler(
            "CnameDelegatedServeStale",
            "cname.delegated.serve.stale.",
            "cname-target.serve.stale.",
            ttl=2,
        ),
        other_types_handler(
            "CnameDelegatedServeStale",
            "cname.delegated.serve.stale.",
            "delegated.serve.stale.",
            ttl=2,
        ),
        soa_handler("SourceStale", "source.stale."),
        ns_handler("SourceStale", "source.stale.", "ns.source.stale.", in_answer=True),
        SourceStaleFallbackHandler(),
        a_handler("NsSourceStale", "ns.source.stale."),
        other_types_handler("NsSourceStale", "ns.source.stale.", "source.stale."),
        cname_handler(
            "AliasSourceStale",
            "alias.source.stale.",
            "www.target.stale.",
            ttl=2,
        ),
        cname_handler(
            "AliasNxSourceStale",
            "aliasnx.source.stale.",
            "nonexist.target.stale.",
            ttl=2,
        ),
        fallback_handler("example."),
    ]


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR
    )
    server.install_response_handlers(*handlers())
    server.install_control_command(ToggleResponsesCommand())
    server.run()


if __name__ == "__main__":
    main()
