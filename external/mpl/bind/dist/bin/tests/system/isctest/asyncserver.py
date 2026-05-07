"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from collections.abc import AsyncGenerator, Callable, Coroutine, Sequence
from dataclasses import dataclass, field
from typing import Any, cast

import abc
import asyncio
import contextlib
import copy
import enum
import functools
import logging
import os
import pathlib
import re
import signal
import struct
import sys

import dns.exception
import dns.flags
import dns.message
import dns.name
import dns.node
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdataset
import dns.rdatatype
import dns.rrset
import dns.tsig
import dns.zone

_UdpHandler = Callable[
    [bytes, tuple[str, int], asyncio.DatagramTransport], Coroutine[Any, Any, None]
]


_TcpHandler = Callable[
    [asyncio.StreamReader, asyncio.StreamWriter], Coroutine[Any, Any, None]
]


class _AsyncUdpHandler(asyncio.DatagramProtocol):
    """
    Protocol implementation for handling UDP traffic using asyncio.
    """

    def __init__(
        self,
        handler: _UdpHandler,
    ) -> None:
        self._transport: asyncio.DatagramTransport | None = None
        self._handler: _UdpHandler = handler

    def connection_made(self, transport: asyncio.BaseTransport) -> None:
        """
        Called by asyncio when a connection is made.
        """
        self._transport = cast(asyncio.DatagramTransport, transport)

    def datagram_received(self, data: bytes, addr: tuple[str, int]) -> None:
        """
        Called by asyncio when a datagram is received.
        """
        assert self._transport
        handler_coroutine = self._handler(data, addr, self._transport)
        try:
            # Python >= 3.7
            asyncio.create_task(handler_coroutine)
        except AttributeError:
            # Python < 3.7
            loop = asyncio.get_event_loop()
            loop.create_task(handler_coroutine)


class AsyncServer:
    """
    A generic asynchronous server which may handle UDP and/or TCP traffic.

    Once the server is executed as asyncio coroutine, it will keep running
    until a SIGINT/SIGTERM signal is received.
    """

    def __init__(
        self,
        udp_handler: _UdpHandler | None,
        tcp_handler: _TcpHandler | None,
        pidfile: str | None = None,
    ) -> None:
        logging.basicConfig(
            format="%(asctime)s %(levelname)8s  %(message)s",
            level=os.environ.get("ANS_LOG_LEVEL", "INFO").upper(),
        )
        try:
            ipv4_address = sys.argv[1]
        except IndexError:
            ipv4_address = self._get_ipv4_address_from_directory_name()

        last_ipv4_address_octet = ipv4_address.split(".")[-1]
        ipv6_address = f"fd92:7065:b8e:ffff::{last_ipv4_address_octet}"

        try:
            port = int(sys.argv[2])
        except IndexError:
            port = int(os.environ.get("PORT", 5300))

        logging.info("Setting up IPv4 listener at %s:%d", ipv4_address, port)
        logging.info("Setting up IPv6 listener at [%s]:%d", ipv6_address, port)

        self._ip_addresses: tuple[str, str] = (ipv4_address, ipv6_address)
        self._port: int = port
        self._udp_handler: _UdpHandler | None = udp_handler
        self._tcp_handler: _TcpHandler | None = tcp_handler
        self._pidfile: str | None = pidfile
        self._work_done: asyncio.Future | None = None

    def _get_ipv4_address_from_directory_name(self) -> str:
        containing_directory = pathlib.Path().absolute().stem
        match_result = re.match(r"ans(?P<index>\d+)", containing_directory)
        if not match_result:
            raise RuntimeError("Unable to auto-determine the IPv4 address to use")

        return f"10.53.0.{match_result.group('index')}"

    def run(self) -> None:
        """
        Start the server in an asynchronous coroutine.
        """
        coroutine = self._run
        try:
            # Python >= 3.7
            asyncio.run(coroutine())
        except AttributeError:
            # Python < 3.7
            loop = asyncio.get_event_loop()
            loop.run_until_complete(coroutine())

    async def _run(self) -> None:
        self._setup_exception_handler()
        self._setup_signals()
        assert self._work_done
        await self._listen_udp()
        await self._listen_tcp()
        self._write_pidfile()
        await self._work_done
        self._cleanup_pidfile()

    def _get_asyncio_loop(self) -> asyncio.AbstractEventLoop:
        try:
            # Python >= 3.7
            loop = asyncio.get_running_loop()
        except AttributeError:
            # Python < 3.7
            loop = asyncio.get_event_loop()
        return loop

    def _setup_exception_handler(self) -> None:
        loop = self._get_asyncio_loop()
        self._work_done = loop.create_future()
        loop.set_exception_handler(self._handle_exception)

    def _handle_exception(
        self, _: asyncio.AbstractEventLoop, context: dict[str, Any]
    ) -> None:
        assert self._work_done
        exception = context.get("exception", RuntimeError(context["message"]))
        try:
            self._work_done.set_exception(exception)
        except asyncio.InvalidStateError:
            pass

    def _setup_signals(self) -> None:
        loop = self._get_asyncio_loop()
        loop.add_signal_handler(signal.SIGINT, functools.partial(self._signal_done))
        loop.add_signal_handler(signal.SIGTERM, functools.partial(self._signal_done))

    def _signal_done(self) -> None:
        assert self._work_done
        try:
            self._work_done.set_result(True)
        except asyncio.InvalidStateError:
            pass

    async def _listen_udp(self) -> None:
        if not self._udp_handler:
            return
        loop = self._get_asyncio_loop()
        for ip_address in self._ip_addresses:
            await loop.create_datagram_endpoint(
                lambda: _AsyncUdpHandler(cast(_UdpHandler, self._udp_handler)),
                (ip_address, self._port),
            )

    async def _listen_tcp(self) -> None:
        if not self._tcp_handler:
            return
        for ip_address in self._ip_addresses:
            await asyncio.start_server(
                self._tcp_handler, host=ip_address, port=self._port
            )

    def _write_pidfile(self) -> None:
        if not self._pidfile:
            return
        logging.info("Writing PID to %s", self._pidfile)
        with open(self._pidfile, "w", encoding="ascii") as pidfile:
            print(f"{os.getpid()}", file=pidfile)

    def _cleanup_pidfile(self) -> None:
        if not self._pidfile:
            return
        logging.info("Removing %s", self._pidfile)
        os.unlink(self._pidfile)


class DnsProtocol(enum.Enum):
    UDP = enum.auto()
    TCP = enum.auto()


@dataclass(frozen=True)
class Peer:
    """
    Pretty-printed connection endpoint.
    """

    host: str
    port: int

    def __str__(self) -> str:
        host = f"[{self.host}]" if ":" in self.host else self.host
        return f"{host}:{self.port}"


