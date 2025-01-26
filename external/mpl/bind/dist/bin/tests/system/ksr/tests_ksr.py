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
import difflib
import os
import shutil
import time
from typing import List, Optional

import pytest

import isctest
from isctest.kasp import (
    Key,
    KeyTimingMetadata,
)

pytestmark = pytest.mark.extra_artifacts(
    [
        "K*",
        "common.test.*",
        "future.test.*",
        "in-the-middle.test.*",
        "ksk-roll.test.*",
        "last-bundle.test.*",
        "past.test.*",
        "two-tone.test.*",
        "unlimited.test.*",
        "ns1/K*",
        "ns1/_default.nzd",
        "ns1/_default.nzf",
        "ns1/common.test.db",
        "ns1/common.test.db.jbk",
        "ns1/common.test.db.signed",
        "ns1/common.test.db.signed.jnl",
        "ns1/common.test.skr.2",
        "ns1/future.test.db",
        "ns1/future.test.db.jbk",
        "ns1/future.test.db.signed",
        "ns1/future.test.skr.1",
        "ns1/in-the-middle.test.db",
        "ns1/in-the-middle.test.db.jbk",
        "ns1/in-the-middle.test.db.signed",
        "ns1/in-the-middle.test.db.signed.jnl",
        "ns1/in-the-middle.test.skr.1",
        "ns1/keydir",
        "ns1/ksk-roll.test.db",
        "ns1/ksk-roll.test.db.jbk",
        "ns1/ksk-roll.test.db.signed",
        "ns1/ksk-roll.test.db.signed.jnl",
        "ns1/ksk-roll.test.skr.1",
        "ns1/last-bundle.test.db",
        "ns1/last-bundle.test.db.jbk",
        "ns1/last-bundle.test.db.signed",
        "ns1/last-bundle.test.db.signed.jnl",
        "ns1/last-bundle.test.skr.1",
        "ns1/offline",
        "ns1/past.test.db",
        "ns1/past.test.db.jbk",
        "ns1/past.test.db.signed",
        "ns1/past.test.skr.1",
        "ns1/two-tone.test.db",
        "ns1/two-tone.test.db.jbk",
        "ns1/two-tone.test.db.signed",
        "ns1/two-tone.test.db.signed.jnl",
        "ns1/two-tone.test.skr.1",
        "ns1/unlimited.test.db",
        "ns1/unlimited.test.db.jbk",
        "ns1/unlimited.test.db.signed",
        "ns1/unlimited.test.db.signed.jnl",
        "ns1/unlimited.test.unlimited.skr.1",
    ]
)


def between(value, start, end):
    if value is None or start is None or end is None:
        return False

    return start < value < end


def check_file_contents_equal(file1, file2):
    def normalize_line(line):
        # remove trailing&leading whitespace and replace multiple whitespaces
        return " ".join(line.split())

    def read_lines(file_path):
        with open(file_path, "r", encoding="utf-8") as file:
            return [normalize_line(line) for line in file.readlines()]

    lines1 = read_lines(file1)
    lines2 = read_lines(file2)

    differ = difflib.Differ()
    diff = differ.compare(lines1, lines2)

    for line in diff:
        assert not line.startswith("+ ") and not line.startswith(
            "- "
        ), f'file contents of "{file1}" and "{file2}" differ'


def keystr_to_keylist(keystr: str, keydir: Optional[str] = None) -> List[Key]:
    return [Key(name, keydir) for name in keystr.split()]


def ksr(zone, policy, action, options="", raise_on_exception=True):
    ksr_command = [
        os.environ.get("KSR"),
        "-l",
        "ns1/named.conf",
        "-k",
        policy,
        *options.split(),
        action,
        zone,
    ]

    out = isctest.run.cmd(
        ksr_command, log_stdout=True, raise_on_exception=raise_on_exception
    )
    return out.stdout.decode("utf-8"), out.stderr.decode("utf-8")


