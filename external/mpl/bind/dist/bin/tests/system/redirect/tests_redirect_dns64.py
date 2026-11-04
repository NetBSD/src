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

import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "ns1/K*",
        "ns1/*.signed",
        "ns1/dsset-nsec3.",
        "ns1/dsset-signed.",
        "ns1/nsec3.db",
        "ns1/signed.db",
        "ns2/example.db",
        "ns2/named.stats",
        "ns2/redirect.db",
        "ns3/K*",
        "ns3/*.signed",
        "ns3/dsset-nsec3.",
        "ns3/dsset-signed.",
        "ns3/nsec3.db",
        "ns3/signed.db",
        "ns4/example.db",
        "ns4/named.stats",
        "ns5/K*",
        "ns5/dsset-*",
        "ns5/*.signed",
        "ns5/root.db",
        "ns5/sign.ns5.*",
        "ns5/signed.db",
        "ns6/signed.db.signed",
        "ns9/K*",
        "ns9/dsset-*",
        "ns9/root.db",
        "ns9/root.db.signed",
        "ns9/sign.ns9.*",
        "ns10/trusted.conf",
    ]
)


def _no_crash(server, qname):
    # DO=0 so nxdomain-redirect is not skipped on validated upstream
    # responses; the documented landing-page deployment serves
    # non-DNSSEC clients.
    msg = isctest.query.create(qname, "AAAA", dnssec=False)
    response = isctest.query.tcp(msg, server.ip)
    isctest.check.noerror(response)


def _alive(server):
    msg = isctest.query.create("ns.redirect.", "A", dnssec=False)
    response = isctest.query.tcp(msg, server.ip)
    isctest.check.noerror(response)


def test_nxdomain_redirect_dns64_authoritative(ns7):
    # Direct AAAA to a server that is authoritative for both '.' (NXDOMAIN)
    # and the redirect zone (wildcard A only). Reproduces the
    # INSIST(!qctx->is_zone) abort in query_notfound() entered via
    # authoritative NXDOMAIN.
    _no_crash(ns7, "no-exist.")
    _alive(ns7)


def test_nxdomain_redirect_dns64_recursive(ns8):
    # Recursive resolver: the upstream returns a real NOERROR-empty AAAA
    # for '*.redirect.', which the resolver caches as NCACHENXRRSET.
    # Reproduces the REQUIRE(rdataset->methods != NULL) abort in
    # dns_rdataset_first() reached via the disassociated rdataset on the
    # second pass through redirect2().
    _no_crash(ns8, "no-exist.")
    _alive(ns8)


def test_nxdomain_redirect_dns64_synth_from_dnssec(ns10):
    # Validating recursor with synth-from-dnssec. Prime the NSEC chain
    # with an A query (the redirect zone serves a wildcard A, so this
    # path returns successfully without entering the DNS64 fallback). A
    # subsequent AAAA query for a different nonexistent name is then
    # synthesized via query_coveringnsec() and reaches query_redirect()
    # through the third documented entry. Same downstream bug as the
    # authoritative path.
    msg = isctest.query.create("prime.", "A", dnssec=False)
    response = isctest.query.tcp(msg, ns10.ip)
    isctest.check.noerror(response)

    _no_crash(ns10, "trigger.")
    _alive(ns10)
