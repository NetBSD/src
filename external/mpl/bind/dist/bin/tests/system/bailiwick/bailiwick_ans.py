"""
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
"""

from typing import Dict, List, Optional, Type

import abc

import dns.name
import dns.rcode
import dns.rdatatype

from isctest.asyncserver import (
    ControlCommand,
    ControllableAsyncDnsServer,
    DnsProtocol,
    QueryContext,
    ResponseHandler,
)


class ResponseSpoofer(ResponseHandler, abc.ABC):

    spoofers: Dict[str, Type["ResponseSpoofer"]] = {}

    def __init_subclass__(cls, mode: str) -> None:
        assert mode not in cls.spoofers
        cls.spoofers[mode] = cls

    @classmethod
    def get_spoofer(cls, mode: str) -> Optional["ResponseSpoofer"]:
        try:
            return cls.spoofers[mode]()
        except KeyError:
            return None

    @property
    @abc.abstractmethod
    def qname(self) -> str:
        raise NotImplementedError

    def match(self, qctx: QueryContext) -> bool:
        return (
            qctx.qname == dns.name.from_text(self.qname)
            and qctx.qtype == dns.rdatatype.TXT
            and qctx.protocol == DnsProtocol.UDP
        )


class SetSpoofingModeCommand(ControlCommand):
    """
    Select the ResponseSpoofer to use while handling queries from the resolver
    under test (ns4).  This control command is used at the start of each test
    function in tests_bailiwick.py.
    """

    control_subdomain = "set-spoofing-mode"

    def __init__(self) -> None:
        self._current_handler: Optional[ResponseSpoofer] = None

    def handle(
        self, args: List[str], server: ControllableAsyncDnsServer, qctx: QueryContext
    ) -> Optional[str]:
        if len(args) != 1:
            qctx.response.set_rcode(dns.rcode.SERVFAIL)
            return "invalid control command"

        mode = args[0]

        if mode == "none":
            if self._current_handler:
                server.uninstall_response_handler(self._current_handler)
                self._current_handler = None
            return "response spoofing disabled"

        spoofer = ResponseSpoofer.get_spoofer(mode)
        if not spoofer:
            qctx.response.set_rcode(dns.rcode.SERVFAIL)
            return f"unknown spoofing mode {mode}"

        if self._current_handler:
            server.uninstall_response_handler(self._current_handler)
        server.install_response_handler(spoofer)
        self._current_handler = spoofer

        return f"response spoofing enabled (mode: {mode})"


def spoofing_server() -> ControllableAsyncDnsServer:
    server = ControllableAsyncDnsServer(default_rcode=dns.rcode.NOERROR)
    server.install_control_command(SetSpoofingModeCommand())
    return server
