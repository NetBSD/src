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

"""
Regression test for issue #5877: a CD=1 (checking-disabled) NXDOMAIN response
is cached at dns_trust_pending_answer and must not evict a DNSSEC-validated
RRset cached at dns_trust_secure for the same name.

ns1 is a validating resolver that forwards "example" to the signed ns2.  We
prime the cache with a validated a.example/A (trust=secure), flip ns2 to a
version of the zone where a.example no longer exists, and send a CD=1 query
that makes the resolver cache an unvalidated NXDOMAIN for a.example.  The
originally cached, more-trusted A record must survive.

The last test covers the inverse: once the validated RRset has passed its
TTL it must no longer block the negative entry, nor be served to the
client in its place.
"""

import os
import shutil
import time

import dns.zone

import isctest

RESOLVER = "10.53.0.1"
AUTH = "10.53.0.2"
A_RDATA = "10.53.0.99"
SHORT_TTL = 3


def bootstrap():
    alg = os.environ["DEFAULT_ALGORITHM"]
    bits = os.environ["DEFAULT_BITS"]
    keygen = isctest.run.EnvCmd("KEYGEN", f"-a {alg} -b {bits} -Kns2 -q")
    signer = isctest.run.EnvCmd("SIGNER", "-S -g")

    name = "example"

    zsk_name = keygen(name).out.strip()
    isctest.kasp.Key(zsk_name, keydir="ns2")
    ksk_name = keygen(f"-f KSK {name}").out.strip()
    ksk = isctest.kasp.Key(ksk_name, keydir="ns2")

    zonetext = """
@ 300 IN SOA ns2.example. admin.example. 0 3600 1800 604800 300
@ NS ns2.example.
ns2 A 10.53.0.2
"""
    zone = dns.zone.from_text(zonetext, origin=name)
    zone.to_file(f"ns2/{name}-empty.db", sorted=True)
    signer(
        f"-P -x -O full -o {name} -f {name}-empty.db.signed {name}-empty.db", cwd="ns2"
    )

    zonetext += f"a A 10.53.0.99\nshort {SHORT_TTL} A 10.53.0.99\n"
    zone = dns.zone.from_text(zonetext, origin=name)
    zone.to_file(f"ns2/{name}-full.db", sorted=True)
    signer(
        f"-P -x -O full -o {name} -f {name}-full.db.signed {name}-full.db", cwd="ns2"
    )

    shutil.copyfile(f"ns2/{name}-full.db.signed", f"ns2/{name}.db.signed")

    return {
        "trust_anchors": [
            ksk.into_ta("static-key"),
        ],
    }


def _serve(ns2, system_test_dir, variant):
    """Make ns2 serve the 'full' or 'empty' (a.example-less) signed zone."""
    src = system_test_dir / "ns2" / f"example-{variant}.db.signed"
    dst = system_test_dir / "ns2" / "example.db.signed"
    # Ensure that the modification time of 'dst' increases on
    # file systems with seconds granuality.
    st_ok = False
    try:
        st = os.stat(dst)
        st_ok = True
    except FileNotFoundError:
        pass
    shutil.copyfile(src, dst)
    if st_ok:
        st2 = os.stat(dst)
        if int(st2.st_mtime) <= int(st.st_mtime):
            os.utime(dst, (st2.st_atime, st.st_mtime + 1))
    ns2.reload()


def _prime_secure_a(ns1):
    """Cache a.example/A at trust=secure and confirm it validated (AD=1)."""
    ns1.rndc("flush")
    res = isctest.query.tcp(isctest.query.create("a.example", "A"), RESOLVER)
    isctest.check.noerror(res)
    isctest.check.adflag(res)
    assert A_RDATA in str(res.answer), res.answer


def test_cd1_nxdomain_keeps_secure_rrset(ns1, ns2, system_test_dir):
    # Prime the resolver with a DNSSEC-validated a.example/A (trust=secure).
    _serve(ns2, system_test_dir, "full")
    _prime_secure_a(ns1)

    # Flip the authoritative zone to a (still signed) version without
    # a.example, so the name now yields a signed NXDOMAIN; confirm at ns2.
    _serve(ns2, system_test_dir, "empty")
    direct = isctest.query.create("a.example", "A", dnssec=False)
    isctest.check.nxdomain(isctest.query.tcp(direct, AUTH))

    # A CD=1 query elicits an unvalidated NXDOMAIN for a.example, cached at
    # trust=pending_answer.  The the secure A record should not be evicted.
    cd_msg = isctest.query.create("a.example", "TXT", cd=True)
    isctest.query.tcp(cd_msg, RESOLVER)

    # The resolver should not refetch from the zone; the validated A
    # record should still be served from cache.
    res = isctest.query.tcp(isctest.query.create("a.example", "A"), RESOLVER)
    isctest.check.noerror(res)
    isctest.check.adflag(res)
    assert A_RDATA in str(res.answer), res.answer


def test_cd1_nxdomain_uncached_type_answer(ns1, ns2, system_test_dir):
    # Prime a.example/A at trust=secure, then make a.example yield NXDOMAIN.
    _serve(ns2, system_test_dir, "full")
    _prime_secure_a(ns1)
    _serve(ns2, system_test_dir, "empty")

    # CD=1 query for an UNCACHED type (AAAA): the unvalidated NXDOMAIN is
    # rejected in favour of the secure A.  Returning the cached A record (the
    # wrong type) in the answer section is incorrect; the answer must be empty.
    res = isctest.query.tcp(
        isctest.query.create("a.example", "AAAA", cd=True), RESOLVER
    )
    isctest.check.servfail(res)
    isctest.check.empty_answer(res)

    # a CD=0 query should now validate the NXDOMAIN, ejecting the A.
    res = isctest.query.tcp(isctest.query.create("a.example", "AAAA"), RESOLVER)
    isctest.check.nxdomain(res)
    isctest.check.adflag(res)
    isctest.check.empty_answer(res)

    res = isctest.query.tcp(isctest.query.create("a.example", "A"), RESOLVER)
    isctest.check.nxdomain(res)
    isctest.check.adflag(res)
    isctest.check.empty_answer(res)


def test_cd1_nxdomain_expired_secure_does_not_block(ns1, ns2, system_test_dir):
    # Prime the resolver with the short-TTL validated short.example/A.
    _serve(ns2, system_test_dir, "full")
    ns1.rndc("flush")
    res = isctest.query.tcp(isctest.query.create("short.example", "A"), RESOLVER)
    isctest.check.noerror(res)
    isctest.check.adflag(res)
    assert A_RDATA in str(res.answer), res.answer

    # Flip the zone to the version without the name and wait out the TTL.
    # Expired headers are only cleaned lazily, so the secure A record is
    # still sitting in the cache when the negative entry arrives.
    _serve(ns2, system_test_dir, "empty")
    time.sleep(SHORT_TTL + 2)
    direct = isctest.query.create("short.example", "A", dnssec=False)
    isctest.check.nxdomain(isctest.query.tcp(direct, AUTH))

    # The expired secure A record must neither block the unvalidated
    # NXDOMAIN from being cached nor be handed back to the client in its
    # place.
    res = isctest.query.tcp(
        isctest.query.create("short.example", "A", cd=True), RESOLVER
    )
    isctest.check.nxdomain(res)
    isctest.check.empty_answer(res)