def check_keys(
    keys,
    lifetime,
    alg=os.environ["DEFAULT_ALGORITHM_NUMBER"],
    size=os.environ["DEFAULT_BITS"],
    offset=0,
    with_state=False,
):
    # Check keys that were created.
    num = 0

    now = KeyTimingMetadata.now()

    for key in keys:
        # created: from keyfile plus offset
        created = key.get_timing("Created") + offset

        # active: retired previous key
        active = created
        if num > 0 and retired is not None:
            active = retired

        # published: dnskey-ttl + publish-safety + propagation
        published = active - timedelta(hours=2, minutes=5)

        # retired: zsk-lifetime
        if lifetime is not None:
            retired = active + lifetime

            if key.is_ksk():
                # removed: ttlds + retire-safety + parent-propagation
                removed = retired + timedelta(days=1, hours=2)
            else:
                # removed: ttlsig + retire-safety + sign-delay + propagation
                removed = retired + timedelta(days=10, hours=1, minutes=5)
        else:
            retired = None
            removed = None

        goal = "hidden"
        state_dnskey = "hidden"
        state_zrrsig = "hidden"
        state_krrsig = "hidden"
        state_ds = "hidden"
        if retired is None or between(now, published, retired):
            goal = "omnipresent"
            pubdelay = published + timedelta(hours=2, minutes=5)
            signdelay = active + timedelta(days=10, hours=1, minutes=5)

            if between(now, published, pubdelay):
                state_dnskey = "rumoured"
                state_krrsig = "rumoured"
            else:
                state_dnskey = "omnipresent"
                state_krrsig = "omnipresent"

            if key.is_ksk():
                state_ds = "hidden"
            else:
                if between(now, active, signdelay):
                    state_zrrsig = "rumoured"
                else:
                    state_zrrsig = "omnipresent"

        with open(key.statefile, "r", encoding="utf-8") as file:
            metadata = file.read()
            assert f"Algorithm: {alg}" in metadata
            assert f"Length: {size}" in metadata

            if key.is_ksk():
                assert "KSK: yes" in metadata
            else:
                assert "KSK: no" in metadata

            if key.is_zsk():
                assert "ZSK: yes" in metadata
            else:
                assert "ZSK: no" in metadata

            assert f"Published: {published}" in metadata
            assert f"Active: {active}" in metadata

            if lifetime is not None:
                assert f"Retired: {retired}" in metadata
                assert f"Removed: {removed}" in metadata
                assert f"Lifetime: {int(lifetime.total_seconds())}" in metadata
            else:
                assert "Lifetime: 0" in metadata
                assert "Retired:" not in metadata
                assert "Removed:" not in metadata

            if with_state:
                assert f"GoalState: {goal}" in metadata
                assert f"DNSKEYState: {state_dnskey}" in metadata

                if key.is_ksk():
                    assert f"KRRSIGState: {state_krrsig}" in metadata
                    assert f"DSState: {state_ds}" in metadata
                else:
                    assert "KRRSIGState:" not in metadata
                    assert "DSState:" not in metadata

                if key.is_zsk():
                    assert f"ZRRSIGState: {state_zrrsig}" in metadata
                else:
                    assert "ZRRSIGState:" not in metadata

        num += 1


def check_keysigningrequest(out, zsks, start, end):
    lines = out.split("\n")
    line_no = 0

    inception = start
    while inception < end:
        next_bundle = end + 1
        # expect bundle header
        assert f";; KeySigningRequest 1.0 {inception}" in lines[line_no]
        line_no += 1
        # expect zsks
        for key in sorted(zsks):
            published = key.get_timing("Publish")
            if between(published, inception, next_bundle):
                next_bundle = published

            removed = key.get_timing("Delete", must_exist=False)
            if between(removed, inception, next_bundle):
                next_bundle = removed

            if published > inception:
                continue
            if removed is not None and inception >= removed:
                continue

            # this zsk must be in the ksr
            assert key.dnskey_equals(lines[line_no])
            line_no += 1

        inception = next_bundle

    # ksr footer
    assert ";; KeySigningRequest 1.0 generated at" in lines[line_no]
    line_no += 1

    # trailing empty lines
    while line_no < len(lines):
        assert lines[line_no] == ""
        line_no += 1

    assert line_no == len(lines)


