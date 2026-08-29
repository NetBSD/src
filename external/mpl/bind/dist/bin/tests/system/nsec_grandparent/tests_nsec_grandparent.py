#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import json

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

import dns.dnssec
import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

import isctest
import isctest.mark

PARENT = "p031.test."
CHILD = f"c.{PARENT}"
GRANDCHILD = f"grand.{CHILD}"
GRANDCHILD3 = f"grand3.{CHILD}"
ATTACK = f"www-bind.{GRANDCHILD}"
ATTACK3 = f"www-bind.{GRANDCHILD3}"
ATTACK_CACHED = f"www2-bind.{GRANDCHILD}"
FORGED_A = "6.6.6.60"
AUTH = "10.53.0.1"  # ans1, the attacker-controlled authoritative server
RESOLVER = "10.53.0.2"  # ns2, the validating resolver under test

REFUSED_NSEC_LOG = (
    "is_insecure_referral: NSEC signer above known secure DS; "
    "refusing insecure-delegation proof"
)
REFUSED_NSEC3_LOG = (
    "is_insecure_referral: NSEC3 signer above known secure DS; "
    "refusing insecure-delegation proof"
)

pytestmark = [
    isctest.mark.with_ecdsa_deterministic,
    pytest.mark.extra_artifacts(
        [
            "ans*/ans.run",
            "ans*/keys.json",
        ]
    ),
]


def _make_key():
    private_key = ec.generate_private_key(ec.SECP256R1())
    dnskey = dns.dnssec.make_dnskey(
        private_key.public_key(),
        algorithm="ECDSAP256SHA256",
        flags=257,
    )
    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.PKCS8,
        encryption_algorithm=serialization.NoEncryption(),
    ).decode("ascii")
    return {
        "private_pem": private_pem,
        "dnskey": dnskey.to_text(),
    }


def bootstrap():
    keys = {PARENT: _make_key()}
    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")
    parent_dnskey = "".join(keys[PARENT]["dnskey"].split()[3:])
    return {"PARENT_DNSKEY": parent_dnskey}


def _query(server, qname, qtype):
    query = isctest.query.create(qname, qtype)
    return isctest.query.tcp(query, server)


def _rrset(response, section, owner, rdtype, covers=None):
    if covers is None:
        return response.get_rrset(
            section,
            dns.name.from_text(owner),
            dns.rdataclass.IN,
            rdtype,
        )
    return response.get_rrset(
        section,
        dns.name.from_text(owner),
        dns.rdataclass.IN,
        rdtype,
        covers=covers,
    )


def _has_a(response, section, owner, address):
    rrset = _rrset(response, section, owner, dns.rdatatype.A)
    return rrset is not None and any(rdata.address == address for rdata in rrset)


def _check_signed_rrset(response, section, owner, rdtype, signer):
    rrsig = _rrset(
        response,
        section,
        owner,
        dns.rdatatype.RRSIG,
        covers=rdtype,
    )
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(signer), response.to_text()


def _auth_query_count(qname, qtype):
    """Number of times the mock auth server received qname/qtype."""
    log = Path("ans1/ans.run").read_text(encoding="utf-8")
    return log.count(f"Received {qname.rstrip('.')}/IN/{qtype} (ID=")


def _check_no_downgrade(response, qname):
    """The forged proof must not downgrade the signed namespace."""
    isctest.check.servfail(response)
    isctest.check.noadflag(response)
    assert not _has_a(response, response.answer, qname, FORGED_A), response.to_text()


def _check_refusal_logged(server, qname, expected_log):
    """
    Check that the validator logged why it refused the forged proof.

    Call this after the query has returned, never around it:
    watch_log_from_start() rescans named.run from the beginning, so the
    line is already there, and an assertion that fails inside a WatchLog
    context manager is masked on the way out by the "wait_for_*() was not
    called" exception __exit__ raises.  Keeping _check_no_downgrade()
    outside the block lets a real regression report the forged answer
    rather than a missing log line.
    """
    with server.watch_log_from_start() as watcher:
        watcher.wait_for_line(f"validating {qname.rstrip('.')}/A: {expected_log}")