@dataclass
class QueryContext:
    """
    Context for the incoming query which may be used for preparing the response.
    """

    query: dns.message.Message
    response: dns.message.Message
    socket: Peer
    peer: Peer
    protocol: DnsProtocol
    zone: dns.zone.Zone | None = field(default=None, init=False)
    soa: dns.rrset.RRset | None = field(default=None, init=False)
    node: dns.node.Node | None = field(default=None, init=False)
    answer: dns.rdataset.Rdataset | None = field(default=None, init=False)
    alias: dns.name.Name | None = field(default=None, init=False)
    _initialized_response: dns.message.Message | None = field(default=None, init=False)
    _initialized_response_with_zone_data: dns.message.Message | None = field(
        default=None, init=False
    )

    @property
    def qname(self) -> dns.name.Name:
        return self.query.question[0].name

    @property
    def current_qname(self) -> dns.name.Name:
        return self.alias or self.qname

    @property
    def qclass(self) -> dns.rdataclass.RdataClass:
        return self.query.question[0].rdclass

    @property
    def qtype(self) -> dns.rdatatype.RdataType:
        return self.query.question[0].rdtype

    def prepare_new_response(
        self, /, with_zone_data: bool = True
    ) -> dns.message.Message:
        if with_zone_data:
            assert self._initialized_response_with_zone_data
            self.response = copy.deepcopy(self._initialized_response_with_zone_data)
        else:
            assert self._initialized_response
            self.response = copy.deepcopy(self._initialized_response)
        return self.response

    def save_initialized_response(self, /, with_zone_data: bool) -> None:
        if with_zone_data:
            self._initialized_response_with_zone_data = copy.deepcopy(self.response)
        else:
            self._initialized_response = copy.deepcopy(self.response)


@dataclass
class ResponseAction(abc.ABC):
    """
    Base class for actions that can be taken in response to a query.
    """

    @abc.abstractmethod
    async def perform(self) -> dns.message.Message | bytes | None:
        """
        This method is expected to carry out arbitrary actions (e.g. wait for a
        specific amount of time, modify the answer, etc.) and then return the
        DNS response to send (a dns.message.Message, a raw bytes object, or
        None, which prevents any response from being sent).
        """
        raise NotImplementedError


@dataclass
class DnsResponseSend(ResponseAction):
    """
    Action which yields a dns.message.Message response.

    The response may be sent with a delay if requested.

    Depending on the value of the `authoritative` property, this class may set
    the AA bit in the response (True), clear it (False), or not touch it at all
    (None).
    """

    response: dns.message.Message
    authoritative: bool | None = None
    delay: float = 0.0
    acknowledge_hand_rolled_response: bool = False

    async def perform(self) -> dns.message.Message | bytes | None:
        """
        Yield a potentially delayed response that is a dns.message.Message.
        """
        assert isinstance(self.response, dns.message.Message)
        if not (
            _is_asyncserver_response(self.response)
            or self.acknowledge_hand_rolled_response
        ):
            error = "The response you are trying to send was not created using "
            error += "AsyncDnsServer's response preparation methods. "
            error += "This will break features such as automatic AA flag "
            error += "and RCODE handling. If you need a fresh copy of a "
            error += "response, use `QueryContext.prepare_new_response` "
            error += "instead of `dns.message.make_response`. "
            error += "To acknowledge this and proceed anyway, set "
            error += "`acknowledge_hand_rolled_response=True` in "
            error += "DnsResponseSend's constructor."
            raise RuntimeError(error)

        if self.authoritative is not None:
            if self.authoritative:
                self.response.flags |= dns.flags.AA
            else:
                self.response.flags &= ~dns.flags.AA
        if self.delay > 0:
            logging.info(
                "Delaying response (ID=%d) by %d ms",
                self.response.id,
                self.delay * 1000,
            )
            await asyncio.sleep(self.delay)
        return self.response


@dataclass
class BytesResponseSend(ResponseAction):
    """
    Action which yields a raw response that is a sequence of bytes.

    The response may be sent with a delay if requested.
    """

    response: bytes
    delay: float = 0.0

    async def perform(self) -> dns.message.Message | bytes | None:
        """
        Yield a potentially delayed response that is a sequence of bytes.
        """
        assert isinstance(self.response, bytes)
        if self.delay > 0:
            logging.info("Delaying raw response by %d ms", self.delay * 1000)
            await asyncio.sleep(self.delay)
        return self.response


@dataclass
class ResponseDrop(ResponseAction):
    """
    Action which does nothing - as if a packet was dropped.
    """

    async def perform(self) -> dns.message.Message | bytes | None:
        return None


class _ConnectionTeardownRequested(Exception):
    pass


@dataclass
class CloseConnection(ResponseAction):
    """
    Action which makes the server close the connection (TCP only).

    The connection may be closed with a delay if requested.
    """

    delay: float = 0.0

    async def perform(self) -> dns.message.Message | bytes | None:
        if self.delay > 0:
            logging.info("Waiting %.1fs before closing TCP connection", self.delay)
            await asyncio.sleep(self.delay)
        raise _ConnectionTeardownRequested


class ConnectionHandler(abc.ABC):
    """
    Base class for TCP connection handlers.

    An installed connection handler is called when a new TCP connection is
    established.  It may be used to perform arbitrary actions before
    AsyncDnsServer processes DNS queries.
    """

    @abc.abstractmethod
    async def handle(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, peer: Peer
    ) -> None:
        """
        Handle the connection with the provided reader and writer.
        """
        raise NotImplementedError


def block_reading(peer: Peer, writer_not_the_reader: asyncio.StreamWriter) -> None:
    """
    Block reads for the reader associated with the provided writer.

    Yes, pass the writer, not the reader. See the comments below for details.
    """

    try:
        # Python >= 3.7
        loop = asyncio.get_running_loop()
    except AttributeError:
        # Python < 3.7
        loop = asyncio.get_event_loop()

    logging.info("Blocking reads from %s", peer)

    # This is Michał's submission for the Ugliest Hack of the Year contest.
    # (The alternative was implementing an asyncio transport from scratch.)
    #
    # In order to prevent the client socket from being read from, simply
    # not calling `reader.read()` is not enough, because asyncio buffers
    # incoming data itself on the transport level.  However, `StreamReader`
    # does not expose the underlying transport as a property.  Therefore,
    # cheat by extracting it from `StreamWriter` as it is the same
    # bidirectional transport as for the read side (a `Transport`, which is
    # a subclass of both `ReadTransport` and `WriteTransport`) and call
    # `ReadTransport.pause_reading()` to remove the underlying socket from
    # the set of descriptors monitored by the selector, thereby preventing
    # any reads from happening on the client socket.  However...
    loop.call_soon(writer_not_the_reader.transport.pause_reading)  # type: ignore

    # ...due to `AsyncDnsServer._handle_tcp()` being a coroutine, by the
    # time it gets executed, asyncio transport code will already have added
    # the client socket to the set of descriptors monitored by the
    # selector.  Therefore, if the client starts sending data immediately,
    # a read from the socket will have already been scheduled by the time
    # this handler gets executed.  There is no way to prevent that from
    # happening, so work around it by abusing the fact that the transport
    # at hand is specifically an instance of `_SelectorSocketTransport`
    # (from asyncio.selector_events) and set the size of its read buffer to
    # just a single byte.  This does give asyncio enough time to read that
    # single byte from the client socket's buffer before that socket is
    # removed from the set of monitored descriptors, but prevents the
    # one-off read from emptying the client socket buffer _entirely_, which
    # is enough to trigger sending an RST segment when the connection is
    # closed shortly afterwards.
    writer_not_the_reader.transport.max_size = 1  # type: ignore