def check_signedkeyresponse(
    out,
    zone,
    ksks,
    zsks,
    start,
    end,
    refresh,
    cdnskey=True,
    cds="SHA-256",
):
    lines = out.split("\n")
    line_no = 0
    next_bundle = end + 1

    inception = start
    while inception < end:
        # A single signed key response may consist of:
        # ;; SignedKeyResponse (header)
        # ;; DNSKEY 257 (one per published key in ksks)
        # ;; DNSKEY 256 (one per published key in zsks)
        # ;; RRSIG(DNSKEY) (one per active key in ksks)
        # ;; CDNSKEY (one per published key in ksks)
        # ;; RRSIG(CDNSKEY) (one per active key in ksks)
        # ;; CDS (one per published key in ksks)
        # ;; RRSIG(CDS) (one per active key in ksks)

        sigstart = inception - timedelta(hours=1)  # clockskew
        sigend = inception + timedelta(days=14)  # sig-validity
        next_bundle = sigend + refresh

        # ignore empty lines
        while line_no < len(lines):
            if lines[line_no] == "":
                line_no += 1
            else:
                break

        # expect bundle header
        assert f";; SignedKeyResponse 1.0 {inception}" in lines[line_no]
        line_no += 1

        # expect ksks
        for key in sorted(ksks):
            published = key.get_timing("Publish")
            if between(published, inception, next_bundle):
                next_bundle = published

            removed = key.get_timing("Delete", must_exist=False)

            if published > inception:
                continue
            if removed is not None and inception >= removed:
                continue

            if between(removed, inception, next_bundle):
                next_bundle = removed

            # this ksk must be in the ksr
            assert key.dnskey_equals(lines[line_no])
            line_no += 1

        # expect zsks
        for key in sorted(zsks):
            published = key.get_timing("Publish")
            if between(published, inception, next_bundle):
                next_bundle = published

            removed = key.get_timing("Delete", must_exist=False)
            if between(removed, inception, next_bundle):
                next_bundle = removed

            if published > inception:
                continue
            if removed is not None and inception >= removed:
                continue

            # this zsk must be in the ksr
            assert key.dnskey_equals(lines[line_no])
            line_no += 1

        # expect rrsig(dnskey)
        for key in sorted(ksks):
            active = key.get_timing("Activate")
            inactive = key.get_timing("Inactive", must_exist=False)
            if active > inception:
                continue
            if inactive is not None and inception >= inactive:
                continue

            # there must be a signature of this ksk
            alg = key.get_metadata("Algorithm")
            expect = f"{zone}. 3600 IN RRSIG DNSKEY {alg} 2 3600 {sigend} {sigstart} {key.tag} {zone}."
            rrsig = " ".join(lines[line_no].split())
            assert expect in rrsig
            line_no += 1

        # expect cdnskey
        have_cdnskey = False
        if cdnskey:
            for key in sorted(ksks):
                published = key.get_timing("SyncPublish")
                if between(published, inception, next_bundle):
                    next_bundle = published

                removed = key.get_timing("SyncDelete", must_exist=False)
                if between(removed, inception, next_bundle):
                    next_bundle = removed

                if published > inception:
                    continue
                if removed is not None and inception >= removed:
                    continue

                # the cdnskey of this ksk must be in the ksr
                assert key.dnskey_equals(lines[line_no], cdnskey=True)
                line_no += 1
                have_cdnskey = True

        if have_cdnskey:
            # expect rrsig(cdnskey)
            for key in sorted(ksks):
                active = key.get_timing("Activate")
                inactive = key.get_timing("Inactive", must_exist=False)
                if active > inception:
                    continue
                if inactive is not None and inception >= inactive:
                    continue

                # there must be a signature of this ksk
                alg = key.get_metadata("Algorithm")
                expect = f"{zone}. 3600 IN RRSIG CDNSKEY {alg} 2 3600 {sigend} {sigstart} {key.tag} {zone}."
                rrsig = " ".join(lines[line_no].split())
                assert expect in rrsig
                line_no += 1

        # expect cds
        have_cds = False
        if cds != "":
            for key in sorted(ksks):
                published = key.get_timing("SyncPublish")
                if between(published, inception, next_bundle):
                    next_bundle = published

                removed = key.get_timing("SyncDelete", must_exist=False)
                if between(removed, inception, next_bundle):
                    next_bundle = removed

                if published > inception:
                    continue
                if removed is not None and inception >= removed:
                    continue

                # the cds of this ksk must be in the ksr
                expected_cds = cds.split(",")
                for alg in expected_cds:
                    assert key.cds_equals(lines[line_no], alg.strip())
                    line_no += 1
                    have_cds = True

        if have_cds:
            # expect rrsig(cds)
            for key in sorted(ksks):
                active = key.get_timing("Activate")
                inactive = key.get_timing("Inactive", must_exist=False)
                if active > inception:
                    continue
                if inactive is not None and inception >= inactive:
                    continue

                # there must be a signature of this ksk
                alg = key.get_metadata("Algorithm")
                expect = f"{zone}. 3600 IN RRSIG CDS {alg} 2 3600 {sigend} {sigstart} {key.tag} {zone}."
                rrsig = " ".join(lines[line_no].split())
                assert expect in rrsig
                line_no += 1

        inception = next_bundle

    # skr footer
    assert ";; SignedKeyResponse 1.0 generated at" in lines[line_no]
    line_no += 1

    # trailing empty lines
    while line_no < len(lines):
        assert lines[line_no] == ""
        line_no += 1

    assert line_no == len(lines)


