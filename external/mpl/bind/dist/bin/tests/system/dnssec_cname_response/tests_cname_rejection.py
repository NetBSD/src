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

from re import compile as Re

import base64
import time

from cryptography.hazmat.primitives.asymmetric import ec
from dns.rdtypes.dnskeybase import Flag

import dns.dnssec
import dns.rdataclass
import dns.zone
import pytest

import isctest


def _sign_zone(db_in, signed_out, origin):
    """Sign 'db_in' with a fresh KSK; write 'signed_out'; return the KSK
    public key (base64) for use as a static trust anchor."""
    ksk_private_key = ec.generate_private_key(ec.SECP384R1())
    ksk_dnskey = dns.dnssec.make_dnskey(
        public_key=ksk_private_key.public_key(),
        algorithm=dns.dnssec.Algorithm.ECDSAP384SHA384,
        flags=Flag.ZONE | Flag.SEP,
    )

    zone = dns.zone.from_file(db_in, origin=origin)
    with zone.writer() as txn:
        dns.dnssec.sign_zone(
            zone=zone,
            txn=txn,
            keys=[(ksk_private_key, ksk_dnskey)],
            lifetime=300,
            add_dnskey=True,
            deterministic=False,  # for OpenSSL<3.2.0 compat
        )
    zone.to_file(signed_out)

    return base64.b64encode(ksk_dnskey.key).decode()


def bootstrap():
    try:
        result = {
            "ksk_public_key": _sign_zone(
                "ans2/example.db.in", "ans2/example.signed.db", "example."
            ),
            "secure_ksk_public_key": _sign_zone(
                "ans2/secure.db.in", "ans2/secure.signed.db", "secure."
            ),
        }
    except ImportError as exc:
        pytest.skip(f"{exc}")
    return result


def _assert_ns3_alive():
    """Fail if ns3 is no longer answering (e.g. it hit an assertion)."""
    liveness = isctest.query.create("version.bind.", "TXT", dns.rdataclass.CH, rd=False)
    res = isctest.query.tcp(liveness, "10.53.0.3", timeout=5)
    assert res is not None, "ns3 did not answer a liveness query -- it may have crashed"


@pytest.mark.parametrize("qtype", ["DNSKEY", "NSEC", "NSEC3", "RRSIG"])
def test_direct_metatype_query_does_not_crash_resolver(qtype):
    """
    A direct recursive client query for a DNSSEC meta-type, answered by a
    malicious authoritative server with a CNAME, must not crash the
    resolver. This probes the client-facing consumers of the resolver
    fetch (ns_query/query_cname), not the validator's internal fetch.

    A resolver fetch that completes with DNS_R_CNAME goes through the
    normal answer path, which binds the answer name and rdataset. An
    earlier resolver-side shortcut returned DNS_R_CNAME without binding
    them, so query_cname() handed an empty (non-absolute) name to
    dns_message_addname() and named aborted on REQUIRE(dns_name_isabsolute).
    """
    msg = isctest.query.create("sub.example.", qtype)

    start_time = time.time()
    res = isctest.query.tcp(msg, "10.53.0.3", timeout=8)
    elapsed_time = time.time() - start_time

    # The resolver must answer promptly. An RRSIG query is handled as a
    # subset of ANY, and a CNAME answer to it used to be dropped without
    # caching or validation, leaving the fetch waiting ~12s for a
    # validator that was never started.
    assert elapsed_time < 5.0, f"{qtype} query took too long: {elapsed_time}s"

    # We do not assert a particular rcode here -- SERVFAIL or a chased
    # answer are both acceptable. The point is that named survives.
    assert res is not None, f"no response to direct {qtype} query"
    _assert_ns3_alive()


def test_rrsig_lone_record_does_not_stall_resolver():
    """
    A direct recursive RRSIG query answered with an unrelated record
    (here a lone A, with no RRSIG and no alias) must not stall the
    resolver. An RRSIG query is handled as a subset of ANY; every record
    of the wrong type is filtered out, and when nothing is left the
    answer used to be accepted as success with no answer bound, leaving
    the fetch waiting ~12s for a validator that was never started.
    """
    msg = isctest.query.create("lone-a.example.", "RRSIG")

    start_time = time.time()
    res = isctest.query.tcp(msg, "10.53.0.3", timeout=8)
    elapsed_time = time.time() - start_time

    assert elapsed_time < 5.0, f"RRSIG query took too long: {elapsed_time}s"
    assert res is not None, "no response to lone-record RRSIG query"
    _assert_ns3_alive()


def test_cname_for_validator_dnskey_fetch(ns3):
    """
    A malicious authoritative server returning a CNAME for the
    validator's DNSKEY fetch must not stall validation. The DNSKEY
    fetch completes with DNS_R_CNAME, which the validator treats as a
    broken trust chain, so the client query terminates with SERVFAIL
    rather than hanging. No resolver-side special case is needed: the
    validator already rejects a CNAME answer to its meta-fetch.
    """
    log_brokenchain = Re(r"broken trust chain resolving 'www\.example/A/IN'")

    msg = isctest.query.create("www.example.", "A")

    start_time = time.time()
    with ns3.watch_log_from_here(timeout=5) as watcher:
        res = isctest.query.tcp(msg, "10.53.0.3")
        watcher.wait_for_line(log_brokenchain)
    elapsed_time = time.time() - start_time

    assert elapsed_time < 5.0, f"Query took too long: {elapsed_time}s"
    isctest.check.servfail(res)


def test_ds_cname_does_not_deadlock():
    """
    A DS query answered with an unsigned CNAME must not send the validator
    into a self-join deadlock (GL#5878). While proving the CNAME insecure
    the validator would fetch the DS for the same name, re-entering the
    in-flight DS fetch it is blocked on and stalling for ~12 seconds until a
    backstop timer fires. The validator now detects that such a fetch cannot
    advance the alias chain and aborts, so the client gets SERVFAIL promptly.

    'secure.' is a properly signed zone (so validation reaches the DS query),
    but its authoritative server answers DS queries with an unsigned CNAME.
    """
    msg = isctest.query.create("insecure.secure.", "DS")

    start_time = time.time()
    res = isctest.query.tcp(msg, "10.53.0.3", timeout=8)
    elapsed_time = time.time() - start_time

    assert (
        elapsed_time < 5.0
    ), f"DS query took too long: {elapsed_time}s (possible deadlock)"
    isctest.check.servfail(res)
    _assert_ns3_alive()
