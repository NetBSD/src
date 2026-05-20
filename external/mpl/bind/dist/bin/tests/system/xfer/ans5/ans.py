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

from collections.abc import AsyncGenerator, Collection
from typing import final

import abc

import dns.message
import dns.name
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset
import dns.tsig

from isctest.asyncserver import (
    AxfrHandler,
    ControllableAsyncDnsServer,
    DnsProtocol,
    DnsResponseSend,
    QueryContext,
    ResponseAction,
    ResponseHandler,
    SwitchControlCommand,
)
from isctest.vars.algorithms import ALG_VARS

GOOD_KEY_DATA = "LSAnCU+Z"
DEFAULT_KEY = dns.tsig.Key("tsig_key", GOOD_KEY_DATA, ALG_VARS["DEFAULT_HMAC"])
BAD_KEY = dns.tsig.Key("bad_key", GOOD_KEY_DATA, ALG_VARS["DEFAULT_HMAC"])
UNUSED_KEY = dns.tsig.Key("unused_key", GOOD_KEY_DATA, ALG_VARS["DEFAULT_HMAC"])
KEYRING = {key.name: key for key in (DEFAULT_KEY, BAD_KEY, UNUSED_KEY)}

KEY_WITH_BAD_DATA = dns.tsig.Key("tsig_key", "abcd1234ffff", ALG_VARS["DEFAULT_HMAC"])


class ResponseHandlerWrapper(ResponseHandler, abc.ABC):
    def __init__(self, inner: ResponseHandler) -> None:
        self._inner = inner

    def match(self, qctx: QueryContext) -> bool:
        return self._inner.match(qctx)

    def _on_query_received(self, qctx: QueryContext) -> None:
        pass

    @abc.abstractmethod
    def _modify_response(
        self, qctx: QueryContext, response_action: ResponseAction
    ) -> None:
        raise NotImplementedError

    @final
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        self._on_query_received(qctx)
        async for response_action in self._inner.get_responses(qctx):
            self._modify_response(qctx, response_action)
            yield response_action

    def __str__(self) -> str:
        return f"{self.__class__.__name__}({self._inner})"


class SignResponses(ResponseHandlerWrapper):
    """
    This handler encapsulates another handler and signs all responses it yields
    with TSIG using the specified key.

    If the query is over TCP, it maintains the TSIG context across multiple
    messages to allow proper signing of multi-message responses.

    Ideally, TSIG context would be handled in isctest.asyncserver, but that would
    require more extensive changes there, so it is implemented here for a single
    test.
    """

    def __init__(self, inner: ResponseHandler, key: dns.tsig.Key = DEFAULT_KEY) -> None:
        super().__init__(inner)
        self._key = key
        self._tsig_ctx: dns.tsig.GSSTSig | dns.tsig.HMACTSig | None = None

    def _on_query_received(self, qctx: QueryContext) -> None:
        self._tsig_ctx = None

    def _apply_tsig_context(self, response: dns.message.Message) -> None:
        # On TCP we need to maintain the TSIG context across multiple messages.
        # Force TSIG materialization to get the updated context by calling to_wire().
        _ = response.to_wire(multi=True, tsig_ctx=self._tsig_ctx)
        # Cache TSIG context for the next message.
        self._tsig_ctx = response.tsig_ctx

    def _modify_response(
        self, qctx: QueryContext, response_action: ResponseAction
    ) -> None:
        assert isinstance(
            response_action, DnsResponseSend
        ), "SignResponses can only wrap handlers that yield DnsResponseSend"
        response_action.response.use_tsig(self._key)
        if qctx.protocol == DnsProtocol.TCP:
            self._apply_tsig_context(response_action.response)

    def __str__(self) -> str:
        return f"SignResponses({self._inner}, key={self._key})"


class SignFirstResponse(ResponseHandlerWrapper):
    def __init__(self, inner: ResponseHandler, key: dns.tsig.Key = DEFAULT_KEY) -> None:
        super().__init__(inner)
        self._key = key
        self._first_yielded = False

    def _on_query_received(self, qctx: QueryContext) -> None:
        self._first_yielded = False

    def _modify_response(
        self, qctx: QueryContext, response_action: ResponseAction
    ) -> None:
        assert isinstance(
            response_action, DnsResponseSend
        ), "SignFirstResponse can only wrap handlers that yield DnsResponseSend"
        if not self._first_yielded:
            response_action.response.use_tsig(self._key)
            self._first_yielded = True
        else:
            response_action.response.tsig = None

    def __str__(self) -> str:
        return f"SignFirstResponse({self._inner}, key={self._key})"