@dataclass
class IgnoreAllConnections(ConnectionHandler):
    """
    A connection handler that makes the server not read anything from the
    client socket, effectively ignoring all incoming connections.
    """

    _connections: set[asyncio.StreamWriter] = field(default_factory=set)

    async def handle(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, peer: Peer
    ) -> None:
        block_reading(peer, writer)
        # Due to the way various asyncio-related objects (tasks, streams,
        # transports, selectors) are referencing each other, pausing reads for
        # a TCP transport (which in practice means removing the client socket
        # from the set of descriptors monitored by a selector) can cause the
        # client task (AsyncDnsServer._handle_tcp()) to be prematurely
        # garbage-collected, causing asyncio code to raise a "Task was
        # destroyed but it is pending!" exception.  Prevent that from happening
        # by keeping a reference to each incoming TCP connection to protect its
        # related asyncio objects from getting garbage-collected.  This
        # prevents AsyncDnsServer from closing any of the ignored TCP
        # connections indefinitely, which is obviously a pretty brain-dead idea
        # for a production-grade DNS server, but AsyncDnsServer was never meant
        # to be one and this hack reliably solves the problem at hand.
        self._connections.add(writer)


@dataclass
class ConnectionReset(ConnectionHandler):
    """
    A connection handler that makes the server close the connection without
    reading anything from the client socket.

    The connection may be closed with a delay if requested.

    The sole purpose of this handler is to trigger a connection reset, i.e. to
    make the server send an RST segment; this happens when the server closes a
    client's socket while there is still unread data in that socket's buffer.
    If closing the connection _after_ the query is read by the server is enough
    for a given use case, the CloseConnection response handler should be used
    instead.
    """

    delay: float = 0.0

    async def handle(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, peer: Peer
    ) -> None:
        block_reading(peer, writer)

        if self.delay > 0:
            logging.info(
                "Waiting %.1fs before closing TCP connection from %s", self.delay, peer
            )
            await asyncio.sleep(self.delay)

        raise _ConnectionTeardownRequested


class ResponseHandler(abc.ABC):
    """
    Base class for generic response handlers.

    If a query passes the `match()` function logic, then it is handled by this
    response handler and response(s) may be generated by the `get_responses()`
    method.
    """

    # pylint: disable=unused-argument
    def match(self, qctx: QueryContext) -> bool:
        """
        Matching logic - the first handler whose `match()` method returns True
        is used for handling the query.

        The default for each handler is to handle all queries.
        """
        return True

    @abc.abstractmethod
    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        """
        Custom handler which may produce response(s) to matching queries.

        The response prepared from zone data is passed to this method in
        qctx.response.
        """
        yield DnsResponseSend(qctx.response)

    def __str__(self) -> str:
        return self.__class__.__name__


class IgnoreAllQueries(ResponseHandler):
    """
    Do not respond to any queries sent to the server.
    """

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        yield ResponseDrop()


class QnameHandler(ResponseHandler):
    """
    Base class used for deriving custom QNAME handlers.

    The derived class must specify a list of `qnames` that it wants to handle.
    Queries for exactly these QNAMEs will then be passed to the
    `get_response()` method in the derived class.
    """

    @property
    @abc.abstractmethod
    def qnames(self) -> list[str]:
        """
        A list of QNAMEs handled by this class.
        """
        raise NotImplementedError

    def __init__(self) -> None:
        self._qnames: list[dns.name.Name] = [dns.name.from_text(d) for d in self.qnames]

    def __str__(self) -> str:
        return f"{self.__class__.__name__}(QNAMEs: {', '.join(self.qnames)})"

    def match(self, qctx: QueryContext) -> bool:
        """
        Handle queries whose QNAME matches any of the QNAMEs handled by this
        class.
        """
        return qctx.qname in self._qnames


class QnameQtypeHandler(QnameHandler):
    """
    Handle queries for which both of the following conditions are true:

    - the query's QNAME is present in `self.qnames`,
    - the query's QTYPE is present in `self.qtypes`.
    """

    @property
    @abc.abstractmethod
    def qtypes(self) -> list[dns.rdatatype.RdataType]:
        """
        A list of QTYPEs handled by this class.
        """
        raise NotImplementedError

    def __init__(self) -> None:
        super().__init__()
        self._qtypes: list[dns.rdatatype.RdataType] = self.qtypes

    def __str__(self) -> str:
        return f"{self.__class__.__name__}(QNAMEs: {', '.join(self.qnames)}; QTYPEs: {', '.join(map(str, self.qtypes))})"

    def match(self, qctx: QueryContext) -> bool:
        """
        Handle queries whose QNAME and QTYPE match any of the QNAMEs and
        QTYPEs handled by this class.
        """
        return qctx.qtype in self._qtypes and super().match(qctx)


class StaticResponseHandler(ResponseHandler):
    """
    Base class used for deriving custom static response handlers.

    The derived class can specify the RRsets to be included in the answer,
    authority, and additional sections of the response, whether to set the AA
    bit in the response, and a delay before sending the response.

    The default implementation of `get_responses()` uses these properties to
    prepare and yield a single response.
    """

    @property
    def rcode(self) -> dns.rcode.Rcode | None:
        """
        Optional RCODE to be set in the response.
        """
        return None

    @property
    def answer(self) -> Sequence[dns.rrset.RRset]:
        """
        RRsets to be included in the answer section of the response.
        """
        return []

    @property
    def authority(self) -> Sequence[dns.rrset.RRset]:
        """
        RRsets to be included in the authority section of the response.
        """
        return []

    @property
    def additional(self) -> Sequence[dns.rrset.RRset]:
        """
        RRsets to be included in the additional section of the response.
        """
        return []

    @property
    def authoritative(self) -> bool | None:
        """
        Whether to set the AA bit in the response.
        """
        return None

    @property
    def delay(self) -> float:
        """
        Delay before sending the response.
        """
        return 0.0

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[DnsResponseSend, None]:
        qctx.prepare_new_response(with_zone_data=False)
        qctx.response.answer.extend(self.answer)
        qctx.response.authority.extend(self.authority)
        qctx.response.additional.extend(self.additional)
        if self.rcode is not None:
            qctx.response.set_rcode(self.rcode)
        yield DnsResponseSend(
            qctx.response, authoritative=self.authoritative, delay=self.delay
        )


