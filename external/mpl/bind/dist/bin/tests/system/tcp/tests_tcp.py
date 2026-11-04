#!/usr/bin/python3

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

from collections.abc import Iterable, Iterator
from types import TracebackType
from typing import NamedTuple

import asyncio
import contextlib
import socket
import struct
import time

import dns.message
import dns.name
import dns.query
import dns.rcode
import dns.rrset
import pytest

from isctest.instance import NamedInstance

import isctest

pytestmark = pytest.mark.extra_artifacts(["ns*/named.stats"])

TIMEOUT: int = 10


class TcpStatus(NamedTuple):
    current: int
    limit: int
    high_water: int
    recursive_high_water: int

    def check(
        self,
        *,
        current: int | None = None,
        limit: int | None = None,
        high_water: int | None = None,
        recursive_high_water: int | None = None,
    ) -> None:
        if current is not None:
            assert (
                self.current == current
            ), f"current TCP clients count: expected {current}, got {self.current}"
        if limit is not None:
            assert (
                self.limit == limit
            ), f"TCP clients limit: expected {limit}, got {self.limit}"
        if high_water is not None:
            assert (
                self.high_water == high_water
            ), f"TCP high-water value: expected {high_water}, got {self.high_water}"
        if recursive_high_water is not None:
            assert self.recursive_high_water == recursive_high_water, (
                "recursive high-water value: "
                f"expected {recursive_high_water}, got {self.recursive_high_water}"
            )

    @classmethod
    def of(cls, ns: NamedInstance) -> "TcpStatus":
        status = ns.rndc("status").out

        def value(label: str) -> str:
            matches = status.grep(f"{label}:")
            assert matches, f"'{label}' not found in rndc status:\n{status}"
            line = matches[0].string.strip()
            _, _, result = line.partition(":")
            return result.strip()

        current, limit = value("tcp clients").split("/", maxsplit=1)

        return cls(
            current=int(current),
            limit=int(limit),
            high_water=int(value("TCP high-water")),
            recursive_high_water=int(value("recursive high-water")),
        )

    @classmethod
    def wait_for(
        cls,
        ns: NamedInstance,
        *,
        current: int | None = None,
        limit: int | None = None,
        high_water: int | None = None,
        recursive_high_water: int | None = None,
        timeout: int = 2,
        delay: int = 1,
    ) -> "TcpStatus":
        status: TcpStatus | None = None

        def check() -> bool:
            nonlocal status
            status = cls.of(ns)
            status.check(
                current=current,
                limit=limit,
                high_water=high_water,
                recursive_high_water=recursive_high_water,
            )
            return True

        isctest.run.retry_with_timeout(check, timeout=timeout, delay=delay)
        assert status is not None
        return status


class TcpConnectionPool:
    OPEN_TIMEOUT = 2

    def __init__(self) -> None:
        self.connections: list[socket.socket] = []

    async def __aenter__(self) -> "TcpConnectionPool":
        return self

    async def __aexit__(
        self,
        _exc_type: type[BaseException] | None,
        _exc: BaseException | None,
        _tb: TracebackType | None,
    ) -> None:
        await self.close()

    async def open(self, count: int, host: str, port: int) -> None:
        tasks = [
            asyncio.create_task(open_tcp_query_connection(host, port))
            for _ in range(count)
        ]

        try:
            results = await asyncio.wait_for(
                asyncio.gather(*tasks, return_exceptions=True),
                timeout=self.OPEN_TIMEOUT,
            )
        except asyncio.TimeoutError as exc:
            for task in tasks:
                task.cancel()
            results = await asyncio.gather(*tasks, return_exceptions=True)
            close_tcp_connections(
                result for result in results if isinstance(result, socket.socket)
            )
            raise AssertionError(f"timed out opening {count} TCP connections") from exc

        connections = [
            result for result in results if isinstance(result, socket.socket)
        ]
        errors = [result for result in results if isinstance(result, BaseException)]
        if errors:
            close_tcp_connections(connections)
            raise AssertionError(
                f"{len(errors)} TCP connection(s) failed: {errors[0]!r}"
            )

        self.connections.extend(connections)

    async def close(self, count: int = 0) -> None:
        if count == 0:
            count = len(self.connections)

        assert count <= len(
            self.connections
        ), f"cannot close {count} of {len(self.connections)} active connection(s)"

        closing = self.connections[:count]
        del self.connections[:count]
        close_tcp_connections(closing)


