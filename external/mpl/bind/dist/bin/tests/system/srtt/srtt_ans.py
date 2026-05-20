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

import abc

import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import DnsResponseSend, QnameQtypeHandler, QueryContext


class DelayedQnameRangeHandler(QnameQtypeHandler):
    """
    Respond to queries for QNAMEs "foo1.example." through "foo<N>.example."
    with QTYPE=A, where <N> must be defined by the subclass.  Every response is
    delayed by a fixed amount of time, which must also be defined (in seconds)
    by the subclass.
    """

    @property
    def qnames(self) -> list[str]:
        return [f"foo{x}.example." for x in range(1, self.max_qname + 1)]

    qtypes = [dns.rdatatype.A]

    @property
    @abc.abstractmethod
    def max_qname(self) -> int:
        raise NotImplementedError

    @property
    @abc.abstractmethod
    def delay(self) -> float:
        raise NotImplementedError

    def __str__(self) -> str:
        return f"{self.__class__.__name__}(foo[1-{self.max_qname}].example/A)"

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        a_rrset = dns.rrset.from_text(
            qctx.qname, 300, dns.rdataclass.IN, dns.rdatatype.A, "10.53.9.9"
        )
        qctx.response.answer.append(a_rrset)
        yield DnsResponseSend(qctx.response, delay=self.delay)
