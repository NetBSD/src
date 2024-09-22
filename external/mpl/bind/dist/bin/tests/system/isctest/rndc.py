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

import abc
import os
import subprocess


# pylint: disable=too-few-public-methods
class RNDCExecutor(abc.ABC):
    """
    An interface which RNDC executors have to implement in order for the
    `NamedInstance` class to be able to use them.
    """

    @abc.abstractmethod
    def call(self, ip: str, port: int, command: str) -> str:
        """
        Send RNDC `command` to the `named` instance at `ip:port` and return the
        server's response.
        """


class RNDCException(Exception):
    """
    Raised by classes implementing the `RNDCExecutor` interface when sending an
    RNDC command fails for any reason.
    """


class RNDCBinaryExecutor(RNDCExecutor):
    """
    An `RNDCExecutor` which sends RNDC commands to servers using the `rndc`
    binary.
    """

    def __init__(self) -> None:
        """
        This class needs the `RNDC` environment variable to be set to the path
        to the `rndc` binary to use.
        """
        rndc_path = os.environ.get("RNDC", "/bin/false")
        rndc_conf = os.path.join("..", "_common", "rndc.conf")
        self._base_cmdline = [rndc_path, "-c", rndc_conf]

    def call(self, ip: str, port: int, command: str) -> str:
        """
        Send RNDC `command` to the `named` instance at `ip:port` and return the
        server's response.
        """
        cmdline = self._base_cmdline[:]
        cmdline.extend(["-s", ip])
        cmdline.extend(["-p", str(port)])
        cmdline.extend(command.split())

        try:
            return subprocess.check_output(
                cmdline, stderr=subprocess.STDOUT, timeout=10, encoding="utf-8"
            )
        except subprocess.SubprocessError as exc:
            msg = getattr(exc, "output", "RNDC exception occurred")
            raise RNDCException(msg) from exc