def create_socket(host: str, port: int) -> socket.socket:
    sock = socket.create_connection((host, port), timeout=10)
    sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, True)
    return sock


def tcp_requests_received(ns: NamedInstance) -> int:
    ns.rndc("stats")
    stats = isctest.text.TextFile(str(ns.directory / "named.stats"))
    matches = stats.grep("TCP requests received")
    assert matches, f"'TCP requests received' not found in {stats}"
    # `rndc stats` appends to the file; only the last occurrence is current
    return int(matches[-1].string.split()[0])


def check_tcp_response(server_ip: str) -> None:
    msg = isctest.query.create("txt.example.", "A")
    response = isctest.query.tcp(msg, server_ip)
    isctest.check.nxdomain(response)


def tcp_round_trip(
    sock: socket.socket, msg: dns.message.Message
) -> dns.message.Message:
    expiration = time.time() + TIMEOUT
    dns.query.send_tcp(sock, msg, expiration)
    response, _ = dns.query.receive_tcp(sock, expiration)
    return response


async def open_tcp_query_connection(host: str, port: int) -> socket.socket:
    try:
        socket.inet_pton(socket.AF_INET, host)
        family = socket.AF_INET
    except OSError:
        family = socket.AF_INET6

    sock = socket.socket(family, socket.SOCK_STREAM)
    sock.setblocking(False)

    try:
        loop = asyncio.get_running_loop()
        await loop.sock_connect(sock, (host, port))
        msg = isctest.query.create(
            "version.bind.", "TXT", "CH", dnssec=False, use_edns=False, ad=False
        )
        await loop.sock_sendall(sock, msg.to_wire(prepend_length=True))
    except BaseException:
        # BaseException so the socket is also closed when the task is
        # cancelled at one of the awaits, not just on connection errors
        sock.close()
        raise

    return sock


def close_tcp_connections(connections: Iterable[socket.socket]) -> None:
    for sock in connections:
        sock.close()


