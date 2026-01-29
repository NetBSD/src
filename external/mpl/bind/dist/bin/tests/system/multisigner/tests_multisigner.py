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

from datetime import timedelta
import os
from re import compile as Re

import pytest

pytest.importorskip("dns", minversion="2.0.0")
import dns
import dns.update

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*.axfr",
        "*.created",
        "cdnskey.ns*",
        "cds.ns*",
        "dig.out.*",
        "rndc.dnssec.status.out.*",
        "secondary.cdnskey.ns*",
        "secondary.cds.ns*",
        "unused.*",
        "verify.out.*",
        "ns*/K*",
        "ns*/db-*",
        "ns*/keygen.out.*",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/*.zsk",
        "ns*/*.signed",
        "ns*/*.journal.out.*",
        "ns*/settime.out.*",
        "ns*/model2.secondary.db",
    ]
)

ALGORITHM = os.environ["DEFAULT_ALGORITHM_NUMBER"]
SIZE = os.environ["DEFAULT_BITS"]
CONFIG = {
    "dnskey-ttl": timedelta(hours=1),
    "ds-ttl": timedelta(days=1),
    "max-zone-ttl": timedelta(days=1),
    "parent-propagation-delay": timedelta(hours=1),
    "publish-safety": timedelta(hours=1),
    "retire-safety": timedelta(hours=1),
    "signatures-refresh": timedelta(days=5),
    "signatures-validity": timedelta(days=14),
    "zone-propagation-delay": timedelta(minutes=5),
}
TTL = 3600


def dsfromkey(key):
    dsfromkey_command = [
        os.environ.get("DSFROMKEY"),
        "-T",
        str(TTL),
        "-a",
        "SHA-256",
        "-C",
        "-w",
        str(key.keyfile),
    ]
    cmd = isctest.run.cmd(dsfromkey_command)
    return cmd.out.split()


def check_dnssec(server, zone, keys, expected):
    ksks = [k for k in keys if k.is_ksk()]
    zsks = [k for k in keys if not k.is_ksk()]

    isctest.kasp.check_keys(zone, keys, expected)

    for kp in expected:
        kp.set_expected_keytimes(CONFIG)
        kp.set_expected_keytimes(CONFIG)
        start = kp.key.get_timing("Created")
        kp.timing["Published"] = start
        kp.timing["Active"] = start
        if kp.role != "zsk":
            kp.timing["PublishCDS"] = start

    isctest.kasp.check_dnssec_verify(server, zone)
    isctest.kasp.check_apex(server, zone, ksks, zsks)


def check_no_dnssec_in_journal(server, zone):
    journalprint = [
        os.environ.get("JOURNALPRINT"),
        f"{server.identifier}/{zone}.db.jnl",
    ]

    cmd = isctest.run.cmd(journalprint)
    assert (
        Re(r"^\s*(?:\S+\s+){4}(NSEC|NSEC3|NSEC3PARAM|RRSIG)") not in cmd.out
    ), "dnssec record found in journal"


def wait_for_serial(primary, server, zone):
    if primary.identifier == server.identifier:
        # No need to check if the transfer has been done.
        return

    def check_serial():
        response = isctest.query.tcp(
            query, primary.ip, primary.ports.dns, timeout=3, attempts=1
        )
        assert response.rcode() == dns.rcode.NOERROR
        soa = response.get_rrset(
            response.answer,
            dns.name.from_text(fqdn),
            dns.rdataclass.IN,
            dns.rdatatype.SOA,
        )
        serial1 = soa[0].serial

        response = isctest.query.tcp(
            query, server.ip, server.ports.dns, timeout=3, attempts=1
        )
        assert response.rcode() == dns.rcode.NOERROR
        soa = response.get_rrset(
            response.answer,
            dns.name.from_text(fqdn),
            dns.rdataclass.IN,
            dns.rdatatype.SOA,
        )
        serial2 = soa[0].serial

        return (
            f"zone {zone}/IN (signed): serial {serial2} (unsigned {serial1})"
            in server.log
        )

    fqdn = f"{zone}."
    query = isctest.query.create(fqdn, dns.rdatatype.SOA)

    isctest.run.retry_with_timeout(check_serial, timeout=10)


