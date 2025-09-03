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

import logging
import os
from pathlib import Path
import re

import dns.message
import dns.rcode

from .log import debug, info, LogFile, WatchLogFromStart, WatchLogFromHere
from .rndc import RNDCBinaryExecutor, RNDCException, RNDCExecutor
from .run import perl
from .query import udp


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
        rndc_logger: Optional[logging.Logger] = None,
        rndc_executor: Optional[RNDCExecutor] = None,
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

        `rndc_logger` is the `logging.Logger` to use for logging RNDC
        commands sent to this `named` instance.

        `rndc_executor` is an object implementing the `RNDCExecutor` interface
        that is used for executing RNDC commands on this `named` instance.
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
        self.log = LogFile(os.path.join(identifier, "named.run"))
        self._rndc_executor = rndc_executor or RNDCBinaryExecutor()
        self._rndc_logger = rndc_logger

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

    def rndc(self, command: str, ignore_errors: bool = False, log: bool = True) -> str:
        """
        Send `command` to this named instance using RNDC.  Return the server's
        response.

        If the RNDC command fails, an `RNDCException` is raised unless
        `ignore_errors` is set to `True`.

        The RNDC command will be logged to `rndc.log` (along with the server's
        response) unless `log` is set to `False`.

        >>> # Instances of the `NamedInstance` class are expected to be passed
        >>> # to pytest tests as fixtures; here, some instances are created
        >>> # directly (with a fake RNDC executor) so that doctest can work.
        >>> import unittest.mock
        >>> mock_rndc_executor = unittest.mock.Mock()
        >>> ns1 = NamedInstance("ns1", rndc_executor=mock_rndc_executor)
        >>> ns2 = NamedInstance("ns2", rndc_executor=mock_rndc_executor)
        >>> ns3 = NamedInstance("ns3", rndc_executor=mock_rndc_executor)
        >>> ns4 = NamedInstance("ns4", rndc_executor=mock_rndc_executor)

        >>> # Send the "status" command to ns1.  An `RNDCException` will be
        >>> # raised if the RNDC command fails.  This command will be logged.
        >>> response = ns1.rndc("status")

        >>> # Send the "thaw foo" command to ns2.  No exception will be raised
        >>> # in case the RNDC command fails.  This command will be logged
        >>> # (even if it fails).
        >>> response = ns2.rndc("thaw foo", ignore_errors=True)

        >>> # Send the "stop" command to ns3.  An `RNDCException` will be
        >>> # raised if the RNDC command fails, but this command will not be
        >>> # logged (the server's response will still be returned to the
        >>> # caller, though).
        >>> response = ns3.rndc("stop", log=False)

        >>> # Send the "halt" command to ns4 in "fire & forget mode": no
        >>> # exceptions will be raised and no logging will take place (the
        >>> # server's response will still be returned to the caller, though).
        >>> response = ns4.rndc("stop", ignore_errors=True, log=False)
        """
        try:
            response = self._rndc_executor.call(self.ip, self.ports.rndc, command)
            if log:
                self._rndc_log(command, response)
        except RNDCException as exc:
            response = str(exc)
            if log:
                self._rndc_log(command, response)
            if not ignore_errors:
                raise

        return response

    def nsupdate(self, update_msg: dns.message.Message):
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
                expected_rcode=dns.rcode.NOERROR,
            )
        except dns.exception.Timeout as exc:
            msg = f"update timeout for {zone}"
            raise dns.exception.Timeout(msg) from exc
        debug(f"update of zone {zone} to server {self.ip} successful")
        return response

    def watch_log_from_start(self) -> WatchLogFromStart:
        """
        Return an instance of the `WatchLogFromStart` context manager for this
        `named` instance's log file.
        """
        return WatchLogFromStart(self.log.path)

    def watch_log_from_here(self) -> WatchLogFromHere:
        """
        Return an instance of the `WatchLogFromHere` context manager for this
        `named` instance's log file.
        """
        return WatchLogFromHere(self.log.path)

    def reconfigure(self) -> None:
        """
        Reconfigure this named `instance` and wait until reconfiguration is
        finished.  Raise an `RNDCException` if reconfiguration fails.
        """
        with self.watch_log_from_here() as watcher:
            self.rndc("reconfig")
            watcher.wait_for_line("any newly configured zones are now loaded")

    def _rndc_log(self, command: str, response: str) -> None:
        """
        Log an `rndc` invocation (and its output) to the `rndc.log` file in the
        current working directory.
        """
        fmt = '%(ip)s: "%(command)s"\n%(separator)s\n%(response)s%(separator)s'
        args = {
            "ip": self.ip,
            "command": command,
            "separator": "-" * 80,
            "response": response,
        }
        if self._rndc_logger is None:
            info(fmt, args)
        else:
            self._rndc_logger.info(fmt, args)

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