class Add50ToMessageIdFromSecondResponse(ResponseHandlerWrapper):
    def __init__(self, inner: ResponseHandler) -> None:
        super().__init__(inner)
        self._first_yielded = False

    def _on_query_received(self, qctx: QueryContext) -> None:
        self._first_yielded = False

    def _modify_response(
        self, qctx: QueryContext, response_action: ResponseAction
    ) -> None:
        if self._first_yielded:
            assert isinstance(
                response_action, DnsResponseSend
            ), "Add50ToMessageIdFromSecondResponse can only wrap handlers that yield DnsResponseSend from the second response onward"
            response_action.response.id += 50
        else:
            self._first_yielded = True


class ClearTsig(ResponseHandlerWrapper):
    def _modify_response(
        self, qctx: QueryContext, response_action: ResponseAction
    ) -> None:
        assert isinstance(
            response_action, DnsResponseSend
        ), "ClearTsig can only wrap handlers that yield DnsResponseSend"
        response_action.response.tsig = None


def rrset(
    owner: str | dns.name.Name,
    ttl: int,
    rdtype: dns.rdatatype.RdataType,
    rdata: str,
) -> dns.rrset.RRset:
    return dns.rrset.from_text(
        owner,
        ttl,
        dns.rdataclass.IN,
        rdtype,
        rdata,
    )


def soa(
    serial: int,
    *,
    owner: str = "nil.",
    mname: str = "ns.nil.",
    rname: str = "root.nil.",
) -> dns.rrset.RRset:
    return rrset(
        owner,
        300,
        dns.rdatatype.SOA,
        f"{mname} {rname} {serial} 300 300 604800 300",
    )


class SoaHandler(ResponseHandler):
    def __init__(self, serial: int = 1) -> None:
        self._serial = serial

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.SOA

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.answer.append(soa(self._serial))
        yield DnsResponseSend(qctx.response)


def ns() -> dns.rrset.RRset:
    return rrset(
        "nil.",
        300,
        dns.rdatatype.NS,
        "ns.nil.",
    )


def txt(data: str) -> dns.rrset.RRset:
    return rrset(
        "nil.",
        300,
        dns.rdatatype.TXT,
        f'"{data}"',
    )


def a() -> dns.rrset.RRset:
    return rrset(
        "a.nil.",
        60,
        dns.rdatatype.A,
        "10.0.0.61",
    )


def extra_a() -> dns.rrset.RRset:
    return rrset(
        "b.nil.",
        60,
        dns.rdatatype.A,
        "10.0.0.62",
    )


class XferAxfrHandler(AxfrHandler):
    def __init__(
        self,
        *,
        txt_data: str,
        soa_serial: int = 1,
        extra_a_record: bool = False,
        final_soa_mismatch: bool = False,
    ) -> None:
        self._txt_data = txt_data
        self._soa_serial = soa_serial
        self._extra_a_record = extra_a_record
        self._final_soa_mismatch = final_soa_mismatch

    @property
    def initial_soa(self) -> dns.rrset.RRset:
        return soa(self._soa_serial)

    @property
    def zone_contents(self) -> Collection[dns.rrset.RRset]:
        records = [ns(), txt(self._txt_data), a()]
        if self._extra_a_record:
            records.append(extra_a())
        return records

    @property
    def final_soa(self) -> dns.rrset.RRset:
        if self._final_soa_mismatch:
            return soa(self._soa_serial, mname="whatever.", rname="other.")
        return soa(self._soa_serial)


class WrongQnameInFinalSoa(ResponseHandlerWrapper):
    def __init__(self, inner: XferAxfrHandler) -> None:
        super().__init__(inner)
        self._messages_until_final_soa = 2

    def _modify_response(
        self, qctx: QueryContext, response_action: ResponseAction
    ) -> None:
        if self._messages_until_final_soa == 0:
            assert isinstance(
                response_action, DnsResponseSend
            ), "WrongQnameInFinalSoaAxfrHandler can only wrap handlers that yield DnsResponseSend from the final SOA response"
            response_action.response.question[0].name = dns.name.from_text("ns.wrong.")
        self._messages_until_final_soa -= 1


