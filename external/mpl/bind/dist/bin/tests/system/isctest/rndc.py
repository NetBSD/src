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

"""
This module implements the RNDC control protocol.
"""

from typing import Any

import base64
import hashlib
import hmac
import random
import socket
import struct
import time

from .text import Text


class RNDCException(Exception):
    """
    Raised when an RNDC command returns a non-zero result code.
    """

    def __init__(self, result: "RNDCResult") -> None:
        super().__init__(f'rndc command failed with result {result.rc}: "{result.err}"')
        self.result = result


class RNDCProtocolError(Exception):
    """
    Raised when the control channel yields a truncated, malformed, or
    unauthenticated response.
    """


class RNDCResult:
    """
    Result of an RNDC command; mirrors isctest.run.CmdResult.
    """

    def __init__(self, response: dict[str, str]) -> None:
        self.response = response
        self.rc = int(response.get("result", "0"))
        self.out = Text(response.get("text", ""))
        self.err = Text(response.get("err", ""))


class RNDCClient:
    """
    RNDC protocol client.

    A pure-Python alternative to controlling a server with the rndc
    binary (`NamedInstance.rndc()`), useful when the overhead of
    spawning the binary for every command is undesirable. Exercising
    the rndc binary itself remains the primary interface in tests.
    """

    _algos = {
        "md5": 157,
        "sha1": 161,
        "sha224": 162,
        "sha256": 163,
        "sha384": 164,
        "sha512": 165,
    }

    def __init__(
        self,
        ip: str,
        port: int,
        algo: str = "sha256",
        secret: str = "1234abcd8765",
        timeout: float = 10,
    ) -> None:
        """
        Creates a persistent connection to the control channel and logs in.

        algo - HMAC algorithm, one of md5, sha1, sha224, sha256, sha384, sha512
        secret - HMAC secret, base64 encoded

        The `algo` and `secret` defaults match _common/rndc.key, which
        virtually all named instances in the system tests use for their
        control channel.
        """
        self.algo = algo
        self.hlalgo = getattr(hashlib, algo)
        self.secret = base64.b64decode(secret)
        self.ser = random.getrandbits(32)
        self.nonce: bytes | None = None
        self.socket = socket.create_connection((ip, port), timeout=timeout)
        try:
            self._login()
        except (OSError, RNDCProtocolError):
            self.socket.close()
            raise

    def __enter__(self) -> "RNDCClient":
        return self

    def __exit__(self, *_: Any) -> None:
        self.close()

    def close(self) -> None:
        self.socket.close()

    def call(self, command: str, *, raise_on_exception: bool = True) -> RNDCResult:
        """
        Call an RNDC command and check its result.
        """
        response = self._command({b"type": command.encode()})
        data = {k.decode(): v.decode() for k, v in response[b"_data"].items()}
        result = RNDCResult(data)
        if result.rc != 0 and raise_on_exception:
            raise RNDCException(result)
        return result

    def _serialize_dict(
        self, data: dict[bytes, Any], ignore_auth: bool = False
    ) -> bytes:
        rv = b""
        for k, v in data.items():
            if ignore_auth and k == b"_auth":
                continue
            rv += bytes([len(k)])
            rv += k
            if isinstance(v, bytes):
                rv += struct.pack(">BI", 1, len(v)) + v
            elif isinstance(v, dict):
                sd = self._serialize_dict(v)
                rv += struct.pack(">BI", 2, len(sd)) + sd
            else:
                raise NotImplementedError(f"Cannot serialize element of type {type(v)}")
        return rv

    def _prep_message(self, data: dict[bytes, Any]) -> bytes:
        self.ser = (self.ser + 1) & 0xFFFFFFFF
        now = int(time.time())

        d: dict[bytes, Any] = {}
        d[b"_auth"] = {}
        d[b"_ctrl"] = {}
        d[b"_ctrl"][b"_ser"] = b"%d" % self.ser
        d[b"_ctrl"][b"_tim"] = b"%d" % now
        d[b"_ctrl"][b"_exp"] = b"%d" % (now + 60)
        if self.nonce is not None:
            d[b"_ctrl"][b"_nonce"] = self.nonce
        d[b"_data"] = data

        msg = self._serialize_dict(d, ignore_auth=True)
        digest = hmac.new(self.secret, msg, self.hlalgo).digest()
        bhash = base64.b64encode(digest)
        if self.algo == "md5":
            d[b"_auth"][b"hmd5"] = struct.pack("22s", bhash)
        else:
            d[b"_auth"][b"hsha"] = struct.pack("B88s", self._algos[self.algo], bhash)
        msg = self._serialize_dict(d)
        msg = struct.pack(">II", len(msg) + 4, 1) + msg
        return msg

    def _verify_msg(self, msg: dict[bytes, Any]) -> bool:
        if self.nonce is not None and msg[b"_ctrl"][b"_nonce"] != self.nonce:
            return False
        bhash = msg[b"_auth"][b"hmd5" if self.algo == "md5" else b"hsha"]
        bhash += b"=" * (4 - (len(bhash) % 4))
        remote_hash = base64.b64decode(bhash)
        my_msg = self._serialize_dict(msg, ignore_auth=True)
        my_hash = hmac.new(self.secret, my_msg, self.hlalgo).digest()
        return my_hash == remote_hash

    def _recv_exact(self, length: int) -> bytes:
        # MSG_WAITALL would not help here: the socket timeout puts the
        # socket in non-blocking mode, where the kernel may return
        # partial data regardless of the flag.
        buf = b""
        while len(buf) < length:
            chunk = self.socket.recv(length - len(buf))
            if not chunk:
                raise RNDCProtocolError(
                    "connection closed mid-response; possible authentication failure"
                )
            buf += chunk
        return buf

    def _command(self, data: dict[bytes, Any]) -> dict[bytes, Any]:
        msg = self._prep_message(data)
        self.socket.sendall(msg)

        header = self._recv_exact(8)
        length, version = struct.unpack(">II", header)
        if version != 1:
            raise RNDCProtocolError(f"Unsupported message version {version}")

        # the length field also covers the 4-byte version word
        payload = self._recv_exact(length - 4)

        try:
            response = self._parse_dict(payload)
            verified = self._verify_msg(response)
        except (
            KeyError,
            IndexError,
            ValueError,
            struct.error,
            NotImplementedError,
        ) as exc:
            raise RNDCProtocolError(f"Malformed response ({exc})") from exc
        if not verified:
            raise RNDCProtocolError("HMAC verification of the response failed")

        return response

    def _login(self) -> None:
        self.nonce = None
        msg = self._command({b"type": b"null"})
        try:
            self.nonce = msg[b"_ctrl"][b"_nonce"]
        except KeyError as exc:
            raise RNDCProtocolError("Login response is missing a nonce") from exc

    def _parse_element(self, buf: bytes) -> tuple[bytes, Any, bytes]:
        pos = 0
        labellen = buf[pos]
        pos += 1
        label = buf[pos : pos + labellen]
        pos += labellen
        etype = buf[pos]
        pos += 1
        datalen = struct.unpack(">I", buf[pos : pos + 4])[0]
        pos += 4
        data = buf[pos : pos + datalen]
        pos += datalen
        rest = buf[pos:]

        if etype == 1:  # raw binary value
            return label, data, rest
        if etype == 2:  # dictionary
            return label, self._parse_dict(data), rest
        # element type 3 (list) is not implemented
        raise NotImplementedError(f"Unknown element type {etype}")

    def _parse_dict(self, buf: bytes) -> dict[bytes, Any]:
        rv: dict[bytes, Any] = {}
        while len(buf) > 0:
            label, value, buf = self._parse_element(buf)
            rv[label] = value
        return rv
