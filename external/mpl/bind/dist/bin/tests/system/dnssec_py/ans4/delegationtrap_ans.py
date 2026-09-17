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

"""Handler for the attack.delegationtrap. zone (DelegationTrap reproducer).

This is a Python port of the DelegationTrap vector from the ReTrap PoC
(gitlab.isc.org/bind-team/nankai-cve-reproducers).  It reproduces the
"deep delegation hierarchy" algorithmic-complexity attack described in
issue #5347 (part of the #5341 meta-issue).

The server is authoritative for the whole attack.delegationtrap. subtree
and pretends that *every* label below the apex is its own secure zone,
all sharing a single reused key.  For any name N under the apex it
answers:

  N/DNSKEY -> DNSKEY(K)                signed by N          (self-signed)
  N/DS     -> DS(K, owner=N)           signed by parent(N)
  N/A      -> A 10.53.0.4              signed by parent(N)

Because the A answer is returned directly (no referral) but its RRSIG
signer is the immediate parent, a validating resolver must build the
whole chain of trust label by label: for a query with a depth-D name it
fetches and validates DNSKEY + DS at each of the D levels.  This is the
"chain-of-trust construction" cost the attack amplifies, and it is what
BIND's per-fetch validation quota (max-validations-per-fetch) is meant
to bound.

Key material is written by bootstrap() in tests_delegationtrap.py to
attack_delegationtrap.pem in this directory before any server starts.
"""

from collections.abc import AsyncGenerator
from pathlib import Path

import time

from cryptography.hazmat.primitives import serialization
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.name
import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdatatype
import dns.rrset

from isctest.asyncserver import (
    DnsResponseSend,
    DomainHandler,
    QueryContext,
    ResponseAction,
)

ZONE_NAME = "attack.delegationtrap."
SERVER_IP = "10.53.0.4"
TTL = 300
PEM_PATH = Path("attack_delegationtrap.pem")


class DelegationTrapHandler(DomainHandler):
    """Serve every label under attack.delegationtrap. as a secure zone cut."""

    domains = [ZONE_NAME]

    def __init__(self) -> None:
        super().__init__()
        self._apex = dns.name.from_text(ZONE_NAME)

        self._priv = serialization.load_pem_private_key(
            PEM_PATH.read_bytes(), password=None
        )
        self._dnskey = dns.dnssec.make_dnskey(
            self._priv.public_key(),
            dns.dnssec.Algorithm.ECDSAP256SHA256,
            flags=Flag.ZONE | Flag.SEP,
        )

        now = int(time.time())
        self._inception = now - 3600
        self._expiration = now + 14 * 86400

    def _sign(self, rrset: dns.rrset.RRset, signer: dns.name.Name) -> dns.rrset.RRset:
        """Return an RRSIG RRset covering `rrset`, signed as zone `signer`."""
        rrsig = dns.dnssec.sign(
            rrset,
            self._priv,
            signer=signer,
            dnskey=self._dnskey,
            inception=self._inception,
            expiration=self._expiration,
            lifetime=None,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )
        rrsig_rrset = dns.rrset.RRset(
            rrset.name, rrset.rdclass, dns.rdatatype.RRSIG, rrset.rdtype
        )
        rrsig_rrset.update_ttl(TTL)
        rrsig_rrset.add(rrsig)
        return rrsig_rrset

    def _dnskey_rrset(self, name: dns.name.Name) -> dns.rrset.RRset:
        rrset = dns.rrset.RRset(name, dns.rdataclass.IN, dns.rdatatype.DNSKEY)
        rrset.update_ttl(TTL)
        rrset.add(self._dnskey)
        return rrset

    def _ds_rrset(self, name: dns.name.Name) -> dns.rrset.RRset:
        ds = dns.dnssec.make_ds(name, self._dnskey, dns.dnssec.DSDigest.SHA256)
        rrset = dns.rrset.RRset(name, dns.rdataclass.IN, dns.rdatatype.DS)
        rrset.update_ttl(TTL)
        rrset.add(ds)
        return rrset

    def _a_rrset(self, name: dns.name.Name) -> dns.rrset.RRset:
        rrset = dns.rrset.RRset(name, dns.rdataclass.IN, dns.rdatatype.A)
        rrset.update_ttl(TTL)
        rrset.add(dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.A, SERVER_IP))
        return rrset

    async def get_responses(
        self, qctx: QueryContext
    ) -> AsyncGenerator[ResponseAction, None]:
        qname = qctx.qname
        qtype = qctx.qtype
        response = qctx.prepare_new_response(with_zone_data=False)
        response.set_rcode(dns.rcode.NOERROR)

        parent = qname.parent() if qname != dns.name.root else qname

        if qtype == dns.rdatatype.DNSKEY:
            # The DNSKEY RRset is signed by the zone itself (self-signed KSK).
            rrset = self._dnskey_rrset(qname)
            response.answer.extend([rrset, self._sign(rrset, qname)])
        elif qtype == dns.rdatatype.DS:
            # A DS lives in the parent zone and is signed by the parent.
            rrset = self._ds_rrset(qname)
            response.answer.extend([rrset, self._sign(rrset, parent)])
        elif qtype == dns.rdatatype.A:
            # The A answer is signed by the immediate parent, forcing the
            # resolver to build the trust chain label by label.
            rrset = self._a_rrset(qname)
            response.answer.extend([rrset, self._sign(rrset, parent)])

        yield DnsResponseSend(response, authoritative=True)