def test_ksr_errors():
    # check that 'dnssec-ksr' errors on unknown action
    _, err = ksr("common.test", "common", "foobar", raise_on_exception=False)
    assert "dnssec-ksr: fatal: unknown command 'foobar'" in err

    # check that 'dnssec-ksr keygen' errors on missing end date
    _, err = ksr("common.test", "common", "keygen", raise_on_exception=False)
    assert "dnssec-ksr: fatal: keygen requires an end date" in err

    # check that 'dnssec-ksr keygen' errors on zone with csk
    _, err = ksr(
        "csk.test", "csk", "keygen", options="-K ns1 -e +2y", raise_on_exception=False
    )
    assert "dnssec-ksr: fatal: no keys created for policy 'csk'" in err

    # check that 'dnssec-ksr request' errors on missing end date
    _, err = ksr("common.test", "common", "request", raise_on_exception=False)
    assert "dnssec-ksr: fatal: request requires an end date" in err

    # check that 'dnssec-ksr sign' errors on missing ksr file
    _, err = ksr(
        "common.test",
        "common",
        "sign",
        options="-K ns1/offline -i now -e +1y",
        raise_on_exception=False,
    )
    assert "dnssec-ksr: fatal: 'sign' requires a KSR file" in err


def test_ksr_common(servers):
    # common test cases (1)
    zone = "common.test"
    policy = "common"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +1y -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    out, _ = ksr(zone, policy, "keygen", options="-i now -e +1y")
    zsks = keystr_to_keylist(out)
    assert len(zsks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    # in the given key directory
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +1y")
    zsks = keystr_to_keylist(out, zskdir)
    assert len(zsks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime)

    for key in zsks:
        privatefile = f"{key.path}.private"
        keyfile = f"{key.path}.key"
        statefile = f"{key.path}.state"
        shutil.copyfile(privatefile, f"{privatefile}.backup")
        shutil.copyfile(keyfile, f"{keyfile}.backup")
        shutil.copyfile(statefile, f"{statefile}.backup")

    # check that 'dnssec-ksr request' creates correct ksr
    now = zsks[0].get_timing("Created")
    until = now + timedelta(days=365)
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {now} -e +1y")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {now} -e +1y"
    )

    fname = f"{zone}.skr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(out, zone, ksks, zsks, now, until, refresh)

    # common test cases (2)
    n = 2

    # check that 'dnssec-ksr keygen' selects pregenerated keys for
    # the same time bundle
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {now} -e +1y")
    selected_zsks = keystr_to_keylist(out, zskdir)
    assert len(selected_zsks) == 2
    for index, key in enumerate(selected_zsks):
        assert zsks[index] == key
        check_file_contents_equal(f"{key.path}.private", f"{key.path}.private.backup")
        check_file_contents_equal(f"{key.path}.key", f"{key.path}.key.backup")
        check_file_contents_equal(f"{key.path}.state", f"{key.path}.state.backup")

    # check that 'dnssec-ksr keygen' generates only necessary keys for
    # overlapping time bundle
    out, err = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {now} -e +2y -v 1")
    overlapping_zsks = keystr_to_keylist(out, zskdir)
    assert len(overlapping_zsks) == 4

    verbose = err.split()
    selected = 0
    generated = 0
    for output in verbose:
        if "Selecting" in output:
            selected += 1
        if "Generating" in output:
            generated += 1
    assert selected == 2
    assert generated == 2
    for index, key in enumerate(overlapping_zsks):
        if index < 2:
            assert zsks[index] == key
            check_file_contents_equal(
                f"{key.path}.private", f"{key.path}.private.backup"
            )
            check_file_contents_equal(f"{key.path}.key", f"{key.path}.key.backup")
            check_file_contents_equal(f"{key.path}.state", f"{key.path}.state.backup")

    # run 'dnssec-ksr keygen' again with verbosity 0
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {now} -e +2y")
    overlapping_zsks2 = keystr_to_keylist(out, zskdir)
    assert len(overlapping_zsks2) == 4
    check_keys(overlapping_zsks2, lifetime)
    for index, key in enumerate(overlapping_zsks2):
        assert overlapping_zsks[index] == key

    # check that 'dnssec-ksr request' creates correct ksr if the
    # interval is shorter
    out, _ = ksr(zone, policy, "request", options=f"-K ns1 -i {now} -e +1y")

    fname = f"{zone}.ksr.{n}.shorter"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, now, until)

    # check that 'dnssec-ksr request' creates correct ksr with new interval
    out, _ = ksr(zone, policy, "request", options=f"-K ns1 -i {now} -e +2y")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    until = now + timedelta(days=365 * 2)
    check_keysigningrequest(out, overlapping_zsks, now, until)

    # check that 'dnssec-ksr request' errors if there are not enough keys
    _, err = ksr(
        zone,
        policy,
        "request",
        options=f"-K ns1 -i {now} -e +3y",
        raise_on_exception=False,
    )
    error = f"no {zone}/ECDSAP256SHA256 zsk key pair found for bundle"
    assert f"dnssec-ksr: fatal: {error}" in err

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K ns1/offline -f {fname} -i {now} -e +2y"
    )

    fname = f"{zone}.skr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(
        out,
        zone,
        ksks,
        overlapping_zsks,
        now,
        until,
        refresh,
    )

    # add zone
    ns1 = servers["ns1"]
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(fname, f"ns1/{fname}")
    ns1.rndc(f"skr -import {fname} {zone}", log=False)

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, overlapping_zsks, policy=policy)
    # - zone is signed
    isctest.kasp.check_zone_is_signed(ns1, zone)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(overlapping_zsks, lifetime, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, overlapping_zsks)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, overlapping_zsks)