class DomainHandler(ResponseHandler):
    """
    Base class used for deriving custom domain handlers.

    The derived class must specify a list of `domains` that it wants to handle.
    Queries for any of these domains (and their subdomains) will then be passed
    to the `get_response()` method in the derived class.

    The most specific matching domain is stored in the `matched_domain` attribute.
    """

    @property
    @abc.abstractmethod
    def domains(self) -> list[str]:
        """
        A list of domain names handled by this class.
        """
        raise NotImplementedError

    def __init__(self) -> None:
        self._domains: list[dns.name.Name] = sorted(
            [dns.name.from_text(d) for d in self.domains], reverse=True
        )
        self._matched_domain: dns.name.Name | None = None

    @property
    def matched_domain(self) -> dns.name.Name:
        assert self._matched_domain is not None
        return self._matched_domain

    def __str__(self) -> str:
        return f"{self.__class__.__name__}(domains: {', '.join(self.domains)})"

    def match(self, qctx: QueryContext) -> bool:
        """
        Handle queries whose QNAME matches any of the domains handled by this
        class.
        """
        self._matched_domain = None
        for domain in self._domains:
            if qctx.qname.is_subdomain(domain):
                self._matched_domain = domain
                return True
        return False


class ForwarderHandler(ResponseHandler):
    """
    A handler forwarding all received queries to another DNS server with an
    optional delay and then relaying the responses back to the original client.

    Queries are currently always forwarded via UDP.
    """

    @property
    @abc.abstractmethod
    def target(self) -> str:
        """
        The address of the DNS server to forward queries to.
        """
        raise NotImplementedError

    @property
    def port(self) -> int:
        """
        The port of the DNS server to forward queries to.

        The default value of 0 causes the same port as the one used by this
        server for listening to be used.
        """
        return 0

    @property
    def delay(self) -> float:
        """
        The number of seconds to wait before forwarding each query.
        """
        return 0.0

    def __str__(self) -> str:
        return f"{self.__class__.__name__}(target: {self.target}:{self.port})"

    class ForwarderProtocol(asyncio.DatagramProtocol):
        def __init__(self, query: bytes, response: asyncio.Future) -> None:
            self._query = query
            self._response = response

        def connection_made(self, transport: asyncio.BaseTransport) -> None:
            logging.debug("[OUT] %s", self._query.hex())
            cast(asyncio.DatagramTransport, transport).sendto(self._query)

        def datagram_received(self, data: bytes, _: tuple[str, int]) -> None:
            logging.debug("[IN] %s", data.hex())
            self._response.set_result(data)

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        loop = asyncio.get_running_loop()
        response = loop.create_future()
        forwarding_target = f"{self.target}:{self.port or qctx.socket.port}"

        if self.delay > 0:
            logging.info(
                "Waiting %.1fs before forwarding %s query from %s to %s over UDP",
                self.delay,
                qctx.protocol.name,
                qctx.peer,
                forwarding_target,
            )
            await asyncio.sleep(self.delay)

        logging.info(
            "Forwarding %s query from %s to %s over UDP",
            qctx.protocol.name,
            qctx.peer,
            forwarding_target,
        )

        transport, _ = await loop.create_datagram_endpoint(
            lambda: self.ForwarderProtocol(qctx.query.to_wire(), response),
            local_addr=(qctx.socket.host, 0),
            remote_addr=(self.target, self.port or qctx.socket.port),
        )

        try:
            await response
        finally:
            transport.close()

        logging.info(
            "Relaying UDP response from %s to %s over %s",
            forwarding_target,
            qctx.peer,
            qctx.protocol.name,
        )

        try:
            message = _DnsMessageWithTsigDisabled.from_wire(response.result())
            yield DnsResponseSend(message, acknowledge_hand_rolled_response=True)
        except dns.exception.DNSException:
            logging.warning(
                "Failed to parse response from %s as a DNS message, relaying it as raw bytes",
                forwarding_target,
            )
            yield BytesResponseSend(response.result())


@dataclass
class _ZoneTreeNode:
    """
    A node representing a zone with one origin.
    """

    zone: dns.zone.Zone | None
    children: list["_ZoneTreeNode"] = field(default_factory=list)


class _ZoneTree:
    """
    Tree with independent zones.

    This zone tree is used as a backing structure for the DNS server. The
    individual zones are independent to allow the (single) server to serve both
    the parent zone and a child zone if needed.
    """

    def __init__(self) -> None:
        self._root: _ZoneTreeNode = _ZoneTreeNode(None)

    def add(self, zone: dns.zone.Zone) -> None:
        """
        Add a zone to the tree and rearrange sub-zones if necessary.
        """
        assert zone.origin
        best_match = self._find_best_match(zone.origin, self._root)
        added_node = _ZoneTreeNode(zone)
        self._move_children(best_match, added_node)
        best_match.children.append(added_node)

    def _find_best_match(
        self, name: dns.name.Name, start_node: _ZoneTreeNode
    ) -> _ZoneTreeNode:
        for child in start_node.children:
            assert child.zone
            assert child.zone.origin
            if name.is_subdomain(child.zone.origin):
                return self._find_best_match(name, child)
        return start_node

    def _move_children(self, node_from: _ZoneTreeNode, node_to: _ZoneTreeNode) -> None:
        assert node_to.zone
        assert node_to.zone.origin

        children_to_move = []
        for child in node_from.children:
            assert child.zone
            assert child.zone.origin
            if child.zone.origin.is_subdomain(node_to.zone.origin):
                children_to_move.append(child)

        for child in children_to_move:
            node_from.children.remove(child)
            node_to.children.append(child)

    def find_best_zone(self, name: dns.name.Name) -> dns.zone.Zone | None:
        """
        Return the closest matching zone (if any) for the domain name.
        """
        node = self._find_best_match(name, self._root)
        return node.zone if node != self._root else None


