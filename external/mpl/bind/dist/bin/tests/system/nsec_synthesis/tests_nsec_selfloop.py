#!/usr/bin/python3

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import dns.flags
import dns.name
import dns.rdataclass
import dns.rdatatype
import pytest

from isctest.run import EnvCmd

import isctest

PARENT = "f007.test."
CHILD = "!.f007.test."
PROBE = f"probe.{CHILD}"
VICTIM = f"victim.{PARENT}"
VICTIM_A = "203.0.113.7"
AUTH = "10.53.0.3"
RESOLVER = "10.53.0.2"

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans1/ans.run",
        "ns3/K*",
        "ns3/child.f007.test.db",
        "ns3/child.f007.test.db.signed",
        "ns3/dsset-*",
        "ns3/f007.test.db",
        "ns3/f007.test.db.signed",
    ]
)


def _write_zone(path, text):
    Path(path).write_text(text, encoding="ascii")


def bootstrap():
    keygen = EnvCmd("KEYGEN", "-a ECDSA256 -q")
    dsfromkey = EnvCmd("DSFROMKEY", "-2")
    signer = EnvCmd("SIGNER", "-S -g")

    parent_ksk = keygen(f"-f KSK {PARENT}", cwd="ns3").out.strip()
    keygen(PARENT, cwd="ns3")
    child_ksk = keygen(f"-f KSK {CHILD}", cwd="ns3").out.strip()
    keygen(CHILD, cwd="ns3")

    child_ds = dsfromkey(f"{child_ksk}.key", cwd="ns3").out

    _write_zone(
        "ns3/child.f007.test.db",
        f"""$TTL 300
$ORIGIN {CHILD}
@ SOA ns.f007.test. hostmaster.f007.test. 1 3600 600 86400 300
@ NS ns.f007.test.
""",
    )
    _write_zone(
        "ns3/f007.test.db",
        f"""$TTL 300
$ORIGIN {PARENT}
@ SOA ns hostmaster 1 3600 600 86400 300
@ NS ns
ns A 10.53.0.3
victim A {VICTIM_A}
! NS ns.f007.test.
{child_ds}""",
    )

    signer(f"-o {CHILD} -f child.f007.test.db.signed child.f007.test.db", cwd="ns3")
    signer(f"-o {PARENT} -f f007.test.db.signed f007.test.db", cwd="ns3")

    dnskey = str(isctest.kasp.Key(parent_ksk, keydir="ns3").dnskey).split()
    ta = "".join(dnskey[7:])

    return {
        "DNSKEY": ta,
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


def _has_a(response, owner, address):
    rrset = _rrset(response, response.answer, owner, dns.rdatatype.A)
    return rrset is not None and any(rdata.address == address for rdata in rrset)


def _check_child_self_loop_nsec(response):
    nsec = _rrset(response, response.authority, CHILD, dns.rdatatype.NSEC)
    assert nsec is not None, response.to_text()
    assert nsec[0].next == dns.name.from_text(CHILD), response.to_text()

    rrsig = _rrset(
        response,
        response.authority,
        CHILD,
        dns.rdatatype.RRSIG,
        covers=dns.rdatatype.NSEC,
    )
    assert rrsig is not None, response.to_text()
    assert rrsig[0].signer == dns.name.from_text(CHILD), response.to_text()


def test_direct_child_self_loop_nsec_fixture():
    response = _query(AUTH, PROBE, "A")
    isctest.check.nxdomain(response)
    assert response.flags & dns.flags.AA
    _check_child_self_loop_nsec(response)


def test_resolver_does_not_synth_parent_nxdomain_from_child_nsec():
    poison = _query(RESOLVER, PROBE, "A")
    isctest.check.nxdomain(poison)
    isctest.check.adflag(poison)
    _check_child_self_loop_nsec(poison)

    response = _query(RESOLVER, VICTIM, "A")
    isctest.check.noerror(response)
    assert _has_a(response, VICTIM, VICTIM_A), response.to_text()
    isctest.check.adflag(response)
    assert _rrset(response, response.authority, CHILD, dns.rdatatype.NSEC) is None