def test_ksr_lastbundle(servers):
    zone = "last-bundle.test"
    policy = "common"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    offset = -timedelta(days=365)
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i -1y -e +1d -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None, offset=offset)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i -1y -e +1d")
    zsks = keystr_to_keylist(out, zskdir)
    assert len(zsks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime, offset=offset)

    # check that 'dnssec-ksr request' creates correct ksr
    then = zsks[0].get_timing("Created") + offset
    until = then + timedelta(days=366)
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {then} -e +1d")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, then, until)

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {then} -e +1d"
    )

    fname = f"{zone}.skr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(out, zone, ksks, zsks, then, until, refresh)

    # add zone
    ns1 = servers["ns1"]
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(fname, f"ns1/{fname}")
    ns1.rndc(f"skr -import {fname} {zone}", log=False)

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - zone is signed
    isctest.kasp.check_zone_is_signed(ns1, zone)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, lifetime, offset=offset, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks)

    # check that last bundle warning is logged
    warning = "last bundle in skr, please import new skr file"
    assert f"zone {zone}/IN (signed): zone_rekey: {warning}" in ns1.log


def test_ksr_inthemiddle(servers):
    zone = "in-the-middle.test"
    policy = "common"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    offset = -timedelta(days=365)
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i -1y -e +1y -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None, offset=offset)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i -1y -e +1y")
    zsks = keystr_to_keylist(out, zskdir)
    assert len(zsks) == 4

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime, offset=offset)

    # check that 'dnssec-ksr request' creates correct ksr
    then = zsks[0].get_timing("Created")
    then = then + offset
    until = then + timedelta(days=365 * 2)
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {then} -e +1y")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, then, until)

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {then} -e +1y"
    )

    fname = f"{zone}.skr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(out, zone, ksks, zsks, then, until, refresh)

    # add zone
    ns1 = servers["ns1"]
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(fname, f"ns1/{fname}")
    ns1.rndc(f"skr -import {fname} {zone}", log=False)

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - zone is signed
    isctest.kasp.check_zone_is_signed(ns1, zone)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, lifetime, offset=offset, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks)

    # check that no last bundle warning is logged
    warning = "last bundle in skr, please import new skr file"
    assert f"zone {zone}/IN (signed): zone_rekey: {warning}" not in ns1.log