class _DnsMessageWithTsigDisabled(dns.message.Message):
    """
    A wrapper for `dns.message.Message` that works around a dnspython bug
    causing exceptions to be raised when `make_response()` or `to_wire()` are
    called for a message created using `dns.message.from_wire(keyring=False)`.

    See https://github.com/rthalley/dnspython/issues/1205 for more details.
    """

    class _DisableTsigHandling(contextlib.ContextDecorator):
        def __init__(self, message: dns.message.Message | None = None) -> None:
            self.original_tsig_sign = dns.tsig.sign
            self.original_tsig_validate = dns.tsig.validate
            if message:
                self.tsig = message.tsig

        def __enter__(self) -> None:
            """
            Override the `dns.tsig.sign` and `dns.tsig.validate` functions to prevent them
            from failing on messages initialized with `dns.message.from_wire(keyring=False)`.
            """

            def sign(*_: Any, **__: Any) -> tuple[dns.rdata.Rdata, None]:
                assert self.tsig
                return self.tsig[0], None

            def validate(*_: Any, **__: Any) -> None:
                return None

            dns.tsig.sign = sign
            dns.tsig.validate = validate

        def __exit__(self, *_: Any, **__: Any) -> None:
            dns.tsig.sign = self.original_tsig_sign
            dns.tsig.validate = self.original_tsig_validate

    @classmethod
    def from_wire(cls, wire: bytes) -> "_DnsMessageWithTsigDisabled":
        with cls._DisableTsigHandling():
            message = dns.message.from_wire(wire, keyring=False)
            message.__class__ = _DnsMessageWithTsigDisabled

        return cast(_DnsMessageWithTsigDisabled, message)

    @property
    def had_tsig(self) -> bool:
        """
        Override the `had_tsig()` method to always return False, to prevent
        `make_response()` from crashing.
        """
        return False

    def to_wire(self, *args: Any, **kwargs: Any) -> bytes:
        """
        Override the `to_wire()` method to prevent it from trying to sign
        the message with TSIG.
        """
        with self._DisableTsigHandling(self):
            return super().to_wire(*args, **kwargs)


class _NoKeyringType:
    pass


_ASYNCSERVER_RESPONSE_MARKER = "__is_asyncserver_response__"


def _make_asyncserver_response(query: dns.message.Message) -> dns.message.Message:
    response = dns.message.make_response(query)
    setattr(response, _ASYNCSERVER_RESPONSE_MARKER, True)
    return response


def _is_asyncserver_response(message: dns.message.Message) -> bool:
    return getattr(message, _ASYNCSERVER_RESPONSE_MARKER, False)