async def send_long_tcp_stream(
    host: str, port: int, message: dns.message.Message, min_bytes: int
) -> None:
    frame = message.to_wire(prepend_length=True)
    chunk_frames = max(1, 65536 // len(frame))
    frames_remaining = (min_bytes + len(frame) - 1) // len(frame)

    async def discard_stream(reader: asyncio.StreamReader) -> None:
        with contextlib.suppress(OSError):
            while await reader.read(65535):
                pass

    async def run() -> None:
        reader, writer = await asyncio.open_connection(host, port)
        discard_task = asyncio.create_task(discard_stream(reader))
        try:
            with contextlib.suppress(ConnectionError):
                remaining = frames_remaining
                while remaining > 0:
                    frames = min(chunk_frames, remaining)
                    writer.write(frame * frames)
                    await writer.drain()
                    remaining -= frames
                writer.write_eof()
                await writer.drain()
            writer.close()
            with contextlib.suppress(ConnectionError, OSError):
                await writer.wait_closed()
            await discard_task
        finally:
            writer.close()
            if not discard_task.done():
                discard_task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await discard_task

    await asyncio.wait_for(run(), timeout=10 * TIMEOUT)


def test_tcp_garbage(ns7: NamedInstance, named_port: int) -> None:
    with create_socket(ns7.ip, named_port) as sock:
        msg = isctest.query.create(
            "a.example.", "A", dnssec=False, use_edns=-1, ad=False
        )
        tcp_round_trip(sock, msg)

        # Send DNS message shorter than DNS message header (12),
        # this should cause the connection to be terminated
        sock.send(struct.pack("!H", 11))
        sock.send(struct.pack("!s", b"0123456789a"))

        with pytest.raises(EOFError):
            try:
                tcp_round_trip(sock, msg)
            except ConnectionError as e:
                raise EOFError from e


def test_tcp_garbage_response(ns7: NamedInstance, named_port: int) -> None:
    with create_socket(ns7.ip, named_port) as sock:
        msg = isctest.query.create(
            "a.example.", "A", dnssec=False, use_edns=-1, ad=False
        )
        tcp_round_trip(sock, msg)

        # Send DNS response instead of DNS query, this should cause
        # the connection to be terminated

        rmsg = dns.message.make_response(msg)

        with pytest.raises(EOFError):
            try:
                tcp_round_trip(sock, rmsg)
            except ConnectionError as e:
                raise EOFError from e


# Regression test for CVE-2022-0396
def test_close_wait(ns7: NamedInstance, named_port: int) -> None:
    with create_socket(ns7.ip, named_port) as sock:
        msg = isctest.query.create(
            "a.example.", "A", dnssec=False, use_edns=-1, ad=False
        )
        tcp_round_trip(sock, msg)

        msg = isctest.query.create(
            "a.example.", "A", dnssec=False, use_edns=0, payload=1232, ad=False
        )
        dns.query.send_tcp(sock, msg, time.time() + TIMEOUT)

        # Shutdown the socket, but ignore the other side closing the socket
        # first because we sent DNS message with EDNS0
        try:
            sock.shutdown(socket.SHUT_RDWR)
        except ConnectionError:
            pass
        except OSError:
            pass

    # BIND allows one TCP client, the part above sends DNS messaage with EDNS0
    # after the first query. BIND should react adequately because of
    # ns7/named.dropedns and close the socket, making room for the next
    # request. If it gets stuck in CLOSE_WAIT state, there is no connection
    # available for the query below and it will time out.
    with create_socket(ns7.ip, named_port) as sock:
        msg = isctest.query.create(
            "a.example.", "A", dnssec=False, use_edns=-1, ad=False
        )
        tcp_round_trip(sock, msg)


# GL #4273
def test_tcp_big(ns7: NamedInstance, named_port: int) -> None:
    with create_socket(ns7.ip, named_port) as sock:
        msg = isctest.query.create(
            dns.name.root, "URI", dnssec=False, use_edns=-1, ad=False, message_id=0
        )
        msg.additional.append(
            dns.rrset.from_text(dns.name.root, 0, 1, "URI", "0 0 " + "b" * 65503)
        )
        tcp_round_trip(sock, msg)

        # Now check that the server is alive and well
        msg = isctest.query.create(
            "a.example.", "A", dnssec=False, use_edns=-1, ad=False
        )
        tcp_round_trip(sock, msg)


def wait_for_stable_tcp_requests(ns: NamedInstance, timeout: int = 10) -> int:
    """Read the TCP request counter until it stops changing.

    The counter is incremented on request receipt, so a client response
    implies its upstream queries are already counted; this only needs to
    absorb unsynchronized traffic such as the resolver's root priming query.
    """
    last = -1

    def stable() -> bool:
        nonlocal last
        previous, last = last, tcp_requests_received(ns)
        return previous == last

    isctest.run.retry_with_timeout(stable, timeout=timeout)
    return last


def test_tcp_request_statistics(
    ns1: NamedInstance, ns2: NamedInstance, ns3: NamedInstance, ns4: NamedInstance
) -> None:
    isctest.log.info("initializing TCP statistics")
    ns1_tcp = tcp_requests_received(ns1)
    ns2_tcp = tcp_requests_received(ns2)

    isctest.log.info("checking TCP request statistics (resolver)")
    msg = isctest.query.create("txt.example.", "A")
    isctest.query.udp(msg, ns3.ip, expected_rcode=dns.rcode.NXDOMAIN)

    ns1_tcp_after_resolver = wait_for_stable_tcp_requests(ns1)
    ns2_tcp_after_resolver = wait_for_stable_tcp_requests(ns2)
    assert ns1_tcp < ns1_tcp_after_resolver
    assert ns2_tcp == ns2_tcp_after_resolver

    isctest.log.info("checking TCP request statistics (forwarder)")
    msg = isctest.query.create("txt.example.", "A")
    isctest.query.udp(msg, ns4.ip, expected_rcode=dns.rcode.NXDOMAIN)

    ns1_tcp_after_forwarder = wait_for_stable_tcp_requests(ns1)
    ns2_tcp_after_forwarder = wait_for_stable_tcp_requests(ns2)
    assert ns1_tcp_after_resolver == ns1_tcp_after_forwarder
    assert ns2_tcp_after_resolver < ns2_tcp_after_forwarder


def test_tcp_high_water(ns5: NamedInstance, named_port: int) -> None:
    async def run() -> None:
        async with TcpConnectionPool() as pool:
            isctest.log.info("checking TCP query response")
            check_tcp_response(ns5.ip)

            ns5.rndc("reset-stats tcp-high-water recursive-high-water")
            isctest.log.info(
                "TCP and recursive high-water: check statistics after reset"
            )
            status = TcpStatus.of(ns5)
            status.check(current=0, high_water=0, recursive_high_water=0)

            isctest.log.info(
                "TCP and recursive high-water: check values after some TCP "
                "and UDP connections are established"
            )
            old_status = status
            tcp_added = 9
            rec_added = 1
            msg = isctest.query.create("recurse.example.", "A")
            isctest.query.udp(msg, ns5.ip)
            await pool.open(tcp_added, ns5.ip, named_port)
            status = TcpStatus.wait_for(
                ns5,
                current=old_status.current + tcp_added,
                high_water=old_status.current + tcp_added,
                recursive_high_water=old_status.recursive_high_water + rec_added,
            )

            isctest.log.info(
                "TCP high-water: check value after some TCP connections are closed"
            )
            old_status = status
            tcp_removed = 5
            await pool.close(tcp_removed)
            status = TcpStatus.wait_for(
                ns5,
                current=old_status.current - tcp_removed,
                high_water=old_status.high_water,
            )

            isctest.log.info("TCP high-water: ensure tcp-clients is an upper bound")
            await pool.open(status.limit + 1, ns5.ip, named_port)
            TcpStatus.wait_for(
                ns5,
                current=status.limit,
                high_water=status.limit,
            )

            isctest.log.info("checking TCP response recovery")
            await pool.close()
            check_tcp_response(ns5.ip)

    asyncio.run(run())


def debug_level(ns: NamedInstance) -> int:
    status = ns.rndc("status").out
    matches = status.grep("debug level:")
    assert matches, f"'debug level' not found in rndc status:\n{status}"
    return int(matches[0].string.partition(":")[2])


@contextlib.contextmanager
def temporary_trace_level(ns: NamedInstance, level: int) -> Iterator[None]:
    """Lower the debug level for a noisy section, then restore the default."""
    prev_level = debug_level(ns)
    ns.rndc(f"trace {level}")
    try:
        yield
    finally:
        # Don't mask an in-flight test failure if named has died.
        ns.rndc(f"trace {prev_level}", raise_on_exception=False)


def test_long_tcp_messages(ns1: NamedInstance, named_port: int) -> None:
    isctest.log.info("checking that BIND 9 doesn't crash on long TCP messages")
    stream_bytes = 6 * 1024 * 1024
    msg = isctest.query.create(
        "isc.org.",
        "AXFR",
        dnssec=False,
        use_edns=False,
        rd=False,
        ad=False,
        message_id=1,
    )

    # Avoid logging the huge query stream at the default debug level.
    with temporary_trace_level(ns1, 1):
        asyncio.run(send_long_tcp_stream(ns1.ip, named_port, msg, stream_bytes))

        msg = isctest.query.create("txt.example.", "A")
        isctest.query.tcp(msg, ns1.ip)
