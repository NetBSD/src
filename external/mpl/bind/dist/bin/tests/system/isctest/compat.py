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

from typing import Any, TYPE_CHECKING

import dns.edns
import dns.rcode

# compatiblity with dnspython<2.0.0
try:
    # In dnspython>=2.0.0, dns.rcode.Rcode class is available
    # pylint: disable=invalid-name
    dns_rcode = dns.rcode.Rcode  # type: Any
except AttributeError:
    # In dnspython<2.0.0, selected rcodes are available as integers directly
    # from dns.rcode
    dns_rcode = dns.rcode


if TYPE_CHECKING:
    EDECode = dns.edns.EDECode
    EDEOption = dns.edns.EDEOption
else:
    try:  # compatiblity with dnspython<2.2.0
        EDECode = dns.edns.EDECode
    except AttributeError:
        # In dnspython<2.2.0, the dns.edns.EDECode doesn't exist.
        #
        # The primary use-case is for us to use existing EDECode objects from the
        # class, e.g. EDECode.FILTERED. To mimick this behavior, use a string
        # factory that just turns the attribute name into a string.
        #
        # The used compatibility hack doesn't really matter (as long as EDECode.xxx
        # doesn't raise exception), as with dnspython versions prior to 2.2.0, any
        # EDE checking will be skipped anyway.
        class _CompatEDECode:
            def __getattr__(self, name: str) -> str:
                return name

        EDECode = _CompatEDECode()
    try:
        EDEOption = dns.edns.EDEOption
    except AttributeError:
        # In dnspython<2.2.0, the dns.edns.EDEOption doesn't exist, so we stub it to be
        # able to use it in type annotations.
        class EDEOption:
            def __new__(cls, *args, **kwargs):
                raise RuntimeError("Using EDEOption requires dnspython>=2.2.0")


# pylint: disable=unused-import
try:
    from dns.dnssec import DSDigest
except ImportError:  # dnspython<2.0.0
    import enum

    class DSDigest(enum.IntEnum):  # type: ignore
        """DNSSEC Delgation Signer Digest Algorithm"""

        SHA1 = 1
        SHA256 = 2
        SHA384 = 4