def check_ksr_rekey_logs_error(server, zone, policy, offset, end):
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    now = KeyTimingMetadata.now()
    then = now + offset
    until = now + end
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i {then} -e {until} -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 1

    # key generation
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {then} -e {until}")
    zsks = keystr_to_keylist(out, zskdir)
    assert len(zsks) == 2

    # create request
    now = zsks[0].get_timing("Created")
    then = now + offset
    until = now + end
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {then} -e {until}")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    # sign request
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {then} -e {until}"
    )

    fname = f"{zone}.skr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    # add zone
    server.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(fname, f"ns1/{fname}")
    server.rndc(f"skr -import {fname} {zone}", log=False)

    # test that rekey logs error
    time_remaining = 10
    warning = "no available SKR bundle"
    line = f"zone {zone}/IN (signed): zone_rekey failure: {warning}"
    while time_remaining > 0:
        if line not in server.log:
            time_remaining -= 1
            time.sleep(1)
        else:
            break
    assert line in server.log


def test_ksr_rekey_logs_error(servers):
    # check that an SKR that is too old logs error
    check_ksr_rekey_logs_error(
        servers["ns1"], "past.test", "common", -63072000, -31536000
    )
    # check that an SKR that is too new logs error
    check_ksr_rekey_logs_error(
        servers["ns1"], "future.test", "common", 2592000, 31536000
    )


