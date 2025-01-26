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

import os
import time
from typing import Any, Callable, Optional

import dns.query
import dns.message

import isctest.log
from isctest.compat import dns_rcode

QUERY_TIMEOUT = 10


def generic_query(
    query_func: Callable[..., Any],
    message: dns.message.Message,
    ip: str,
    port: Optional[int] = None,
    source: Optional[str] = None,
    timeout: int = QUERY_TIMEOUT,
    attempts: int = 10,
    expected_rcode: dns_rcode = None,
) -> Any:
    if port is None:
        port = int(os.environ["PORT"])
    res = None
    for attempt in range(attempts):
        try:
            isctest.log.debug(
                f"{query_func.__name__}(): ip={ip}, port={port}, source={source}, "
                f"timeout={timeout}, attempts left={attempts-attempt}"
            )
            res = query_func(message, ip, timeout, port=port, source=source)
            if res.rcode() == expected_rcode or expected_rcode is None:
                return res
        except (dns.exception.Timeout, ConnectionRefusedError) as e:
            isctest.log.debug(f"{query_func.__name__}(): the '{e}' exceptio raised")
        time.sleep(1)
    if expected_rcode is not None:
        last_rcode = dns_rcode.to_text(res.rcode()) if res else None
        isctest.log.debug(
            f"{query_func.__name__}(): expected rcode={dns_rcode.to_text(expected_rcode)}, last rcode={last_rcode}"
        )
    raise dns.exception.Timeout


def udp(*args, **kwargs) -> Any:
    return generic_query(dns.query.udp, *args, **kwargs)


def tcp(*args, **kwargs) -> Any:
    return generic_query(dns.query.tcp, *args, **kwargs)
