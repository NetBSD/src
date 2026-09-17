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
# The attacker-controlled sibling zone.
SIBLING = f"attacker.{PARENT}"
# #5967 (grandparent-zone NSEC/NSEC3): a grandchild whose forged NSEC/NSEC3
# insecure-delegation proof is owned by its grandparent zone.
GRANDCHILD = f"grand.{CHILD}"
GRANDCHILD3 = f"grand3.{CHILD}"
# #6234 (sibling-zone NSEC3): a grandchild whose forged NSEC3
# insecure-delegation proof is owned by an unrelated but correctly delegated
# and signed sibling zone.
GRANDCHILD3_SIBLING = f"grandsib.{CHILD}"
# #6321 (mixed-signer RRSIG): grandchildren whose grandparent-signed NSEC
# forgery also carries a dummy RRSIG naming the NSEC owner itself as signer,
# in either order relative to the genuine one.
GRANDCHILD_DUMMY_FIRST = f"grand-dummy-first.{CHILD}"
GRANDCHILD_DUMMY_LAST = f"grand-dummy-last.{CHILD}"
# RRSIG count cap: grandchildren whose grandparent-signed NSEC forgery carries
# as many same-signer RRSIGs as ns2 allows validations per fetch, or one fewer.
GRANDCHILD_TOO_MANY = f"grand-too-many.{CHILD}"
GRANDCHILD_ALMOST_TOO_MANY = f"grand-almost-too-many.{CHILD}"
# The names under attack.
ATTACK = f"www-bind.{GRANDCHILD}"
ATTACK3 = f"www-bind.{GRANDCHILD3}"
ATTACK3_SIBLING = f"www-bind.{GRANDCHILD3_SIBLING}"
ATTACK_CACHED = f"www2-bind.{GRANDCHILD}"
ATTACK_DUMMY_FIRST = f"www-bind.{GRANDCHILD_DUMMY_FIRST}"
ATTACK_DUMMY_FIRST_CACHED = f"www2-bind.{GRANDCHILD_DUMMY_FIRST}"
ATTACK_DUMMY_LAST = f"www-bind.{GRANDCHILD_DUMMY_LAST}"
ATTACK_TOO_MANY = f"www-bind.{GRANDCHILD_TOO_MANY}"
ATTACK_ALMOST_TOO_MANY = f"www-bind.{GRANDCHILD_ALMOST_TOO_MANY}"
FORGED_A = "6.6.6.60"
GENUINE_ALGORITHM = 13  # ECDSAP256SHA256, the parent key
DUMMY_ALGORITHM = 0  # keep in sync with ans1/ans.py
MAX_VALIDATIONS_PER_FETCH = 16  # keep in sync with ans1/ans.py

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
IGNORED_NSEC3_LOG = (
    "is_insecure_referral: NSEC3 owner zone does not enclose the DS name; ignoring"
)
REFUSED_MIXED_LOG = (
    "is_insecure_referral: NSEC RRSIG signers differ; "
    "refusing insecure-delegation proof"
)
REFUSED_TOO_MANY_LOG = (
    "is_insecure_referral: NSEC RRSIG too many signatures; "
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
    keys = {PARENT: _make_key(), CHILD: _make_key(), SIBLING: _make_key()}
    Path("ans1/keys.json").write_text(json.dumps(keys, indent=2), encoding="ascii")
    parent_dnskey = "".join(keys[PARENT]["dnskey"].split()[3:])
    return {
        "PARENT_DNSKEY": parent_dnskey,
        "MAX_VALIDATIONS": str(MAX_VALIDATIONS_PER_FETCH),
    }


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


def _rrsig_signers(response, section, owner, covered):
    """(signer, algorithm) of every RRSIG covering owner/covered, in wire order."""
    rrsig = _rrset(response, section, owner, dns.rdatatype.RRSIG, covers=covered)
    assert rrsig is not None, response.to_text()
    return [(rdata.signer.to_text(), int(rdata.algorithm)) for rdata in rrsig]


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


def test_resolver_rejects_sibling_zone_nsec3(servers):
    """
    Reproducer for #6234 (sibling-zone NSEC3).

    The forged DS NODATA answer carries, ahead of the real child-signed proof,
    an NSEC3 owned by an unrelated but genuinely delegated and signed sibling
    zone, whose owner hash matches the grandchild under the sibling's own
    parameters and whose NS bit is set.  Every signature in the answer is
    valid.  is_insecure_referral()'s trynsec3 arm sorts this record first (the
    salts are chosen so its hash does) and, before the fix, derived its signer
    as owner-minus-hash-label -> SIBLING, which is not on the path between the
    grandparent and the grandchild, so the label-count check in
    closer_secure_ds_exists() was vacuous and the secure DS at CHILD was never
    consulted.

    Before the fix this fails: the resolver returns the forged answer.
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK3_SIBLING, "A"), ATTACK3_SIBLING)
    _check_refusal_logged(servers["ns2"], ATTACK3_SIBLING, IGNORED_NSEC3_LOG)


def test_auth_serves_mixed_signer_grandparent_nsec():
    """
    Premise check for the #6321 reproducers below, again against ans1
    directly rather than the resolver.

    The DS query for each mixed-signer grandchild must be answered with
    the same grandparent-signed NSEC forgery as above, plus a second,
    dummy RRSIG that names the NSEC owner itself as signer and uses an
    algorithm the validator does not support.  The order of the two
    RRSIGs on the wire is the whole point, so it is checked here for
    both names: if dnspython ever started shuffling them, the reproducer
    would flap instead of failing cleanly.
    """
    for grandchild, expected in [
        (
            GRANDCHILD_DUMMY_FIRST,
            [(GRANDCHILD_DUMMY_FIRST, DUMMY_ALGORITHM), (PARENT, GENUINE_ALGORITHM)],
        ),
        (
            GRANDCHILD_DUMMY_LAST,
            [(PARENT, GENUINE_ALGORITHM), (GRANDCHILD_DUMMY_LAST, DUMMY_ALGORITHM)],
        ),
    ]:
        grandchild_ds = _query(AUTH, grandchild, "DS")
        isctest.check.noerror(grandchild_ds)
        nsec = _rrset(
            grandchild_ds, grandchild_ds.authority, grandchild, dns.rdatatype.NSEC
        )
        assert nsec is not None, grandchild_ds.to_text()
        assert nsec[0].next == dns.name.from_text(
            f"grandz.{CHILD}"
        ), grandchild_ds.to_text()
        signers = _rrsig_signers(
            grandchild_ds, grandchild_ds.authority, grandchild, dns.rdatatype.NSEC
        )
        assert signers == expected, grandchild_ds.to_text()


def test_resolver_rejects_mixed_signer_nsec_dummy_first(servers):
    """
    Reproducer for #6321: the NSEC is still the grandparent's forgery from
    test_resolver_rejects_grandparent_nsec_downgrade(), and it still
    authenticates only through the grandparent's RRSIG.  But the RRSIG
    rdataset now starts with a dummy signature (unsupported algorithm,
    skipped by the validator) whose signer is the NSEC owner itself.  If
    the signer used to bound the NSEC's authority is taken from the first
    RRSIG rather than from the one that verified, the bound collapses to
    the queried name, the secure DS at c.p031.test is never consulted,
    and the forged answer below the secure delegation is accepted.
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK_DUMMY_FIRST, "A"), ATTACK_DUMMY_FIRST)
    _check_refusal_logged(servers["ns2"], ATTACK_DUMMY_FIRST, REFUSED_MIXED_LOG)

    # A mixed-signer set is forged outright, so the walk must stop there
    # just like the plain grandparent forgery does.
    assert _auth_query_count(ATTACK_DUMMY_FIRST, "DS") == 0


