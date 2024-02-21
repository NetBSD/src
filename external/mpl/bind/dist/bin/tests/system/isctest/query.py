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
from typing import Optional

import dns.query
import dns.message


QUERY_TIMEOUT = 10


def udp(
    message: dns.message.Message, ip: str, port: Optional[int] = None
) -> dns.message.Message:
    if port is None:
        port = int(os.environ["PORT"])
    return dns.query.udp(message, ip, QUERY_TIMEOUT, port=port)


def tcp(
    message: dns.message.Message, ip: str, port: Optional[int] = None
) -> dns.message.Message:
    if port is None:
        port = int(os.environ["PORT"])
    return dns.query.tcp(message, ip, QUERY_TIMEOUT, port=port)