class AsyncDnsServer(AsyncServer):
    """
    DNS server which responds to queries based on zone data and/or custom
    handlers.

    The server may use custom handlers which allow arbitrary query processing.
    These don't need to be standards-compliant and can be used for testing all
    sorts of scenarios, including delaying responses, synthesizing them based
    on query contents etc.

    The server also loads any zone files (*.db) found in its directory and
    serves them. Responses prepared using zone data can then be modified,
    replaced, or suppressed by query handlers. Query handlers can also generate
    response from scratch, without using zone data at all.
    """

    def __init__(
        self,
        /,
        default_rcode: dns.rcode.Rcode = dns.rcode.REFUSED,
        default_aa: bool = False,
        keyring: (
            dict[dns.name.Name, dns.tsig.Key] | None | _NoKeyringType
        ) = _NoKeyringType(),
        acknowledge_manual_dname_handling: bool = False,
    ) -> None:
        super().__init__(self._handle_udp, self._handle_tcp, "ans.pid")

        self._zone_tree: _ZoneTree = _ZoneTree()
        self._connection_handler: ConnectionHandler | None = None
        self._response_handlers: list[ResponseHandler] = []
        self._default_rcode = default_rcode
        self._default_aa = default_aa
        self._keyring = keyring
        self._acknowledge_manual_dname_handling = acknowledge_manual_dname_handling

        self._load_zones()

    def install_response_handler(
        self, handler: ResponseHandler, prepend: bool = False
    ) -> None:
        """
        Add a response handler that will be used to handle matching queries.

        Response handlers can modify, replace, or suppress the answers prepared
        from zone file contents.

        The provided handler is installed at the end of the response handler
        list unless `prepend` is set to True, in which case it is installed at
        the beginning of the response handler list.
        """
        logging.info("Installing response handler: %s", handler)
        if prepend:
            self._response_handlers.insert(0, handler)
        else:
            self._response_handlers.append(handler)

    def install_response_handlers(self, *handlers: ResponseHandler) -> None:
        for handler in handlers:
            self.install_response_handler(handler)

    def replace_response_handlers(self, *new_handlers: ResponseHandler) -> None:
        """
        Uninstall all currently installed handlers and install the provided ones.
        """
        logging.info("Uninstalling response handlers: %s", str(self._response_handlers))
        self._response_handlers.clear()
        self.install_response_handlers(*new_handlers)

    def uninstall_response_handler(self, handler: ResponseHandler) -> None:
        """
        Remove the specified handler from the list of response handlers.
        """
        logging.info("Uninstalling response handler: %s", handler)
        self._response_handlers.remove(handler)

    def install_connection_handler(self, handler: ConnectionHandler) -> None:
        """
        Install a connection handler that will be called when a new TCP
        connection is established.
        """
        if self._connection_handler:
            raise RuntimeError("Only one connection handler can be installed")
        self._connection_handler = handler

    def _load_zones(self) -> None:
        for entry in os.scandir():
            entry_path = pathlib.Path(entry.path)
            if entry_path.suffix != ".db":
                continue
            zone = self._load_zone(entry_path)
            self._zone_tree.add(zone)

    def _load_zone(self, zone_file_path: pathlib.Path) -> dns.zone.Zone:
        logging.info("Loading zone file %s", zone_file_path)
        zone = self._load_zone_file(zone_file_path)
        self._abort_if_dname_found_unless_acknowledged(zone)
        return zone

    def _load_zone_file(self, zone_file_path: pathlib.Path) -> dns.zone.Zone:
        try:
            zone = self._load_zone_file_with_origin(zone_file_path)
        except dns.zone.UnknownOrigin:
            zone = self._load_zone_file_without_origin(zone_file_path)

        return zone

    def _load_zone_file_with_origin(
        self, zone_file_path: pathlib.Path
    ) -> dns.zone.Zone:
        zone = dns.zone.from_file(str(zone_file_path), origin=None, relativize=False)
        if zone.origin != dns.name.root:
            error = "only the root zone may use $ORIGIN in the zone file; "
            error += "for every other zone, its origin is determined by "
            error += "the name of the file it is loaded from"
            raise ValueError(error)
        return zone

    def _load_zone_file_without_origin(
        self, zone_file_path: pathlib.Path
    ) -> dns.zone.Zone:
        origin = dns.name.from_text(zone_file_path.stem)
        return dns.zone.from_file(str(zone_file_path), origin=origin, relativize=False)

    def _abort_if_dname_found_unless_acknowledged(self, zone: dns.zone.Zone) -> None:
        if self._acknowledge_manual_dname_handling:
            return

        error = f'DNAME records found in zone "{zone.origin}"; '
        error += "this server does not handle DNAME in a standards-compliant way; "
        error += "pass `acknowledge_manual_dname_handling=True` to the "
        error += "AsyncDnsServer constructor to acknowledge this and load zone anyway"

        for node in zone.nodes.values():
            for rdataset in node:
                if rdataset.rdtype == dns.rdatatype.DNAME:
                    raise ValueError(error)

    async def _handle_udp(
        self, wire: bytes, addr: tuple[str, int], transport: asyncio.DatagramTransport
    ) -> None:
        logging.debug("Received UDP message: %s", wire.hex())
        socket_info = transport.get_extra_info("sockname")
        socket = Peer(socket_info[0], socket_info[1])
        peer = Peer(addr[0], addr[1])
        responses = self._handle_query(wire, socket, peer, DnsProtocol.UDP)
        async for response in responses:
            logging.debug("Sending UDP message: %s", response.hex())
            transport.sendto(response, addr)

    async def _handle_tcp(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        peer_info = writer.get_extra_info("peername")
        peer = Peer(peer_info[0], peer_info[1])
        logging.debug("Accepted TCP connection from %s", peer)

        try:
            if self._connection_handler:
                await self._connection_handler.handle(reader, writer, peer)
            while True:
                wire = await self._read_tcp_query(reader, peer)
                if not wire:
                    break
                await self._send_tcp_response(writer, peer, wire)
        except _ConnectionTeardownRequested:
            pass
        except ConnectionResetError:
            logging.error("TCP connection from %s reset by peer", peer)
            return

        logging.debug("Closing TCP connection from %s", peer)
        writer.close()
        try:
            # Python >= 3.7
            await writer.wait_closed()
        except AttributeError:
            # Python < 3.7
            pass

    async def _read_tcp_query(
        self, reader: asyncio.StreamReader, peer: Peer
    ) -> bytes | None:
        wire_length = await self._read_tcp_query_wire_length(reader, peer)
        if not wire_length:
            return None

        return await self._read_tcp_query_wire(reader, peer, wire_length)

    async def _read_tcp_query_wire_length(
        self, reader: asyncio.StreamReader, peer: Peer
    ) -> int | None:
        logging.debug("Receiving TCP message length from %s...", peer)

        wire_length_bytes = await self._read_tcp_octets(reader, peer, 2)
        if not wire_length_bytes:
            return None

        (wire_length,) = struct.unpack("!H", wire_length_bytes)

        return wire_length

    async def _read_tcp_query_wire(
        self, reader: asyncio.StreamReader, peer: Peer, wire_length: int
    ) -> bytes | None:
        logging.debug("Receiving TCP message (%d octets) from %s...", wire_length, peer)

        wire = await self._read_tcp_octets(reader, peer, wire_length)
        if not wire:
            return None

        logging.debug("Received complete TCP message from %s: %s", peer, wire.hex())

        return wire

    async def _read_tcp_octets(
        self, reader: asyncio.StreamReader, peer: Peer, expected: int
    ) -> bytes | None:
        buffer = b""

        while len(buffer) < expected:
            chunk = await reader.read(expected - len(buffer))
            if not chunk:
                if buffer:
                    logging.debug(
                        "Received short TCP message (%d octets) from %s: %s",
                        len(buffer),
                        peer,
                        buffer.hex(),
                    )
                else:
                    logging.debug("Received disconnect from %s", peer)
                return None

            logging.debug("Received %d TCP octets from %s", len(chunk), peer)
            buffer += chunk

        return buffer

    async def _send_tcp_response(
        self, writer: asyncio.StreamWriter, peer: Peer, wire: bytes
    ) -> None:
        socket_info = writer.get_extra_info("sockname")
        socket = Peer(socket_info[0], socket_info[1])
        responses = self._handle_query(wire, socket, peer, DnsProtocol.TCP)
        async for response in responses:
            logging.debug("Sending TCP response: %s", response.hex())
            writer.write(response)
            await writer.drain()

    def _log_query(self, qctx: QueryContext) -> None:
        logging.info(
            "Received %s/%s/%s (ID=%d) query from %s on %s (%s)",
            qctx.qname.to_text(omit_final_dot=True),
            dns.rdataclass.to_text(qctx.qclass),
            dns.rdatatype.to_text(qctx.qtype),
            qctx.query.id,
            qctx.peer,
            qctx.socket,
            qctx.protocol.name,
        )
        logging.debug(
            "\n".join([f"[IN] {l}" for l in [""] + str(qctx.query).splitlines()])
        )

    def _log_response(
        self, qctx: QueryContext, response: dns.message.Message | bytes | None
    ) -> None:
        if not response:
            logging.info(
                "Not sending a response to query (ID=%d) from %s on %s (%s)",
                qctx.query.id,
                qctx.peer,
                qctx.socket,
                qctx.protocol.name,
            )
            return

        if isinstance(response, dns.message.Message):
            try:
                qname = response.question[0].name.to_text(omit_final_dot=True)
                qclass = dns.rdataclass.to_text(response.question[0].rdclass)
                qtype = dns.rdatatype.to_text(response.question[0].rdtype)
            except IndexError:
                qname = "<empty>"
                qclass = "-"
                qtype = "-"

            logging.info(
                "Sending %s/%s/%s (ID=%d) response (%d/%d/%d/%d) to a query (ID=%d) from %s on %s (%s)",
                qname,
                qclass,
                qtype,
                response.id,
                len(response.question),
                len(response.answer),
                len(response.authority),
                len(response.additional),
                qctx.query.id,
                qctx.peer,
                qctx.socket,
                qctx.protocol.name,
            )
            logging.debug(
                "\n".join([f"[OUT] {l}" for l in [""] + str(response).splitlines()])
            )
            return

        logging.info(
            "Sending response (%d bytes) to a query (ID=%d) from %s on %s (%s)",
            len(response),
            qctx.query.id,
            qctx.peer,
            qctx.socket,
            qctx.protocol.name,
        )
        logging.debug("[OUT] %s", response.hex())

    async def _handle_query(
        self, wire: bytes, socket: Peer, peer: Peer, protocol: DnsProtocol
    ) -> AsyncGenerator[bytes, None]:
        """
        Yield wire data to send as a response over the established transport.
        """
        try:
            query = self._parse_message(wire)
        except dns.exception.DNSException as exc:
            logging.error("Invalid query from %s (%s): %s", peer, wire.hex(), exc)
            return
        response_stub = _make_asyncserver_response(query)
        qctx = QueryContext(query, response_stub, socket, peer, protocol)
        self._log_query(qctx)
        responses = self._prepare_responses(qctx)
        async for response in responses:
            self._log_response(qctx, response)
            if response:
                if isinstance(response, dns.message.Message):
                    response = response.to_wire(max_size=65535)
                if protocol == DnsProtocol.UDP:
                    yield response
                else:
                    response_length = struct.pack("!H", len(response))
                    yield response_length + response

    def _parse_message(self, wire: bytes) -> dns.message.Message:
        try:
            if isinstance(self._keyring, _NoKeyringType):
                keyring = None
            else:
                keyring = self._keyring
            return dns.message.from_wire(wire, keyring=keyring)
        except dns.message.UnknownTSIGKey as exc:
            if isinstance(self._keyring, _NoKeyringType):
                error = "TSIG-signed query received but no `keyring` was provided; "
                error += "either provide a keyring (in which case the server will "
                error += "ignore any TSIG-invalid queries), or set `keyring=None` "
                error += "explicitly to disable TSIG validation altogether. "
                error += "This requires some hacking around a dnspython bug, "
                error += "so there may be unexpected side effects."
                raise ValueError(error) from exc
            if self._keyring is None:
                return _DnsMessageWithTsigDisabled.from_wire(wire)
            raise

    async def _prepare_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[dns.message.Message | bytes | None, None]:
        """
        Yield response(s) either from response handlers or zone data.
        """
        qctx.response.set_rcode(self._default_rcode)
        if self._default_aa:
            qctx.response.flags |= dns.flags.AA
        qctx.save_initialized_response(with_zone_data=False)

        self._prepare_response_from_zone_data(qctx)
        qctx.save_initialized_response(with_zone_data=True)

        response_handled = False
        async for action in self._run_response_handlers(qctx):
            yield await action.perform()
            response_handled = True

        if not response_handled:
            logging.debug("Responding based on zone data")
            yield qctx.response

    def _prepare_response_from_zone_data(self, qctx: QueryContext) -> None:
        """
        Prepare a response to the query based on the available zone data.

        The functionality is split across smaller functions that modify the
        query context until a proper response is formed.
        """
        if self._refused_response(qctx):
            return

        if self._delegation_response(qctx):
            return

        qctx.response.flags |= dns.flags.AA

        if self._ent_response(qctx):
            return

        if self._nxdomain_response(qctx):
            return

        if self._cname_response(qctx):
            return

        if self._nodata_response(qctx):
            return

        self._noerror_response(qctx)

    def _refused_response(self, qctx: QueryContext) -> bool:
        zone = self._zone_tree.find_best_zone(qctx.current_qname)
        if zone:
            qctx.zone = zone
            return False

        # RCODE is already set to self._default_rcode, i.e. REFUSED by default;
        # it should also not be changed when following a CNAME chain
        return True

    def _delegation_response(self, qctx: QueryContext) -> bool:
        assert qctx.zone

        name = qctx.current_qname
        delegation = None

        while name != qctx.zone.origin:
            node = qctx.zone.get_node(name)
            if node:
                delegation = node.get_rdataset(qctx.qclass, dns.rdatatype.NS)
                if delegation:
                    break
            name = name.parent()

        if not delegation:
            return False

        delegation_rrset = dns.rrset.RRset(name, qctx.qclass, dns.rdatatype.NS)
        delegation_rrset.update(delegation)

        qctx.response.set_rcode(dns.rcode.NOERROR)
        qctx.response.authority.append(delegation_rrset)

        self._delegation_response_additional(qctx)

        return True

    def _delegation_response_additional(self, qctx: QueryContext) -> None:
        assert qctx.zone
        assert qctx.response.authority[0]

        for nameserver in qctx.response.authority[0]:
            if not nameserver.target.is_subdomain(qctx.response.authority[0].name):
                continue
            glue_a = qctx.zone.get_rrset(nameserver.target, dns.rdatatype.A)
            if glue_a:
                qctx.response.additional.append(glue_a)
            glue_aaaa = qctx.zone.get_rrset(nameserver.target, dns.rdatatype.AAAA)
            if glue_aaaa:
                qctx.response.additional.append(glue_aaaa)

    def _ent_response(self, qctx: QueryContext) -> bool:
        assert qctx.zone
        assert qctx.zone.origin

        qctx.soa = qctx.zone.find_rrset(qctx.zone.origin, dns.rdatatype.SOA)
        assert qctx.soa

        qctx.node = qctx.zone.get_node(qctx.current_qname)
        if qctx.node or not any(
            n for n in qctx.zone.nodes if n.is_subdomain(qctx.current_qname)
        ):
            return False

        qctx.response.set_rcode(dns.rcode.NOERROR)
        qctx.response.authority.append(qctx.soa)
        return True

    def _nxdomain_response(self, qctx: QueryContext) -> bool:
        assert qctx.soa

        if qctx.node:
            return False

        qctx.response.set_rcode(dns.rcode.NXDOMAIN)
        qctx.response.authority.append(qctx.soa)
        return True

    def _cname_response(self, qctx: QueryContext) -> bool:
        assert qctx.node

        cname = qctx.node.get_rdataset(qctx.qclass, dns.rdatatype.CNAME)
        if not cname:
            return False

        qctx.response.set_rcode(dns.rcode.NOERROR)
        cname_rrset = dns.rrset.RRset(qctx.current_qname, qctx.qclass, cname.rdtype)
        cname_rrset.update(cname)
        qctx.response.answer.append(cname_rrset)

        qctx.alias = cname[0].target
        self._prepare_response_from_zone_data(qctx)
        return True

    def _nodata_response(self, qctx: QueryContext) -> bool:
        assert qctx.node
        assert qctx.soa

        qctx.answer = qctx.node.get_rdataset(qctx.qclass, qctx.qtype)
        if qctx.answer:
            return False

        qctx.response.set_rcode(dns.rcode.NOERROR)
        if not qctx.response.answer:
            qctx.response.authority.append(qctx.soa)
        return True

    def _noerror_response(self, qctx: QueryContext) -> None:
        assert qctx.answer

        answer_rrset = dns.rrset.RRset(qctx.current_qname, qctx.qclass, qctx.qtype)
        answer_rrset.update(qctx.answer)

        qctx.response.set_rcode(dns.rcode.NOERROR)
        qctx.response.answer.append(answer_rrset)

    async def _run_response_handlers(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        """
        Yield response(s) to the query from a matching query handler.
        """
        for handler in self._response_handlers:
            if handler.match(qctx):
                logging.debug("Matched response handler: %s", handler)
                async for response in handler.get_responses(qctx):
                    yield response
                return


class ControllableAsyncDnsServer(AsyncDnsServer):
    """
    An AsyncDnsServer whose behavior can be dynamically changed by sending TXT
    queries to a "magic" domain.
    """

    _CONTROL_DOMAIN = "_control."

    @functools.cached_property
    def _control_domain(self) -> dns.name.Name:
        return dns.name.from_text(self._CONTROL_DOMAIN)

    @functools.cached_property
    def _commands(self) -> dict[dns.name.Name, "ControlCommand"]:
        return {}

    def install_control_commands(self, *commands: "ControlCommand") -> None:
        for command in commands:
            self.install_control_command(command)

    def install_control_command(self, command: "ControlCommand") -> None:
        command_subdomain = dns.name.Name([command.control_subdomain])
        control_subdomain = command_subdomain.concatenate(self._control_domain)
        try:
            existing_command = self._commands[control_subdomain]
        except KeyError:
            self._commands[control_subdomain] = command
        else:
            raise RuntimeError(
                f"{control_subdomain} already handled by {existing_command}"
            )

    async def _prepare_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[dns.message.Message | bytes | None, None]:
        """
        Detect and handle control queries, falling back to normal processing
        for non-control queries.
        """
        control_response = self._handle_control_command(qctx)
        if control_response:
            yield await DnsResponseSend(response=control_response).perform()
            return

        async for response in super()._prepare_responses(qctx):
            yield response

    def _handle_control_command(self, qctx: QueryContext) -> dns.message.Message | None:
        """
        Detect and handle control queries.

        A control query must be of type TXT; if it is not, a FORMERR response
        is sent back.

        The list of commands that the server should respond to is passed to its
        constructor.  If the server is unable to handle the control query using
        any of the enabled commands, an NXDOMAIN response is sent.

        Otherwise, the relevant command's handler is expected to provide the
        response via qctx.response and/or return a string that is converted to
        a TXT RRset inserted into the ANSWER section of the response to the
        control query.  The RCODE for a command-provided response defaults to
        NOERROR, but can be overridden by the command's handler.
        """
        if not qctx.qname.is_subdomain(self._control_domain):
            return None

        if qctx.qtype != dns.rdatatype.TXT:
            logging.error("Non-TXT control query %s from %s", qctx.qname, qctx.peer)
            qctx.response.set_rcode(dns.rcode.FORMERR)
            return qctx.response

        control_subdomain = dns.name.Name(qctx.qname.labels[-3:])
        try:
            command = self._commands[control_subdomain]
        except KeyError:
            logging.error("Unhandled control query %s from %s", qctx.qname, qctx.peer)
            qctx.response.set_rcode(dns.rcode.NXDOMAIN)
            return qctx.response

        logging.info("Received control query %s from %s", qctx.qname, qctx.peer)
        logging.debug("Handling control query %s using %s", qctx.qname, command)
        qctx.response.set_rcode(dns.rcode.NOERROR)
        qctx.response.flags |= dns.flags.AA

        command_qname = qctx.qname.relativize(control_subdomain)
        try:
            command_args = [l.decode("ascii") for l in command_qname.labels]
        except UnicodeDecodeError:
            logging.error("Non-ASCII control query %s from %s", qctx.qname, qctx.peer)
            qctx.response.set_rcode(dns.rcode.FORMERR)
            return qctx.response

        command_response = command.handle(command_args, self, qctx)
        if command_response:
            command_response_rrset = dns.rrset.from_text(
                qctx.qname, 0, qctx.qclass, dns.rdatatype.TXT, f'"{command_response}"'
            )
            qctx.response.answer.append(command_response_rrset)

        return qctx.response


class ControlCommand(abc.ABC):
    """
    Base class for control commands.

    The derived class must define the control query subdomain that it handles
    and the callback that handles the control queries.
    """

    @property
    @abc.abstractmethod
    def control_subdomain(self) -> str:
        """
        The subdomain of the control domain handled by this command.  Needs to
        be defined as a string by the derived class.
        """
        raise NotImplementedError

    @abc.abstractmethod
    def handle(
        self, args: list[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> str | None:
        """
        This method is expected to carry out arbitrary actions in response to a
        control query.  Note that it is invoked synchronously (it is not a
        coroutine).

        `args` is a list of arguments for the command extracted from the
        control query's QNAME; these arguments (and therefore the QNAME as
        well) must only contain ASCII characters.  For example, if a command's
        subdomain is `my-command`, control query `foo.bar.my-command._control.`
        causes `args` to be set to `["foo", "bar"]` while control query
        `my-command._control.` causes `args` to be set to `[]`.

        `server` is the server instance that received the control query.  This
        method can change the server's behavior by altering its response
        handler list using the appropriate methods.

        `qctx` is the query context for the control query.  By operating on
        qctx.response, this method can prepare the DNS response sent to
        the client in response to the control query.  Alternatively (or in
        addition to the above), it can also return a string; if it does, the
        returned string is converted to a TXT RRset that is inserted into the
        ANSWER section of the response to the control query.
        """
        raise NotImplementedError

    def __str__(self) -> str:
        return self.__class__.__name__


class ToggleResponsesCommand(ControlCommand):
    """
    Disable/enable sending responses from the server.
    """

    control_subdomain = "send-responses"

    def __init__(self) -> None:
        self._current_handler: IgnoreAllQueries | None = None

    def handle(
        self, args: list[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> str | None:
        if len(args) != 1:
            logging.error("Invalid %s query %s", self, qctx.qname)
            qctx.response.set_rcode(dns.rcode.SERVFAIL)
            return "invalid query; use exactly one of 'enable' or 'disable' in QNAME"

        mode = args[0]

        if mode == "disable":
            if self._current_handler:
                return "sending responses already disabled"
            self._current_handler = IgnoreAllQueries()
            server.install_response_handler(self._current_handler, prepend=True)
            return "sending responses disabled"

        if mode == "enable":
            if not self._current_handler:
                return "sending responses already enabled"
            server.uninstall_response_handler(self._current_handler)
            self._current_handler = None
            return "sending responses enabled"

        logging.error("Unrecognized response sending mode '%s'", mode)
        qctx.response.set_rcode(dns.rcode.SERVFAIL)
        return f"unrecognized response sending mode '{mode}'"


class SwitchControlCommand(ControlCommand):
    """
    Switch the server's response handlers based on the control query.

    A sequence of response handlers is associated with each key.  When a
    control query is received, the server's response handlers are replaced
    with the sequence associated with the key extracted from the control
    query.
    """

    control_subdomain = "switch"

    def __init__(self, handler_mapping: dict[str, Sequence[ResponseHandler]]):
        self._handler_mapping = handler_mapping

    def handle(
        self, args: list[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> str | None:
        if len(args) != 1 or args[0] not in self._handler_mapping:
            logging.error("Invalid %s query %s", self, qctx.qname)
            qctx.response.set_rcode(dns.rcode.SERVFAIL)
            return f"invalid query; exactly one of {list(self._handler_mapping.keys())} is expected in QNAME"

        server.replace_response_handlers(*self._handler_mapping[args[0]])
        return f"switched to handler set '{args[0]}'"
