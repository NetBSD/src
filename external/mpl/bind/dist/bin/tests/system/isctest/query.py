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

from collections.abc import Callable
from typing import Any

import os
import time

import dns.exception
import dns.flags
import dns.message
import dns.name
import dns.query
import dns.rcode
import dns.rdataclass
import dns.rdatatype

import isctest.log
import isctest.run

QUERY_TIMEOUT = 10


def generic_query(
    query_func: Callable[..., Any],
    message: dns.message.Message,
    ip: str,
    port: int | None = None,
    source: str | None = None,
    timeout: int = QUERY_TIMEOUT,
    attempts: int = 10,
    expected_rcode: dns.rcode.Rcode | None = None,
    verify: bool = False,
    log_query: bool = True,
    log_response: bool = True,
) -> Any:

    def _safe_to_text(msg: dns.message.Message) -> str:
        """
        Convert a DNS message to text, tolerating dnspython's failure to render
        RRSIG inception/expiration timestamps that overflow the platform's
        time_t (e.g. post-2038 values on 32-bit systems).
        """
        try:
            return msg.to_text()
        except OverflowError:
            return "<message not representable as text>"

    def log_querymsg(exception: Exception | None = None) -> None:
        """
        Helper for logging query message. Call this *after* query_func() has
        been called, as it may modify the message, e.g. with a TSIG.

        If an exception is provided, it will be logged as well.
        """
        nonlocal log_query
        if log_query:
            isctest.log.debug(
                f"isc.query.{query_func.__name__}(): query\n{_safe_to_text(message)}"
            )
            log_query = False  # only log query once

        if exception:
            isctest.log.debug(
                f"isc.query.{query_func.__name__}(): the '{exception}' exception raised"
            )

    if port is None:
        if query_func.__name__ == "tls":
            port = int(os.environ["TLSPORT"])
        else:
            port = int(os.environ["PORT"])

    query_args = {
        "q": message,
        "where": ip,
        "timeout": timeout,
        "port": port,
        "source": source,
    }
    if query_func.__name__ == "tls":
        query_args["verify"] = verify

    res = None

    for attempt in range(attempts):
        log_msg = (
            f"isc.query.{query_func.__name__}(): ip={ip}, port={port}, source={source}, "
            f"timeout={timeout}, attempts left={attempts-attempt}"
        )
        isctest.log.debug(log_msg)

        exc = None
        try:
            res = query_func(**query_args)
        except (dns.exception.Timeout, ConnectionRefusedError) as e:
            exc = e
        finally:
            log_querymsg(exc)

        if res:
            if log_response:
                isctest.log.debug(
                    f"isc.query.{query_func.__name__}(): response\n{_safe_to_text(res)}"
                )
            if res.rcode() == expected_rcode or expected_rcode is None:
                return res

        time.sleep(1)

    if expected_rcode is not None:
        last_rcode = dns.rcode.to_text(res.rcode()) if res else None
        isctest.log.debug(
            f"isc.query.{query_func.__name__}(): expected rcode={dns.rcode.to_text(expected_rcode)}, last rcode={last_rcode}"
        )
    raise dns.exception.Timeout


def udp(*args, **kwargs) -> Any:
    return generic_query(dns.query.udp, *args, **kwargs)


def tcp(*args, **kwargs) -> Any:
    return generic_query(dns.query.tcp, *args, **kwargs)


def tls(*args, **kwargs) -> Any:
    return generic_query(dns.query.tls, *args, **kwargs)


def create(
    qname,
    qtype,
    qclass=dns.rdataclass.IN,
    dnssec: bool = True,
    use_edns: int | bool = True,
    payload: int = 1232,
    rd: bool = True,
    cd: bool = False,
    ad: bool = True,
    message_id: int | None = None,
) -> dns.message.Message:
    """Create DNS query with defaults suitable for our tests."""
    msg = dns.message.make_query(
        qname,
        qtype,
        qclass,
        use_edns=use_edns,
        want_dnssec=dnssec,
        payload=payload,
        id=message_id,
    )
    msg.flags = 0
    if rd:
        msg.flags = dns.flags.RD
    if ad:
        msg.flags |= dns.flags.AD
    if cd:
        msg.flags |= dns.flags.CD
    return msg


def get_soa_serial(server_ip, zone, timeout=10):
    """
    Get the current SOA serial of a zone from a server.

    Queries the server repeatedly until it responds with a well-formed
    SOA answer or the timeout expires.
    """
    query = create(zone, "SOA", dnssec=False)
    serial = None

    def check():
        nonlocal serial
        res = tcp(
            query,
            server_ip,
            timeout=3,
            attempts=1,
            expected_rcode=dns.rcode.NOERROR,
        )
        soa = res.get_rrset(
            res.answer,
            dns.name.from_text(zone),
            dns.rdataclass.IN,
            dns.rdatatype.SOA,
        )
        assert soa is not None and len(soa) == 1
        serial = soa[0].serial
        return True

    isctest.run.retry_with_timeout(
        check,
        timeout=timeout,
        msg=f"timed out getting SOA serial of {zone} from {server_ip}",
    )
    return serial


def wait_for_serial(server_ip, zone, expected_serial, timeout=30):
    """
    Wait until the server has the expected SOA serial for the zone.

    Queries the server repeatedly until the SOA serial matches or the
    timeout expires.
    """

    def check():
        return get_soa_serial(server_ip, zone) == expected_serial

    isctest.run.retry_with_timeout(
        check,
        timeout=timeout,
        msg=f"timed out waiting for serial {expected_serial} at {server_ip} for {zone}",
    )
