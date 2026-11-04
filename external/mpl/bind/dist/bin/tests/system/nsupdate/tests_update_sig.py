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
Regression tests for GL#5818: legacy DNSSEC types on the dynamic-update path.

SIG (24) and NXT (30) are obsolete DNSSEC record types, superseded by RRSIG
and NSEC in RFC 3755.  Allowing a client to inject them via dynamic update
exposed two bugs in sequence:

  * dns__db_findrdataset() asserted `covers == 0 || type == RRSIG`, which
    crashed named when a SIG update reached the prescan foreach_rr() call.
  * diff.c rdata_covers() dropped the covered type for SIG rdatas, so the
    zone DB stored every SIG rdataset under typepair (SIG, 0) instead of
    (SIG, covered_type); a second SIG add with a different covers and
    different TTL then tripped DNS_DBADD_EXACTTTL in qpzone and came back
    as SERVFAIL.

The adopted defence is to outright refuse SIG and NXT updates at the front
door (ns/update.c), keeping KEY updates permitted for SIG(0) transaction
signatures.  These tests verify the refusal.  The reachability of the
diff.c:rdata_covers() bug via inbound zone transfer is covered separately
by the AXFR-based regression test in this file.
"""

import time

import dns.rcode
import dns.rdata
import dns.rdataclass
import dns.rdataset
import dns.rdatatype
import dns.update
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ans*/ans.run",
        "ns*/*.bk",
        "ns*/*.conf",
        "ns*/*.db",
        "ns*/*.db.jnl",
        "ns*/*.db.signed",
        "ns*/*.jnl",
        "ns*/*.key",
        "ns*/K*.key",
        "ns*/K*.private",
        "ns*/K*.state",
        "ns*/dsset-*.",
        "ns6/sigaxfr.bk",
        "verylarge",
    ]
)


def _make_sig_rdata(text):
    """Create a SIG rdata from text.

    dnspython has no native text parser for the legacy SIG type (24),
    but the wire format is identical to RRSIG (46).  Parse as RRSIG,
    then re-wrap as SIG via the wire representation.
    """
    rrsig = dns.rdata.from_text(dns.rdataclass.IN, dns.rdatatype.RRSIG, text)
    wire = rrsig.to_digestable()
    return dns.rdata.from_wire(dns.rdataclass.IN, dns.rdatatype.SIG, wire, 0, len(wire))


def _make_nxt_rdata():
    """Create a minimal NXT rdata.

    NXT wire format (RFC 2535) is: next-name + type-bitmap.  The exact
    content does not matter for the refusal test; we just need a
    syntactically valid NXT rdata.
    """
    # next-name = root (\x00), type bitmap covering type A only.
    wire = b"\x00\x00\x00\x00\x40"
    return dns.rdata.from_wire(dns.rdataclass.IN, dns.rdatatype.NXT, wire, 0, len(wire))


def test_tcp_self_sig_record(ns6):
    """SIG (type 24) updates must be refused at the front door.

    Prior to the fix in dns__db_findrdataset(), a SIG update here
    crashed named.  Prior to the fix in diff.c rdata_covers(), the
    record was silently misfiled under typepair (SIG, 0).  The
    adopted policy outright refuses SIG (obsolete; use RRSIG) so the
    buggy dynamic-update paths are no longer reachable.  A PTR add
    first ensures the node exists, which is the original
    crash-reproducing precondition.
    """
    ptr_update = dns.update.UpdateMessage("in-addr.arpa.")
    ptr_update.add("1.0.53.10.in-addr.arpa.", 600, "PTR", "localhost.")
    response = isctest.query.tcp(
        ptr_update, ns6.ip, port=ns6.ports.dns, source="10.53.0.1"
    )
    assert response.rcode() == dns.rcode.NOERROR

    sig = _make_sig_rdata("A 6 0 86400 20260331170000 20260318160000 21831 . 0000")
    rds = dns.rdataset.Rdataset(dns.rdataclass.IN, dns.rdatatype.SIG)
    rds.update_ttl(600)
    rds.add(sig)
    sig_update = dns.update.UpdateMessage("in-addr.arpa.")
    sig_update.add("1.0.0.127.in-addr.arpa.", rds)

    with ns6.watch_log_from_here() as watcher:
        response = isctest.query.tcp(
            sig_update, ns6.ip, port=ns6.ports.dns, source="10.53.0.1"
        )
        assert response.rcode() == dns.rcode.REFUSED

        watcher.wait_for_line(
            "updating zone 'in-addr.arpa/IN': update failed: SIG updates are not allowed (REFUSED)"
        )

    # Confirm nothing of type SIG was stored.
    msg = isctest.query.create("1.0.53.10.in-addr.arpa.", "SIG")
    res = isctest.query.tcp(msg, ns6.ip, port=ns6.ports.dns)
    stored = any(rrset.rdtype == dns.rdatatype.SIG for rrset in res.answer)
    assert not stored, "SIG record was stored despite REFUSED response"


def test_tcp_self_nxt_record(ns6):
    """NXT (type 30) updates must be refused at the front door.

    NXT is the legacy DNSSEC denial-of-existence type, obsolete since
    RFC 3755 replaced it with NSEC.  Accepting it via dynamic update
    would let an authorised updater inject records that the signing
    and cut-point logic has no provision for.
    """
    # A second owner under a source that also matches tcp-self.
    source = "10.53.0.2"
    owner = "2.0.53.10.in-addr.arpa."

    ptr_update = dns.update.UpdateMessage("in-addr.arpa.")
    ptr_update.add(owner, 600, "PTR", "localhost.")
    response = isctest.query.tcp(ptr_update, ns6.ip, port=ns6.ports.dns, source=source)
    assert response.rcode() == dns.rcode.NOERROR

    nxt = _make_nxt_rdata()
    rds = dns.rdataset.Rdataset(dns.rdataclass.IN, dns.rdatatype.NXT)
    rds.update_ttl(600)
    rds.add(nxt)
    nxt_update = dns.update.UpdateMessage("in-addr.arpa.")
    nxt_update.add(owner, rds)

    with ns6.watch_log_from_here() as watcher:
        response = isctest.query.tcp(
            nxt_update, ns6.ip, port=ns6.ports.dns, source="10.53.0.1"
        )
        assert response.rcode() == dns.rcode.REFUSED

        watcher.wait_for_line(
            "updating zone 'in-addr.arpa/IN': update failed: NXT updates are not allowed (REFUSED)"
        )


def test_sig_covers_preserved_via_axfr(ns6):
    """Regression test for GL#5818 Finding 1, reached via AXFR.

    ans11 serves an AXFR for sigaxfr.nil. containing two SIG rdatas at
    the same owner with different covered types (A, MX) and different
    TTLs (600, 1200).  ns6 pulls the zone via dns_diff_load(), which
    calls diff.c rdata_covers(); before the fix that helper returned 0
    for SIG, so both tuples were grouped and filed under typepair
    (SIG, 0) with the first TTL (600) — the MX-covering record's TTL
    (1200) was silently dropped.  With the fix the records land in
    distinct typepairs and both TTLs survive.

    rndc dumpdb is used to inspect the secondary's stored state
    directly; the wire-level response can merge same-(owner,type,class)
    RRs and mask the difference.
    """
    zone = "sigaxfr.nil"
    owner = f"host.{zone}."
    dump_path = ns6.directory / "named_dump.db"

    # ns6 may have tried to SOA-poll ans11 before it was listening; force
    # a fresh refresh attempt and wait for the transfer to complete.
    with ns6.watch_log_from_here() as watcher:
        ns6.rndc(f"refresh {zone}")
        watcher.wait_for_line(f"zone {zone}/IN: transferred serial 1")

    # Remove any stale dump and ask named for a fresh one.
    if dump_path.exists():
        dump_path.unlink()
    ns6.rndc("dumpdb -zones")

    # rndc dumpdb is asynchronous; wait for the file and for its
    # trailing "Dump complete" marker.
    deadline_marker = "; Dump complete"
    for _ in range(50):
        if dump_path.exists():
            text = dump_path.read_text()
            if deadline_marker in text:
                break
        time.sleep(0.1)
    else:
        raise AssertionError(f"{dump_path} never contained {deadline_marker!r}")

    # Collect every SIG line for the owner from the dump.  Format is:
    #   <owner>. <ttl> IN SIG <covered> <alg> <labels> ...
    sig_lines = []
    for line in text.splitlines():
        fields = line.split()
        if len(fields) < 6:
            continue
        if not fields[0].lower().startswith("host.sigaxfr.nil"):
            continue
        if fields[2] != "IN" or fields[3] != "SIG":
            continue
        sig_lines.append(fields)

    assert (
        len(sig_lines) == 2
    ), f"expected 2 SIG records at {owner}, got {len(sig_lines)}: {sig_lines}"

    ttl_by_covers = {fields[4]: int(fields[1]) for fields in sig_lines}
    assert ttl_by_covers == {"A": 600, "MX": 1200}, (
        f"SIG records lost their covers/TTL binding: {ttl_by_covers}.  With "
        "the Finding 1 bug both records are filed under typepair (SIG, 0) "
        "and share the first-seen TTL (600)."
    )