def test_ksr_unlimited(servers):
    zone = "unlimited.test"
    policy = "unlimited"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +2y -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +2y")
    zsks = keystr_to_keylist(out, zskdir)
    assert len(zsks) == 1

    lifetime = None
    check_keys(zsks, lifetime)

    # check that 'dnssec-ksr request' creates correct ksr
    now = zsks[0].get_timing("Created")
    until = now + timedelta(days=365 * 4)
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {now} -e +4y")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr without cdnskey
    out, _ = ksr(
        zone, "no-cdnskey", "sign", options=f"-K {kskdir} -f {fname} -i {now} -e +4y"
    )

    skrfile = f"{zone}.no-cdnskey.skr.{n}"
    with open(skrfile, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(
        out,
        zone,
        ksks,
        zsks,
        now,
        until,
        refresh,
        cdnskey=False,
        cds="SHA-1, SHA-256, SHA-384",
    )

    # check that 'dnssec-ksr sign' creates correct skr without cds
    out, _ = ksr(
        zone, "no-cds", "sign", options=f"-K {kskdir} -f {fname} -i {now} -e +4y"
    )

    skrfile = f"{zone}.no-cds.skr.{n}"
    with open(skrfile, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(
        out,
        zone,
        ksks,
        zsks,
        now,
        until,
        refresh,
        cds="",
    )

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {now} -e +4y"
    )

    skrfile = f"{zone}.{policy}.skr.{n}"
    with open(skrfile, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(out, zone, ksks, zsks, now, until, refresh)

    # add zone
    ns1 = servers["ns1"]
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(skrfile, f"ns1/{skrfile}")
    ns1.rndc(f"skr -import {skrfile} {zone}", log=False)

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - zone is signed
    isctest.kasp.check_zone_is_signed(ns1, zone)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, lifetime, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks)


def test_ksr_twotone(servers):
    zone = "two-tone.test"
    policy = "two-tone"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +1y -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 2

    ksks_defalg = []
    ksks_altalg = []
    for ksk in ksks:
        alg = ksk.get_metadata("Algorithm")
        if alg == os.environ.get("DEFAULT_ALGORITHM_NUMBER"):
            ksks_defalg.append(ksk)
        elif alg == os.environ.get("ALTERNATIVE_ALGORITHM_NUMBER"):
            ksks_altalg.append(ksk)

    assert len(ksks_defalg) == 1
    assert len(ksks_altalg) == 1

    check_keys(ksks_defalg, None)

    alg = os.environ.get("ALTERNATIVE_ALGORITHM_NUMBER")
    size = os.environ.get("ALTERNATIVE_BITS")
    check_keys(ksks_altalg, None, alg, size)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +1y")
    zsks = keystr_to_keylist(out, zskdir)
    # First algorithm keys have a lifetime of 3 months, so there should
    # be 4 created keys. Second algorithm keys have a lifetime of 5
    # months, so there should be 3 created keys.  While only two time
    # bundles of 5 months fit into one year, we need to create an extra
    # key for the remainder of the bundle. So 7 in total.
    assert len(zsks) == 7

    zsks_defalg = []
    zsks_altalg = []
    for zsk in zsks:
        alg = zsk.get_metadata("Algorithm")
        if alg == os.environ.get("DEFAULT_ALGORITHM_NUMBER"):
            zsks_defalg.append(zsk)
        elif alg == os.environ.get("ALTERNATIVE_ALGORITHM_NUMBER"):
            zsks_altalg.append(zsk)

    assert len(zsks_defalg) == 4
    assert len(zsks_altalg) == 3

    lifetime = timedelta(days=31 * 3)
    check_keys(zsks_defalg, lifetime)

    alg = os.environ.get("ALTERNATIVE_ALGORITHM_NUMBER")
    size = os.environ.get("ALTERNATIVE_BITS")
    lifetime = timedelta(days=31 * 5)
    check_keys(zsks_altalg, lifetime, alg, size)

    # check that 'dnssec-ksr request' creates correct ksr
    now = zsks[0].get_timing("Created")
    until = now + timedelta(days=365)
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {now} -e +1y")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {now} -e +1y"
    )

    skrfile = f"{zone}.skr.{n}"
    with open(skrfile, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -timedelta(days=5)
    check_signedkeyresponse(out, zone, ksks, zsks, now, until, refresh)

    # add zone
    ns1 = servers["ns1"]
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(skrfile, f"ns1/{skrfile}")
    ns1.rndc(f"skr -import {skrfile} {zone}", log=False)

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - zone is signed
    isctest.kasp.check_zone_is_signed(ns1, zone)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    lifetime = timedelta(days=31 * 3)
    check_keys(zsks_defalg, lifetime, with_state=True)

    alg = os.environ.get("ALTERNATIVE_ALGORITHM_NUMBER")
    size = os.environ.get("ALTERNATIVE_BITS")
    lifetime = timedelta(days=31 * 5)
    check_keys(zsks_altalg, lifetime, alg, size, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks)


def test_ksr_kskroll(servers):
    zone = "ksk-roll.test"
    policy = "ksk-roll"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +1y -o")
    ksks = keystr_to_keylist(out, kskdir)
    assert len(ksks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(ksks, lifetime)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    out, _ = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +1y")
    zsks = keystr_to_keylist(out, zskdir)
    assert len(zsks) == 1

    check_keys(zsks, None)

    # check that 'dnssec-ksr request' creates correct ksr
    now = zsks[0].get_timing("Created")
    until = now + timedelta(days=365)
    out, _ = ksr(zone, policy, "request", options=f"-K {zskdir} -i {now} -e +1y")

    fname = f"{zone}.ksr.{n}"
    with open(fname, "w", encoding="utf-8") as file:
        file.write(out)

    check_keysigningrequest(out, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr
    out, _ = ksr(
        zone, policy, "sign", options=f"-K {kskdir} -f {fname} -i {now} -e +1y"
    )

    skrfile = f"{zone}.skr.{n}"
    with open(skrfile, "w", encoding="utf-8") as file:
        file.write(out)

    refresh = -432000  # 5 days
    check_signedkeyresponse(out, zone, ksks, zsks, now, until, refresh)

    # add zone
    ns1 = servers["ns1"]
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
        log=False,
    )

    # import skr
    shutil.copyfile(skrfile, f"ns1/{skrfile}")
    ns1.rndc(f"skr -import {skrfile} {zone}", log=False)

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - zone is signed
    isctest.kasp.check_zone_is_signed(ns1, zone)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, None, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks)