class IxfrNotimpHandler(ResponseHandler):
    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.IXFR

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.set_rcode(dns.rcode.NOTIMP)
        yield DnsResponseSend(qctx.response)


class AxfrEdnsRcodeHandler(ResponseHandler):
    def __init__(self, rcode: dns.rcode.Rcode) -> None:
        self._rcode = rcode

    def match(self, qctx: QueryContext) -> bool:
        return qctx.qtype == dns.rdatatype.AXFR and qctx.query.edns > -1

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.response.set_rcode(self._rcode)
        yield DnsResponseSend(qctx.response)


def main() -> None:
    server = ControllableAsyncDnsServer(
        default_aa=True, default_rcode=dns.rcode.NOERROR, keyring=KEYRING
    )
    switch_command = SwitchControlCommand(
        {
            "badkeydata": (
                SignResponses(SoaHandler(serial := 3)),
                SignResponses(
                    XferAxfrHandler(soa_serial=serial, txt_data="bad keydata AXFR"),
                    KEY_WITH_BAD_DATA,
                ),
            ),
            "badmessageid": (
                SignResponses(SoaHandler()),
                SignResponses(
                    Add50ToMessageIdFromSecondResponse(
                        XferAxfrHandler(txt_data="bad message id")
                    ),
                ),
            ),
            "ednsformerr": (
                SignResponses(SoaHandler()),
                SignResponses(AxfrEdnsRcodeHandler(rcode=dns.rcode.FORMERR)),
                SignResponses(XferAxfrHandler(txt_data="EDNS FORMERR")),
            ),
            "ednsnotimp": (
                SignResponses(SoaHandler()),
                SignResponses(AxfrEdnsRcodeHandler(rcode=dns.rcode.NOTIMP)),
                SignResponses(XferAxfrHandler(txt_data="EDNS NOTIMP")),
            ),
            "goodaxfr": (
                SignResponses(SoaHandler()),
                SignResponses(XferAxfrHandler(txt_data="initial AXFR")),
            ),
            "ixfrnotimp": (
                SignResponses(SoaHandler(serial := 2)),
                SignResponses(IxfrNotimpHandler()),
                SignResponses(
                    XferAxfrHandler(soa_serial=serial, txt_data="IXFR NOTIMP")
                ),
            ),
            "partial": (
                SignResponses(SoaHandler(serial := 4)),
                SignFirstResponse(
                    XferAxfrHandler(
                        soa_serial=serial,
                        txt_data="partially signed AXFR",
                        extra_a_record=True,
                    ),
                ),
            ),
            "soamismatch": (
                SignResponses(SoaHandler()),
                SignResponses(
                    XferAxfrHandler(
                        txt_data="SOA mismatch AXFR",
                        final_soa_mismatch=True,
                    )
                ),
            ),
            "unknownkey": (
                SignResponses(SoaHandler(serial := 5), BAD_KEY),
                SignResponses(
                    XferAxfrHandler(
                        soa_serial=serial,
                        txt_data="unknown key AXFR",
                        extra_a_record=True,
                    ),
                    BAD_KEY,
                ),
            ),
            "unsigned": (
                SignResponses(SoaHandler(serial := 2)),
                ClearTsig(
                    XferAxfrHandler(
                        soa_serial=serial,
                        txt_data="unsigned AXFR",
                        extra_a_record=True,
                    )
                ),
            ),
            "wrongkey": (
                SignResponses(SoaHandler(serial := 6), UNUSED_KEY),
                SignResponses(
                    XferAxfrHandler(
                        soa_serial=serial,
                        txt_data="incorrect key AXFR",
                        extra_a_record=True,
                    ),
                    UNUSED_KEY,
                ),
            ),
            "wrongname": (
                SignResponses(SoaHandler()),
                SignResponses(
                    WrongQnameInFinalSoa(
                        XferAxfrHandler(txt_data="wrong question AXFR")
                    )
                ),
            ),
        }
    )
    server.install_control_command(switch_command)
    server.run()


if __name__ == "__main__":
    main()