def check_add_zsk(server, zone, keys, expected, extra_keys, extra, primary=None):
    if primary is None:
        primary = server

    isctest.log.info("add dnskey record:")

    isctest.log.info(
        f"- zone {zone} {primary.identifier}: update zone with ZSK from other providers"
    )

    update_msg = dns.update.UpdateMessage(zone)
    for zsk in extra_keys:
        dnskey = str(zsk.dnskey).split()
        rdata = " ".join(dnskey[4:])
        update_msg.add(f"{zone}.", TTL, "DNSKEY", rdata)
    primary.nsupdate(update_msg)

    wait_for_serial(primary, server, zone)

    # Check the new DNSKEY RRset.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check DNSKEY RRset after update add"
    )
    check_dnssec(server, zone, keys + extra_keys, expected + extra)

    # Check the logs for find zone keys errors.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: make sure we did not try to sign with the keys added with nsupdate"
    )
    assert f"dns_zone_findkeys: error reading ./K{zone}" not in server.log

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys + extra_keys, expected + extra)
    assert f"dns_zone_findkeys: error reading ./K{zone}" not in server.log


def _check_remove_zsk_fail(
    server, zone, keys, expected, extra_keys, extra, primary=None
):
    if primary is None:
        primary = server

    isctest.log.info(
        f"- zone {zone} {primary.identifier}: try to remove own ZSK (should fail)"
    )

    zsks = [k for k in keys if not k.is_ksk()]
    dnskey = str(zsks[0].dnskey).split()
    rdata = " ".join(dnskey[4:])
    update_msg = dns.update.UpdateMessage(zone)
    update_msg.delete(f"{zone}.", "DNSKEY", rdata)
    with primary.watch_log_from_here() as watcher:
        primary.nsupdate(update_msg)
        watcher.wait_for_line(
            f"updating zone '{zone}/IN': attempt to delete in use DNSKEY ignored"
        )

    # Both ZSKs should still be published.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check DNSKEY RRset after ignored remove"
    )
    check_dnssec(server, zone, keys + extra_keys, expected + extra)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys + extra_keys, expected + extra)


def check_remove_zsk(
    server, zone, keys, expected, extra_keys, extra, primary=None, check_fail=False
):
    isctest.log.info("remove dnskey record:")

    if primary is None:
        primary = server

    if check_fail:
        _check_remove_zsk_fail(
            server, zone, keys, expected, extra_keys, extra, primary=primary
        )

    # Remove actual ZSK.
    isctest.log.info(
        f"- zone {zone} {primary.identifier}: remove ZSK from other providers"
    )

    update_msg = dns.update.UpdateMessage(zone)
    for zsk in extra_keys:
        dnskey = str(zsk.dnskey).split()
        rdata = " ".join(dnskey[4:])
        update_msg.delete(f"{zone}.", "DNSKEY", rdata)
    primary.nsupdate(update_msg)

    wait_for_serial(primary, server, zone)

    # We should have only the KSK and ZSK from server.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check DNSKEY RRset after update remove"
    )
    check_dnssec(server, zone, keys, expected)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys, expected)


def check_add_cdnskey(server, zone, keys, expected, extra_keys, extra, primary=None):
    if primary is None:
        primary = server

    isctest.log.info("add cdnskey record:")

    isctest.log.info(
        f"- zone {zone} {primary.identifier}: update zone with CDNSKEY from other providers"
    )

    # Update the server with the CDNSKEY record from the other providers.
    update_msg = dns.update.UpdateMessage(zone)
    for ksk in extra_keys:
        dnskey = str(ksk.dnskey).split()
        rdata = " ".join(dnskey[4:])
        update_msg.add(f"{zone}.", TTL, "CDNSKEY", rdata)
    primary.nsupdate(update_msg)

    wait_for_serial(primary, server, zone)

    # Now there should be two CDNSKEY records.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check CDNSKEY RRset after update add"
    )
    check_dnssec(server, zone, keys + extra_keys, expected + extra)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys + extra_keys, expected + extra)


