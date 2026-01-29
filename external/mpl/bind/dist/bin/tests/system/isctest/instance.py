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

from typing import List, NamedTuple, Optional

import os
from pathlib import Path
import re

import dns.message
import dns.rcode

from .log import debug, WatchLogFromStart, WatchLogFromHere
from .run import CmdResult, EnvCmd, perl
from .query import udp
from .text import TextFile


class NamedPorts(NamedTuple):
    dns: int = 53
    rndc: int = 953

    @staticmethod
    def from_env():
        return NamedPorts(
            dns=int(os.environ["PORT"]),
            rndc=int(os.environ["CONTROLPORT"]),
        )


class NamedInstance:
    """
    A class representing a `named` instance used in a system test.

    This class is expected to be instantiated as part of the `servers` fixture:

    ```python
    def test_foo(servers):
        servers["ns1"].rndc("status")
    ```
    """

    def __init__(
        self,
        identifier: str,
        num: Optional[int] = None,
        ports: Optional[NamedPorts] = None,
    ) -> None:
        """
        `identifier` is the name of the instance's directory

        `num` is optional if the identifier is in a form of `ns<X>`, in which
        case `<X>` is assumed to be numeric identifier; otherwise it must be
        provided to assign a numeric identification to the server

        `ports` is the `NamedPorts` instance listing the UDP/TCP ports on which
        this `named` instance is listening for various types of traffic (both
        DNS traffic and RNDC commands). Defaults to ports set by the test
        framework.
        """
        self.directory = Path(identifier).absolute()
        if not self.directory.is_dir():
            raise ValueError(f"{self.directory} isn't a directory")
        self.system_test_name = self.directory.parent.name

        self.identifier = identifier
        self.num = self._identifier_to_num(identifier, num)
        if ports is None:
            ports = NamedPorts.from_env()
        self.ports = ports
        self.log = TextFile(os.path.join(identifier, "named.run"))

        self._rndc_conf = Path("../_common/rndc.conf").absolute()
        self._rndc = EnvCmd("RNDC", self.rndc_args)

    @property
    def rndc_args(self) -> str:
        """Base arguments for calling RNDC to control the instance."""
        return f"-c {self._rndc_conf} -s {self.ip} -p {self.ports.rndc}"

    @property
    def ip(self) -> str:
        """IPv4 address of the instance."""
        return f"10.53.0.{self.num}"

    @staticmethod
    def _identifier_to_num(identifier: str, num: Optional[int] = None) -> int:
        regex_match = re.match(r"^ns(?P<index>[0-9]{1,2})$", identifier)
        if not regex_match:
            if num is None:
                raise ValueError(f'Can\'t parse numeric identifier from "{identifier}"')
            return num
        parsed_num = int(regex_match.group("index"))
        assert num is None or num == parsed_num, "mismatched num and identifier"
        return parsed_num

    def rndc(self, command: str, timeout=10, **kwargs) -> CmdResult:
        """
        Send `command` to this named instance using RNDC.  Return the server's
        response.

        To suppress exceptions, redirect outputs, control logging change
        timeout etc. use keyword arguments which are passed to
        isctest.cmd.run().
        """
        return self._rndc(command, timeout=timeout, **kwargs)

    def nsupdate(
        self, update_msg: dns.message.Message, expected_rcode=dns.rcode.NOERROR
    ):
        """
        Issue a dynamic update to a server's zone.
        """
        # FUTURE update_msg is actually dns.update.UpdateMessage, but it not
        # typed properly here in order to support use of this module with
        # dnspython<2.0.0
        zone = str(update_msg.zone[0].name)  # type: ignore[attr-defined]
        try:
            response = udp(
                update_msg,
                self.ip,
                self.ports.dns,
                timeout=3,
                expected_rcode=expected_rcode,
            )
        except dns.exception.Timeout as exc:
            msg = f"update timeout for {zone}"
            raise dns.exception.Timeout(msg) from exc
        debug(
            f"update of zone {zone} to server {self.ip} finished with {expected_rcode}"
        )
        return response

    def watch_log_from_start(
        self, timeout: float = WatchLogFromStart.DEFAULT_TIMEOUT
    ) -> WatchLogFromStart:
        """
        Return an instance of the `WatchLogFromStart` context manager for this
        `named` instance's log file.
        """
        return WatchLogFromStart(self.log.path, timeout)

    def watch_log_from_here(
        self, timeout: float = WatchLogFromHere.DEFAULT_TIMEOUT
    ) -> WatchLogFromHere:
        """
        Return an instance of the `WatchLogFromHere` context manager for this
        `named` instance's log file.
        """
        return WatchLogFromHere(self.log.path, timeout)

    def reconfigure(self, **kwargs) -> CmdResult:
        """
        Reconfigure this named `instance` and wait until reconfiguration is
        finished.
        """
        with self.watch_log_from_here() as watcher:
            cmd = self.rndc("reconfig", **kwargs)
            watcher.wait_for_line("any newly configured zones are now loaded")
        return cmd

    def stop(self, args: Optional[List[str]] = None) -> None:
        """Stop the instance."""
        args = args or []
        perl(
            f"{os.environ['srcdir']}/stop.pl",
            [self.system_test_name, self.identifier] + args,
        )

    def start(self, args: Optional[List[str]] = None) -> None:
        """Start the instance."""
        args = args or []
        perl(
            f"{os.environ['srcdir']}/start.pl",
            [self.system_test_name, self.identifier] + args,
        )

    def __repr__(self):
        return self.identifier
