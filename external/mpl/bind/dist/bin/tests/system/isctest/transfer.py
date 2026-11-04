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

from re import Pattern

import re


def transfer_message(
    zone: str, source_ns: str | None, msg: str | Pattern, port: int | None = None
) -> Pattern:
    """Return the expected log message for an incoming zone transfer.

    Mirrors the format produced by xfrin_log() in lib/dns/xfrin.c:

        transfer of '<zone>/IN' from <source_ns>#<port>: <msg>

    Always returns a compiled Pattern.  When source_ns or port is None,
    the unknown part is replaced by a wildcard in the regex.

    Args:
        zone:      Zone name (without class, e.g. "example.com").
        source_ns: Source nameserver IP address string (e.g. "10.53.0.1"),
                   or None to match any source address.
        msg:       Transfer status or other message as a plain string
                   (e.g. "Transfer status: success"), which is regex-escaped,
                   or a compiled Pattern whose .pattern is spliced in as-is
                   for callers that need regex syntax in the message part.
        port:      Source port number, or None to match any port.
    """
    source_str = re.escape(source_ns) if source_ns is not None else ".*"
    port_str = str(port) if port is not None else "[0-9]+"
    msg_str = msg.pattern if isinstance(msg, Pattern) else re.escape(msg)

    return re.compile(
        re.escape(f"transfer of '{zone}/IN' from ")
        + f"{source_str}#{port_str}: "
        + msg_str
    )
