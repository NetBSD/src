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

from typing import Any

import dns.rcode
import dns.message


# compatiblity with dnspython<2.0.0
try:
    # In dnspython>=2.0.0, dns.rcode.Rcode class is available
    # pylint: disable=invalid-name
    dns_rcode = dns.rcode.Rcode  # type: Any
except AttributeError:
    # In dnspython<2.0.0, selected rcodes are available as integers directly
    # from dns.rcode
    dns_rcode = dns.rcode


def rcode(message: dns.message.Message, expected_rcode) -> None:
    assert message.rcode() == expected_rcode, str(message)


def noerror(message: dns.message.Message) -> None:
    rcode(message, dns_rcode.NOERROR)
