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

"""Handler for the sibling-ds. zone.

When returning a referral for child.sibling-ds, this server injects a DS
record for sibling.sibling-ds into the authority section.  The resolver
should reject this because the DS owner name does not match the
delegation (NS) name.
"""

from collections.abc import AsyncGenerator

import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)


class SiblingDsInjectionHandler(DomainHandler):
    """Inject a DS record for sibling.sibling-ds into child.sibling-ds referrals."""

    domains = ["child.sibling-ds."]

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        # The default zone-data response already has the NS delegation for
        # child.sibling-ds. and glue.  Add a DS record for the *sibling* zone
        # (wrong name for this referral).
        sibling_ds = dns.rrset.from_text(
            "sibling.sibling-ds.",
            300,
            qctx.qclass,
            dns.rdatatype.DS,
            "12345 8 2 "
            "49FD46E6C4B45C55D4AC69CBD3CD34AC1AFE51DE7B2B585ABCDEABCDEABCDEAB",
        )
        qctx.response.authority.append(sibling_ds)
        yield DnsResponseSend(qctx.response)