def _check_remove_cdnskey_fail(
    server, zone, keys, expected, extra_keys, extra, primary=None
):
    if primary is None:
        primary = server

    isctest.log.info(
        f"- zone {zone} {primary.identifier}: try to remove own CDNSKEY (should fail)"
    )

    ksks = [k for k in keys if not k.is_ksk()]
    dnskey = str(ksks[0].dnskey).split()
    rdata = " ".join(dnskey[4:])
    update_msg = dns.update.UpdateMessage(zone)
    update_msg.delete(f"{zone}.", "CDNSKEY", rdata)
    with primary.watch_log_from_here() as watcher:
        primary.nsupdate(update_msg)
        watcher.wait_for_line(
            f"updating zone '{zone}/IN': attempt to delete in use CDNSKEY ignored"
        )

    # Both CDNSKEY records should still be published.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check CDNSKEY RRset after ignored remove"
    )
    check_dnssec(server, zone, keys + extra_keys, expected + extra)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys + extra_keys, expected + extra)


def check_remove_cdnskey(
    server, zone, keys, expected, extra_keys, extra, primary=None, check_fail=False
):
    isctest.log.info("remove cdnskey record:")

    if primary is None:
        primary = server

    if check_fail:
        _check_remove_cdnskey_fail(
            server, zone, keys, expected, extra_keys, extra, primary=primary
        )

    # Remove actual CDNSKEY.
    isctest.log.info(
        f"- zone {zone} {primary.identifier}: remove CDNSKEY from other providers"
    )

    update_msg = dns.update.UpdateMessage(zone)
    for ksk in extra_keys:
        dnskey = str(ksk.dnskey).split()
        rdata = " ".join(dnskey[4:])
        update_msg.delete(f"{zone}.", "CDNSKEY", rdata)
    primary.nsupdate(update_msg)

    wait_for_serial(primary, server, zone)

    # Now there should be one CDNSKEY record again.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check CDNSKEY RRset after update remove"
    )
    check_dnssec(server, zone, keys, expected)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys, expected)


def check_add_cds(server, zone, keys, expected, extra_keys, extra, primary=None):
    isctest.log.info("add cds record:")

    if primary is None:
        primary = server

    isctest.log.info(
        f"- zone {zone} {primary.identifier}: update zone with CDS from other providers"
    )

    # Update the server with the CDS record from the other providers.
    update_msg = dns.update.UpdateMessage(zone)
    for ksk in extra_keys:
        ds = dsfromkey(ksk)
        rdata = " ".join(ds[4:])
        update_msg.add(f"{zone}.", TTL, "CDS", rdata)
    primary.nsupdate(update_msg)

    wait_for_serial(primary, server, zone)

    # Now there should be two CDS records.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check CDS RRset after update add"
    )
    check_dnssec(server, zone, keys + extra_keys, expected + extra)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys + extra_keys, expected + extra)


def _check_remove_cds_fail(
    server, zone, keys, expected, extra_keys, extra, primary=None
):
    if primary is None:
        primary = server

    isctest.log.info(
        f"- zone {zone} {primary.identifier}: try to remove own CDS (should fail)"
    )

    ksks = [k for k in keys if not k.is_ksk()]
    ds = dsfromkey(ksks[0])
    rdata = " ".join(ds[4:])
    update_msg = dns.update.UpdateMessage(zone)
    update_msg.delete(f"{zone}.", "CDS", rdata)
    with primary.watch_log_from_here() as watcher:
        primary.nsupdate(update_msg)
        watcher.wait_for_line(
            f"updating zone '{zone}/IN': attempt to delete in use CDS ignored"
        )

    # Both CDS records should still be published.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check CDS RRset after ignored remove"
    )
    check_dnssec(server, zone, keys + extra_keys, expected + extra)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys + extra_keys, expected + extra)