def test_auth_serves_forged_grandparent_nsec():
    """
    Check the attacker's server, not the resolver.

    This queries ans1 directly, so ns2's validator never sees it and the
    test passes whether or not BIND rejects the forgery -- it is not a
    reproducer for #5967.  It guards the premise the reproducers below
    rest on: that c.p031.test is a secure delegation, and that a DS query
    for grand.c.p031.test is answered with an NSEC signed by the
    grandparent p031.test rather than by the real parent c.p031.test.  If
    ans1/ans.py ever stops serving that forgery, this fails here instead
    of quietly turning every test_resolver_rejects_* below into a pass.
    """
    child_ds = _query(AUTH, CHILD, "DS")
    isctest.check.noerror(child_ds)
    assert _rrset(child_ds, child_ds.answer, CHILD, dns.rdatatype.DS) is not None
    _check_signed_rrset(child_ds, child_ds.answer, CHILD, dns.rdatatype.DS, PARENT)

    grandchild_ds = _query(AUTH, GRANDCHILD, "DS")
    isctest.check.noerror(grandchild_ds)
    nsec = _rrset(
        grandchild_ds, grandchild_ds.authority, GRANDCHILD, dns.rdatatype.NSEC
    )
    assert nsec is not None, grandchild_ds.to_text()
    assert nsec[0].next == dns.name.from_text(
        f"grandz.{CHILD}"
    ), grandchild_ds.to_text()
    _check_signed_rrset(
        grandchild_ds,
        grandchild_ds.authority,
        GRANDCHILD,
        dns.rdatatype.NSEC,
        PARENT,
    )


def test_resolver_rejects_grandparent_nsec_downgrade(servers):
    """
    Reproducer for #5967: an NSEC signed by the grandparent must not
    downgrade a secure delegation to insecure.  Here the forged proof
    arrives in a fresh DS fetch, so the refusal runs in
    fetch_callback_ds().
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK, "A"), ATTACK)
    _check_refusal_logged(servers["ns2"], ATTACK, REFUSED_NSEC_LOG)

    # The rejected proof must also stop the insecurity walk: without the
    # early stop, the validator descends and asks the (attacker-controlled)
    # server for a DS at the attack name.
    assert _auth_query_count(ATTACK, "DS") == 0


def test_resolver_rejects_grandparent_nsec3_downgrade(servers):
    """
    The same downgrade as above, with the forgery expressed as an NSEC3,
    which reaches is_insecure_referral()'s trynsec3 arm instead.
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK3, "A"), ATTACK3)
    _check_refusal_logged(servers["ns2"], ATTACK3, REFUSED_NSEC3_LOG)

    assert _auth_query_count(ATTACK3, "DS") == 0


def test_resolver_rejects_downgrade_from_cached_proof(servers):
    """
    The same downgrade as above, with the forged proof already in the
    cache when the insecurity walk reaches it, so the refusal runs in
    seek_ds() rather than in fetch_callback_ds().
    """
    # Prime the cache: walking the insecurity proof for ATTACK fetches the
    # forged NODATA proof for GRANDCHILD/DS, which is validated and cached
    # independently of the failed A validation.
    isctest.check.servfail(_query(RESOLVER, ATTACK, "A"))

    ds_fetches = _auth_query_count(GRANDCHILD, "DS")
    _check_no_downgrade(_query(RESOLVER, ATTACK_CACHED, "A"), ATTACK_CACHED)
    _check_refusal_logged(servers["ns2"], ATTACK_CACHED, REFUSED_NSEC_LOG)

    # No new DS fetch for GRANDCHILD confirms the proof really did come from
    # the cache; none for ATTACK_CACHED confirms the walk stopped there.
    assert _auth_query_count(GRANDCHILD, "DS") == ds_fetches
    assert _auth_query_count(ATTACK_CACHED, "DS") == 0
