"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from dataclasses import dataclass, field
from typing import (
    Any,
    AsyncGenerator,
    Callable,
    Coroutine,
    List,
    Optional,
    Tuple,
    Union,
    cast,
)

import abc
import asyncio
import enum
import functools
import logging
import os
import pathlib
import re
import signal
import struct
import sys

import dns.flags
import dns.message
import dns.name
import dns.node
import dns.rcode
import dns.rdataclass
import dns.rdatatype
import dns.rrset
import dns.zone

try:
    RdataType = dns.rdatatype.RdataType
    RdataClass = dns.rdataclass.RdataClass
except AttributeError:  # dnspython < 2.0.0 compat
    RdataType = int  # type: ignore
    RdataClass = int  # type: ignore


_UdpHandler = Callable[
    [bytes, Tuple[str, int], asyncio.DatagramTransport], Coroutine[Any, Any, None]
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
        self._transport: Optional[asyncio.DatagramTransport] = None
        self._handler: _UdpHandler = handler

    def connection_made(self, transport: asyncio.BaseTransport) -> None:
        """
        Called by asyncio when a connection is made.
        """
        self._transport = cast(asyncio.DatagramTransport, transport)

    def datagram_received(self, data: bytes, addr: Tuple[str, int]) -> None:
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
        udp_handler: Optional[_UdpHandler],
        tcp_handler: Optional[_TcpHandler],
        pidfile: Optional[str] = None,
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

        self._ip_addresses: Tuple[str, str] = (ipv4_address, ipv6_address)
        self._port: int = port
        self._udp_handler: Optional[_UdpHandler] = udp_handler
        self._tcp_handler: Optional[_TcpHandler] = tcp_handler
        self._pidfile: Optional[str] = pidfile
        self._work_done: Optional[asyncio.Future] = None

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

    def _setup_signals(self) -> None:
        loop = self._get_asyncio_loop()
        self._work_done = loop.create_future()
        loop.add_signal_handler(signal.SIGINT, functools.partial(self._signal_done))
        loop.add_signal_handler(signal.SIGTERM, functools.partial(self._signal_done))

    def _signal_done(self) -> None:
        assert self._work_done
        self._work_done.set_result(True)

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


@dataclass
class QueryContext:
    """
    Context for the incoming query which may be used for preparing the response.
    """

    query: dns.message.Message
    response: dns.message.Message
    peer: Tuple[str, int]
    protocol: DnsProtocol
    zone: Optional[dns.zone.Zone] = None
    soa: Optional[dns.rrset.RRset] = None
    node: Optional[dns.node.Node] = None
    answer: Optional[dns.rdataset.Rdataset] = None

    @property
    def qname(self) -> dns.name.Name:
        return self.query.question[0].name

    @property
    def qclass(self) -> RdataClass:
        return self.query.question[0].rdclass

    @property
    def qtype(self) -> RdataType:
        return self.query.question[0].rdtype


@dataclass
class ResponseAction(abc.ABC):
    """
    Base class for actions that can be taken in response to a query.
    """

    @abc.abstractmethod
    async def perform(self) -> Optional[Union[dns.message.Message, bytes]]:
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
    authoritative: Optional[bool] = None
    delay: float = 0.0

    async def perform(self) -> Optional[Union[dns.message.Message, bytes]]:
        """
        Yield a potentially delayed response that is a dns.message.Message.
        """
        assert isinstance(self.response, dns.message.Message)
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

    async def perform(self) -> Optional[Union[dns.message.Message, bytes]]:
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

    async def perform(self) -> Optional[Union[dns.message.Message, bytes]]:
        return None


class ResponseHandler(abc.ABC):
    """
    Base class for generic response handlers.

    If a query passes the `match()` function logic, then it is handled by this
    response handler and response(s) may be generated by the `get_responses()`
    method.
    """

    @abc.abstractmethod
    def match(self, qctx: QueryContext) -> bool:
        """
        Matching logic - query is handled when it returns True.
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


class DomainHandler(ResponseHandler):
    """
    Base class used for deriving custom domain handlers.

    The derived class must specify a list of `domains` that it wants to handle.
    Queries for any of these domains (and their subdomains) will then be passed
    to the `get_response()` method in the derived class.
    """

    @property
    @abc.abstractmethod
    def domains(self) -> List[str]:
        """
        A list of domain names handled by this class.
        """
        raise NotImplementedError

    def __init__(self) -> None:
        self._domains: List[dns.name.Name] = [
            dns.name.from_text(d) for d in self.domains
        ]

    def __str__(self) -> str:
        return f"{self.__class__.__name__}(domains: {', '.join(self.domains)})"

    def match(self, qctx: QueryContext) -> bool:
        """
        Handle queries whose QNAME matches any of the domains handled by this
        class.
        """
        for domain in self._domains:
            if qctx.qname.is_subdomain(domain):
                return True
        return False


@dataclass
class _ZoneTreeNode:
    """
    A node representing a zone with one origin.
    """

    zone: Optional[dns.zone.Zone]
    children: List["_ZoneTreeNode"] = field(default_factory=list)


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

    def find_best_zone(self, name: dns.name.Name) -> Optional[dns.zone.Zone]:
        """
        Return the closest matching zone (if any) for the domain name.
        """
        node = self._find_best_match(name, self._root)
        return node.zone if node != self._root else None


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

    def __init__(self, load_zones: bool = True):
        super().__init__(self._handle_udp, self._handle_tcp, "ans.pid")

        self._zone_tree: _ZoneTree = _ZoneTree()
        self._response_handlers: List[ResponseHandler] = []

        if load_zones:
            self._load_zones()

    def install_response_handler(self, handler: ResponseHandler) -> None:
        """
        Add a response handler which will be used to handle matching queries.

        Response handlers can modify, replace, or suppress the answers prepared
        from zone file contents.
        """
        logging.info("Installing response handler: %s", handler)
        self._response_handlers.append(handler)

    def _load_zones(self) -> None:
        for entry in os.scandir():
            entry_path = pathlib.Path(entry.path)
            if entry_path.suffix != ".db":
                continue
            origin = dns.name.from_text(entry_path.stem)
            logging.info("Loading zone file %s", entry_path)
            zone = dns.zone.from_file(entry.path, origin, relativize=False)
            self._zone_tree.add(zone)

    async def _handle_udp(
        self, wire: bytes, peer: Tuple[str, int], transport: asyncio.DatagramTransport
    ) -> None:
        logging.debug("Received UDP message: %s", wire.hex())
        responses = self._handle_query(wire, peer, DnsProtocol.UDP)
        async for response in responses:
            transport.sendto(response, peer)

    async def _handle_tcp(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        wire_length_bytes = await reader.read(2)
        (wire_length,) = struct.unpack("!H", wire_length_bytes)
        logging.debug("Receiving TCP message (%d octets)...", wire_length)

        wire = await reader.read(wire_length)
        full_message = wire_length_bytes + wire
        logging.debug("Received complete TCP message: %s", full_message.hex())

        peer = writer.get_extra_info("peername")
        responses = self._handle_query(wire, peer, DnsProtocol.TCP)
        async for response in responses:
            writer.write(response)
            try:
                await writer.drain()
            except ConnectionResetError:
                logging.error(
                    "TCP connection from %s reset by peer", self._format_peer(peer)
                )
                return

        writer.close()
        await writer.wait_closed()

    def _format_peer(self, peer: Tuple[str, int]) -> str:
        host = peer[0]
        port = peer[1]
        if "::" in host:
            host = f"[{host}]"
        return f"{host}:{port}"

    def _log_query(
        self, qctx: QueryContext, peer: Tuple[str, int], protocol: DnsProtocol
    ) -> None:
        logging.info(
            "Received %s/%s/%s (ID=%d) query from %s (%s)",
            qctx.qname.to_text(omit_final_dot=True),
            dns.rdataclass.to_text(qctx.qclass),
            dns.rdatatype.to_text(qctx.qtype),
            qctx.query.id,
            self._format_peer(peer),
            protocol.name,
        )
        logging.debug(
            "\n".join([f"[IN] {l}" for l in [""] + str(qctx.query).splitlines()])
        )

    def _log_response(
        self,
        qctx: QueryContext,
        response: Optional[Union[dns.message.Message, bytes]],
        peer: Tuple[str, int],
        protocol: DnsProtocol,
    ) -> None:
        if not response:
            logging.info(
                "Not sending a response to query (ID=%d) from %s (%s)",
                qctx.query.id,
                self._format_peer(peer),
                protocol.name,
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
                "Sending %s/%s/%s (ID=%d) response (%d/%d/%d/%d) to a query (ID=%d) from %s (%s)",
                qname,
                qclass,
                qtype,
                response.id,
                len(response.question),
                len(response.answer),
                len(response.authority),
                len(response.additional),
                qctx.query.id,
                self._format_peer(peer),
                protocol.name,
            )
            logging.debug(
                "\n".join([f"[OUT] {l}" for l in [""] + str(response).splitlines()])
            )
            return

        logging.info(
            "Sending response (%d bytes) to a query (ID=%d) from %s (%s)",
            len(response),
            qctx.query.id,
            self._format_peer(peer),
            protocol.name,
        )
        logging.debug("[OUT] %s", response.hex())

    async def _handle_query(
        self, wire: bytes, peer: Tuple[str, int], protocol: DnsProtocol
    ) -> AsyncGenerator[bytes, None]:
        """
        Yield wire data to send as a response over the established transport.
        """
        query = dns.message.from_wire(wire)
        response_stub = dns.message.make_response(query)
        qctx = QueryContext(query, response_stub, peer, protocol)
        self._log_query(qctx, peer, protocol)
        responses = self._prepare_responses(qctx)
        async for response in responses:
            self._log_response(qctx, response, peer, protocol)
            if response:
                if isinstance(response, dns.message.Message):
                    response = response.to_wire(max_size=65535)
                if protocol == DnsProtocol.UDP:
                    yield response
                else:
                    response_length = struct.pack("!H", len(response))
                    yield response_length + response

    async def _prepare_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[Optional[Union[dns.message.Message, bytes]], None]:
        """
        Yield response(s) either from response handlers or zone data.
        """
        self._prepare_response_from_zone_data(qctx)

        response_handled = False
        async for action in self._run_response_handlers(qctx):
            yield await action.perform()
            response_handled = True

        if not response_handled:
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

        if self._nodata_response(qctx):
            return

        self._noerror_response(qctx)

    def _refused_response(self, qctx: QueryContext) -> bool:
        qctx.zone = self._zone_tree.find_best_zone(qctx.qname)
        if qctx.zone:
            return False

        qctx.response.set_rcode(dns.rcode.REFUSED)
        return True

    def _delegation_response(self, qctx: QueryContext) -> bool:
        assert qctx.zone

        name = qctx.qname
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

        qctx.node = qctx.zone.get_node(qctx.qname)
        if qctx.node or not any(
            n for n in qctx.zone.nodes if n.is_subdomain(qctx.qname)
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

    def _nodata_response(self, qctx: QueryContext) -> bool:
        assert qctx.node
        assert qctx.soa

        qctx.answer = qctx.node.get_rdataset(qctx.qclass, qctx.qtype)
        if qctx.answer:
            return False

        qctx.response.set_rcode(dns.rcode.NOERROR)
        qctx.response.authority.append(qctx.soa)
        return True

    def _noerror_response(self, qctx: QueryContext) -> None:
        assert qctx.answer

        answer_rrset = dns.rrset.RRset(qctx.qname, qctx.qclass, qctx.qtype)
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
                async for response in handler.get_responses(qctx):
                    yield response
                return