def check_remove_cds(
    server, zone, keys, expected, extra_keys, extra, primary=None, check_fail=False
):
    isctest.log.info("remove cds record:")

    if primary is None:
        primary = server

    if check_fail:
        _check_remove_cds_fail(
            server, zone, keys, expected, extra_keys, extra, primary=primary
        )

    # Remove actual CDS.
    isctest.log.info(
        f"- zone {zone} {primary.identifier}: remove CDS from other providers"
    )

    update_msg = dns.update.UpdateMessage(zone)
    for ksk in extra_keys:
        ds = dsfromkey(ksk)
        rdata = " ".join(ds[4:])
        update_msg.delete(f"{zone}.", "CDS", rdata)
    primary.nsupdate(update_msg)

    wait_for_serial(primary, server, zone)

    # Now there should be one CDS record again.
    isctest.log.info(
        f"- zone {zone} {server.identifier}: check CDS RRset after update remove"
    )
    check_dnssec(server, zone, keys, expected)

    # Trigger keymgr.
    with server.watch_log_from_here() as watcher:
        server.rndc(f"loadkeys {zone}")
        watcher.wait_for_line(f"keymgr: {zone} done")

    # Check again.
    isctest.log.info(f"- zone {zone} {server.identifier}: check again after keymgr run")
    check_dnssec(server, zone, keys, expected)