def test_resolver_rejects_mixed_signer_nsec_dummy_last(servers):
    """
    Control for the test above with the RRSIGs in the other order: the
    genuine grandparent signature first, the dummy second.  Whatever the
    resolver does with the mixed-signer rdataset must not depend on the
    wire order the attacker chooses.
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK_DUMMY_LAST, "A"), ATTACK_DUMMY_LAST)
    _check_refusal_logged(servers["ns2"], ATTACK_DUMMY_LAST, REFUSED_MIXED_LOG)

    assert _auth_query_count(ATTACK_DUMMY_LAST, "DS") == 0


def test_resolver_rejects_mixed_signer_nsec_from_cache(servers):
    """
    The dummy-first forgery again, with a second name below the same
    grandchild so that whatever the first walk left in the cache (the
    negative DS proof, complete with its mixed RRSIG rdataset, if the
    resolver accepted it) is what the insecurity walk finds this time.
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK_DUMMY_FIRST, "A"), ATTACK_DUMMY_FIRST)

    ds_fetches = _auth_query_count(GRANDCHILD_DUMMY_FIRST, "DS")
    _check_no_downgrade(
        _query(RESOLVER, ATTACK_DUMMY_FIRST_CACHED, "A"), ATTACK_DUMMY_FIRST_CACHED
    )
    _check_refusal_logged(servers["ns2"], ATTACK_DUMMY_FIRST_CACHED, REFUSED_MIXED_LOG)

    # As in test_resolver_rejects_downgrade_from_cached_proof(): no new DS
    # fetch for the grandchild proves the mixed proof came from the cache,
    # none for the attack name proves the walk stopped there.
    assert _auth_query_count(GRANDCHILD_DUMMY_FIRST, "DS") == ds_fetches
    assert _auth_query_count(ATTACK_DUMMY_FIRST_CACHED, "DS") == 0


