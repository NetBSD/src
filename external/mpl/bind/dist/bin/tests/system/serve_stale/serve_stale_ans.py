"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import Callable, Sequence

import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import QueryContext, ResponseHandler, StaticResponseHandler


def rrset(
    owner: str,
    ttl: int,
    rdtype: dns.rdatatype.RdataType,
    rdata: str,
) -> dns.rrset.RRset:
    return dns.rrset.from_text(owner, ttl, dns.rdataclass.IN, rdtype, rdata)


def soa(owner: str, ttl: int = 300, minimum: int = 300) -> dns.rrset.RRset:
    return rrset(owner, ttl, dns.rdatatype.SOA, f". . 0 0 0 0 {minimum}")


def ns(owner: str, target: str, ttl: int = 300) -> dns.rrset.RRset:
    return rrset(owner, ttl, dns.rdatatype.NS, target)


def a(owner: str, address: str, ttl: int = 300) -> dns.rrset.RRset:
    return rrset(owner, ttl, dns.rdatatype.A, address)


def txt(owner: str, data: str, ttl: int = 2) -> dns.rrset.RRset:
    return rrset(owner, ttl, dns.rdatatype.TXT, f'"{data}"')


def cname(owner: str, target: str, ttl: int) -> dns.rrset.RRset:
    return rrset(owner, ttl, dns.rdatatype.CNAME, target)


# The delay before sending a response may either be fixed or determined at
# response time by a callable.
Delay = float | Callable[[], float]


class StaticHandler(StaticResponseHandler):
    """
    A response handler answering with statically configured content.

    Matches queries by QNAME (any QNAME when `qnames` is None) and optionally
    by QTYPE, and responds with the RRsets/RCODE/delay given to the
    constructor.  `name` is only used for logging.
    """

    def __init__(
        self,
        name: str,
        qnames: str | list[str] | None,
        qtypes: list[dns.rdatatype.RdataType] | None = None,
        *,
        answer: Sequence[dns.rrset.RRset] = (),
        authority: Sequence[dns.rrset.RRset] = (),
        additional: Sequence[dns.rrset.RRset] = (),
        rcode: dns.rcode.Rcode | None = None,
        delay: Delay = 0.0,
    ) -> None:
        if isinstance(qnames, str):
            qnames = [qnames]
        self._name = name
        self._qnames = (
            None if qnames is None else [dns.name.from_text(q) for q in qnames]
        )
        self._qtypes = qtypes
        self._answer = list(answer)
        self._authority = list(authority)
        self._additional = list(additional)
        self._rcode = rcode
        self._delay = delay

    def match(self, qctx: QueryContext) -> bool:
        if self._qnames is not None and qctx.qname not in self._qnames:
            return False
        if self._qtypes is not None and qctx.qtype not in self._qtypes:
            return False
        return True

    @property
    def answer(self) -> Sequence[dns.rrset.RRset]:
        return self._answer

    @property
    def authority(self) -> Sequence[dns.rrset.RRset]:
        return self._authority

    @property
    def additional(self) -> Sequence[dns.rrset.RRset]:
        return self._additional

    @property
    def rcode(self) -> dns.rcode.Rcode | None:
        return self._rcode

    @property
    def delay(self) -> float:
        if callable(self._delay):
            return self._delay()
        return self._delay

    def __str__(self) -> str:
        if self._qnames is None:
            return self._name
        qnames = ", ".join(n.to_text() for n in self._qnames)
        return f"{self._name}(QNAMEs: {qnames})"


def a_handler(
    name_prefix: str,
    owner: str,
    ttl: int = 300,
    address: str = "10.53.0.2",
) -> ResponseHandler:
    return StaticHandler(
        f"{name_prefix}AHandler",
        owner,
        [dns.rdatatype.A],
        answer=[a(owner, ttl=ttl, address=address)],
    )


def ns_handler(
    name_prefix: str,
    owner: str,
    target: str,
    ttl: int = 300,
    glue_ttl: int = 300,
    address: str = "10.53.0.2",
    in_answer: bool = False,
) -> ResponseHandler:
    ns_rrsets = [ns(owner, target, ttl=ttl)]
    return StaticHandler(
        f"{name_prefix}NsHandler",
        owner,
        [dns.rdatatype.NS],
        answer=ns_rrsets if in_answer else (),
        authority=() if in_answer else ns_rrsets,
        additional=[a(target, address=address, ttl=glue_ttl)],
    )


def soa_handler(name_prefix: str, owner: str, ttl: int = 300) -> ResponseHandler:
    return StaticHandler(
        f"{name_prefix}SoaHandler",
        owner,
        [dns.rdatatype.SOA],
        answer=[soa(owner, ttl=ttl)],
    )


def other_types_handler(
    name_prefix: str,
    qname_or_qnames: str | list[str],
    zone: str,
    ttl: int = 300,
) -> ResponseHandler:
    return StaticHandler(
        f"{name_prefix}OtherTypesHandler",
        qname_or_qnames,
        authority=[soa(zone, ttl=ttl)],
    )


def cname_handler(
    name_prefix: str, owner: str, target: str, ttl: int = 300
) -> ResponseHandler:
    return StaticHandler(
        f"{name_prefix}CnameHandler",
        owner,
        answer=[cname(owner, target, ttl=ttl)],
    )


def cname_a_handler(
    name_prefix: str, owner: str, target: str, ttl: int = 300
) -> ResponseHandler:
    return StaticHandler(
        f"{name_prefix}CnameAHandler",
        owner,
        [dns.rdatatype.A],
        answer=[cname(owner, target, ttl=ttl)],
    )


def txt_handler(
    name_prefix: str, owner: str, text: str, ttl: int = 300
) -> ResponseHandler:
    return StaticHandler(
        f"{name_prefix}TxtHandler",
        owner,
        [dns.rdatatype.TXT],
        answer=[txt(owner, text, ttl=ttl)],
    )


def fallback_handler(zone: str, ttl: int = 300) -> ResponseHandler:
    return StaticHandler(
        "FallbackHandler",
        None,
        rcode=dns.rcode.NXDOMAIN,
        authority=[soa(zone, ttl=ttl)],
    )