def test_multisigner(ns3, ns4):
    zone = "model2.multisigner"
    keyprops = [
        f"ksk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]

    # First make sure the zone is properly signed.
    isctest.log.info(f"basic DNSSEC tests for {zone}")
    isctest.kasp.wait_keymgr_done(ns3, zone)
    isctest.kasp.wait_keymgr_done(ns4, zone)

    keys3 = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    ksks3 = [k for k in keys3 if k.is_ksk()]
    zsks3 = [k for k in keys3 if not k.is_ksk()]
    expected3 = isctest.kasp.policy_to_properties(ttl=TTL, keys=keyprops)

    check_dnssec(ns3, zone, keys3, expected3)

    keys4 = isctest.kasp.keydir_to_keylist(zone, ns4.identifier)
    ksks4 = [k for k in keys4 if k.is_ksk()]
    zsks4 = [k for k in keys4 if not k.is_ksk()]
    expected4 = isctest.kasp.policy_to_properties(ttl=TTL, keys=keyprops)

    check_dnssec(ns4, zone, keys4, expected4)

    # Add DNSKEY to RRset.
    newprops = [f"zsk unlimited {ALGORITHM} {SIZE}"]
    extra = isctest.kasp.policy_to_properties(ttl=TTL, keys=newprops)
    extra[0].private = False  # noqa
    extra[0].legacy = True  # noqa

    check_add_zsk(ns3, zone, keys3, expected3, [zsks4[0]], extra)
    check_add_zsk(ns4, zone, keys4, expected4, [zsks3[0]], extra)
    check_no_dnssec_in_journal(ns4, zone)

    # Remove DNSKEY from RRset.
    check_remove_zsk(ns3, zone, keys3, expected3, [zsks4[0]], extra, check_fail=True)
    check_remove_zsk(ns4, zone, keys4, expected4, [zsks3[0]], extra, check_fail=True)
    check_no_dnssec_in_journal(ns4, zone)

    # Add CDNSKEY RRset.
    newprops = [f"ksk unlimited {ALGORITHM} {SIZE}"]
    extra = isctest.kasp.policy_to_properties(ttl=TTL, keys=newprops)
    extra[0].private = False  # noqa
    extra[0].legacy = True  # noqa

    check_add_cdnskey(ns3, zone, keys3, expected3, [ksks4[0]], extra)
    check_add_cdnskey(ns4, zone, keys4, expected4, [ksks3[0]], extra)
    check_no_dnssec_in_journal(ns4, zone)

    # Remove CDNSKEY RRset.
    check_remove_cdnskey(
        ns3, zone, keys3, expected3, [ksks4[0]], extra, check_fail=True
    )
    check_remove_cdnskey(
        ns4, zone, keys4, expected4, [ksks3[0]], extra, check_fail=True
    )
    check_no_dnssec_in_journal(ns4, zone)

    # Update CDS RRset.
    check_add_cds(ns3, zone, keys3, expected3, [ksks4[0]], extra)
    check_add_cds(ns4, zone, keys4, expected4, [ksks3[0]], extra)
    check_no_dnssec_in_journal(ns4, zone)

    # Remove CDS RRset.
    check_remove_cds(ns3, zone, keys3, expected3, [ksks4[0]], extra, check_fail=True)
    check_remove_cds(ns4, zone, keys4, expected4, [ksks3[0]], extra, check_fail=True)
    check_no_dnssec_in_journal(ns4, zone)


def test_multisigner_secondary(ns3, ns4, ns5):
    zone = "model2.secondary"
    keyprops = [
        f"ksk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:omnipresent krrsig:omnipresent ds:omnipresent",
        f"zsk 0 {ALGORITHM} {SIZE} goal:omnipresent dnskey:omnipresent zrrsig:omnipresent",
    ]

    # First make sure the zone is properly signed.
    isctest.log.info(f"basic DNSSEC tests for {zone}")
    isctest.kasp.wait_keymgr_done(ns3, zone)
    isctest.kasp.wait_keymgr_done(ns4, zone)

    keys3 = isctest.kasp.keydir_to_keylist(zone, ns3.identifier)
    ksks3 = [k for k in keys3 if k.is_ksk()]
    zsks3 = [k for k in keys3 if not k.is_ksk()]
    expected3 = isctest.kasp.policy_to_properties(ttl=TTL, keys=keyprops)

    check_dnssec(ns3, zone, keys3, expected3)

    keys4 = isctest.kasp.keydir_to_keylist(zone, ns4.identifier)
    ksks4 = [k for k in keys4 if k.is_ksk()]
    zsks4 = [k for k in keys4 if not k.is_ksk()]
    expected4 = isctest.kasp.policy_to_properties(ttl=TTL, keys=keyprops)

    check_dnssec(ns4, zone, keys4, expected4)

    # Add DNSKEY to RRset.
    newprops = [f"zsk unlimited {ALGORITHM} {SIZE}"]
    extra = isctest.kasp.policy_to_properties(ttl=TTL, keys=newprops)
    extra[0].private = False  # noqa
    extra[0].legacy = True  # noqa

    check_add_zsk(ns3, zone, keys3, expected3, [zsks4[0]], extra, primary=ns5)
    check_add_zsk(ns4, zone, keys4, expected4, [zsks3[0]], extra, primary=ns5)
    check_no_dnssec_in_journal(ns3, zone)
    check_no_dnssec_in_journal(ns4, zone)

    # Remove DNSKEY from RRset.
    check_remove_zsk(ns3, zone, keys3, expected3, [zsks4[0]], extra, primary=ns5)
    check_remove_zsk(ns4, zone, keys4, expected4, [zsks3[0]], extra, primary=ns5)
    check_no_dnssec_in_journal(ns3, zone)
    check_no_dnssec_in_journal(ns4, zone)

    # Add CDNSKEY RRset.
    newprops = [f"ksk unlimited {ALGORITHM} {SIZE}"]
    extra = isctest.kasp.policy_to_properties(ttl=TTL, keys=newprops)
    extra[0].private = False  # noqa
    extra[0].legacy = True  # noqa

    check_add_cdnskey(ns3, zone, keys3, expected3, [ksks4[0]], extra, primary=ns5)
    check_add_cdnskey(ns4, zone, keys4, expected4, [ksks3[0]], extra, primary=ns5)
    check_no_dnssec_in_journal(ns3, zone)
    check_no_dnssec_in_journal(ns4, zone)

    # Remove CDNSKEY RRset.
    check_remove_cdnskey(ns3, zone, keys3, expected3, [ksks4[0]], extra, primary=ns5)
    check_remove_cdnskey(ns4, zone, keys4, expected4, [ksks3[0]], extra, primary=ns5)
    check_no_dnssec_in_journal(ns3, zone)
    check_no_dnssec_in_journal(ns4, zone)

    # Update CDS RRset.
    check_add_cds(ns3, zone, keys3, expected3, [ksks4[0]], extra, primary=ns5)
    check_add_cds(ns4, zone, keys4, expected4, [ksks3[0]], extra, primary=ns5)
    check_no_dnssec_in_journal(ns3, zone)
    check_no_dnssec_in_journal(ns4, zone)

    # Remove CDS RRset.
    check_remove_cds(ns3, zone, keys3, expected3, [ksks4[0]], extra, primary=ns5)
    check_remove_cds(ns4, zone, keys4, expected4, [ksks3[0]], extra, primary=ns5)
    check_no_dnssec_in_journal(ns3, zone)
    check_no_dnssec_in_journal(ns4, zone)