def test_auth_serves_many_rrsig_grandparent_nsec():
    """
    Premise check for the RRSIG-count tests below, against ans1 directly.

    Both grandchildren must get the grandparent NSEC forgery with the
    intended number of RRSIGs, every one of them naming the grandparent
    as signer, and the single genuine signature last.
    """
    for grandchild, count in [
        (GRANDCHILD_TOO_MANY, MAX_VALIDATIONS_PER_FETCH),
        (GRANDCHILD_ALMOST_TOO_MANY, MAX_VALIDATIONS_PER_FETCH - 1),
    ]:
        grandchild_ds = _query(AUTH, grandchild, "DS")
        isctest.check.noerror(grandchild_ds)
        assert (
            _rrset(
                grandchild_ds, grandchild_ds.authority, grandchild, dns.rdatatype.NSEC
            )
            is not None
        ), grandchild_ds.to_text()
        signers = _rrsig_signers(
            grandchild_ds, grandchild_ds.authority, grandchild, dns.rdatatype.NSEC
        )
        assert len(signers) == count, grandchild_ds.to_text()
        assert all(signer == PARENT for signer, _ in signers), grandchild_ds.to_text()
        assert [alg for _, alg in signers] == [DUMMY_ALGORITHM] * (count - 1) + [
            GENUINE_ALGORITHM
        ], grandchild_ds.to_text()


def test_resolver_rejects_nsec_with_too_many_rrsigs(servers):
    """
    An NSEC proof carrying at least max-validations-per-fetch RRSIGs is
    refused outright, before any signer is looked at.  The signatures
    here are uniform (all the grandparent's), so without the cap this
    would be the plain #5967 forgery and be refused for that reason
    instead; the log line pins which rule fired.
    """
    _check_no_downgrade(_query(RESOLVER, ATTACK_TOO_MANY, "A"), ATTACK_TOO_MANY)
    _check_refusal_logged(servers["ns2"], ATTACK_TOO_MANY, REFUSED_TOO_MANY_LOG)

    assert _auth_query_count(ATTACK_TOO_MANY, "DS") == 0


def test_resolver_bounds_nsec_just_below_rrsig_cap(servers):
    """
    Control for the test above with one RRSIG fewer: the cap must not
    fire, the validator must still authenticate the NSEC through the
    genuine signature after skipping every dummy, and the proof must then
    be refused by the ordinary grandparent-signer bound.
    """
    _check_no_downgrade(
        _query(RESOLVER, ATTACK_ALMOST_TOO_MANY, "A"), ATTACK_ALMOST_TOO_MANY
    )
    _check_refusal_logged(servers["ns2"], ATTACK_ALMOST_TOO_MANY, REFUSED_NSEC_LOG)

    assert _auth_query_count(ATTACK_ALMOST_TOO_MANY, "DS") == 0


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
