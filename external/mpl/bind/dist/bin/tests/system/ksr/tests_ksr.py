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
import re
import shutil
import time

import pytest

import isctest
from isctest.kasp import KeyTimingMetadata

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


def ksr(zone, policy, action, options="", raise_on_exception=True, to_file=""):
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

    if to_file:
        with open(to_file, "wb") as f:
            cmd = isctest.run.cmd(
                ksr_command, raise_on_exception=raise_on_exception, stdout=f
            )
    else:
        cmd = isctest.run.cmd(ksr_command, raise_on_exception=raise_on_exception)
    return cmd


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


def check_key_bundle(bundle_keys, bundle_lines, cdnskey=False):
    count = 0
    for key in bundle_keys:
        found = False
        for line in bundle_lines:
            if key.dnskey_equals(line, cdnskey):
                found = True
                count += 1
        assert found

    assert count == len(bundle_keys)
    assert count == len(bundle_lines)


def check_cds_bundle(bundle_keys, bundle_lines, expected_cds):
    count = 0
    for key in bundle_keys:
        found = False
        # the cds of this ksk must be in the ksr
        for line in bundle_lines:
            for alg in expected_cds:
                if key.cds_equals(line, alg.strip()):
                    found = True
                    count += 1
        assert found

    assert count == len(expected_cds) * len(bundle_keys)
    assert count == len(bundle_lines)


def check_rrsig_bundle(bundle_keys, bundle_lines, zone, rrtype, sigend, sigstart):
    count = 0
    for key in bundle_keys:
        found = False
        alg = key.get_metadata("Algorithm")
        expect = f"{zone}. 3600 IN RRSIG {rrtype} {alg} 2 3600 {sigend} {sigstart} {key.tag} {zone}."
        # there must be a signature of this ksk
        for line in bundle_lines:
            rrsig = " ".join(line.split())
            if expect in rrsig:
                found = True
                count += 1
        assert found

    assert count == len(bundle_keys)
    assert count == len(bundle_lines)


def check_keysigningrequest(path, zsks, start, end):
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    line_no = 0

    inception = start
    while inception < end:
        next_bundle = end + 1
        # expect bundle header
        assert f";; KeySigningRequest 1.0 {inception}" in lines[line_no]
        line_no += 1
        bundle_keys = []
        bundle_lines = []
        # expect zsks
        for key in zsks:
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

            # collect keys that should be in this bundle
            # collect lines that should be in this bundle
            bundle_keys.append(key)
            bundle_lines.append(lines[line_no])
            line_no += 1

        check_key_bundle(bundle_keys, bundle_lines)

        inception = next_bundle

    # ksr footer
    assert ";; KeySigningRequest 1.0 generated at" in lines[line_no]
    line_no += 1

    # trailing empty lines
    while line_no < len(lines):
        assert lines[line_no].rstrip() == ""
        line_no += 1

    assert line_no == len(lines)


def check_signedkeyresponse(
    path,
    zone,
    ksks,
    zsks,
    start,
    end,
    refresh,
    cdnskey=True,
    cds="SHA-256",
):
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
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
            if lines[line_no].rstrip() == "":
                line_no += 1
            else:
                break

        # expect bundle header
        assert f";; SignedKeyResponse 1.0 {inception}" in lines[line_no]
        line_no += 1

        # expect ksks
        bundle_keys = []
        bundle_lines = []
        for key in ksks:
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

            # collect keys that should be in this bundle
            # collect lines that should be in this bundle
            bundle_keys.append(key)
            bundle_lines.append(lines[line_no])
            line_no += 1

        check_key_bundle(bundle_keys, bundle_lines)

        # expect zsks
        bundle_keys = []
        bundle_lines = []
        for key in zsks:
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

            # collect keys that should be in this bundle
            # collect lines that should be in this bundle
            bundle_keys.append(key)
            bundle_lines.append(lines[line_no])
            line_no += 1

        check_key_bundle(bundle_keys, bundle_lines)

        # expect rrsig(dnskey)
        bundle_keys = []
        bundle_lines = []
        for key in ksks:
            active = key.get_timing("Activate")
            inactive = key.get_timing("Inactive", must_exist=False)
            if active > inception:
                continue
            if inactive is not None and inception >= inactive:
                continue

            # collect keys that should be in this bundle
            # collect lines that should be in this bundle
            bundle_keys.append(key)
            bundle_lines.append(lines[line_no])
            line_no += 1

        check_rrsig_bundle(bundle_keys, bundle_lines, zone, "DNSKEY", sigend, sigstart)

        # expect cdnskey
        have_cdnskey = False
        if cdnskey:
            bundle_keys = []
            bundle_lines = []
            for key in ksks:
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

                # collect keys that should be in this bundle
                # collect lines that should be in this bundle
                bundle_keys.append(key)
                bundle_lines.append(lines[line_no])
                line_no += 1
                have_cdnskey = True

            check_key_bundle(bundle_keys, bundle_lines, cdnskey=True)

        if have_cdnskey:
            # expect rrsig(cdnskey)
            bundle_keys = []
            bundle_lines = []
            for key in ksks:
                active = key.get_timing("Activate")
                inactive = key.get_timing("Inactive", must_exist=False)
                if active > inception:
                    continue
                if inactive is not None and inception >= inactive:
                    continue

                # collect keys that should be in this bundle
                # collect lines that should be in this bundle
                bundle_keys.append(key)
                bundle_lines.append(lines[line_no])
                line_no += 1

            check_rrsig_bundle(
                bundle_keys, bundle_lines, zone, "CDNSKEY", sigend, sigstart
            )

        # expect cds
        have_cds = False
        if cds != "":
            bundle_keys = []
            bundle_lines = []
            expected_cds = cds.split(",")
            for key in ksks:
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

                # collect keys that should be in this bundle
                # collect lines that should be in this bundle
                bundle_keys.append(key)
                # pylint: disable=unused-variable
                for _arg in expected_cds:
                    bundle_lines.append(lines[line_no])
                    line_no += 1
                    have_cds = True

            check_cds_bundle(bundle_keys, bundle_lines, expected_cds)

        if have_cds:
            # expect rrsig(cds)
            bundle_keys = []
            bundle_lines = []
            for key in ksks:
                active = key.get_timing("Activate")
                inactive = key.get_timing("Inactive", must_exist=False)
                if active > inception:
                    continue
                if inactive is not None and inception >= inactive:
                    continue

                # collect keys that should be in this bundle
                # collect lines that should be in this bundle
                bundle_keys.append(key)
                bundle_lines.append(lines[line_no])
                line_no += 1

            check_rrsig_bundle(bundle_keys, bundle_lines, zone, "CDS", sigend, sigstart)

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
    cmd = ksr("common.test", "common", "foobar", raise_on_exception=False)
    assert "dnssec-ksr: fatal: unknown command 'foobar'" in cmd.err

    # check that 'dnssec-ksr keygen' errors on missing end date
    cmd = ksr("common.test", "common", "keygen", raise_on_exception=False)
    assert "dnssec-ksr: fatal: keygen requires an end date" in cmd.err

    # check that 'dnssec-ksr keygen' errors on zone with csk
    cmd = ksr(
        "csk.test", "csk", "keygen", options="-K ns1 -e +2y", raise_on_exception=False
    )
    assert "dnssec-ksr: fatal: no keys created for policy 'csk'" in cmd.err

    # check that 'dnssec-ksr request' errors on missing end date
    cmd = ksr("common.test", "common", "request", raise_on_exception=False)
    assert "dnssec-ksr: fatal: request requires an end date" in cmd.err

    # check that 'dnssec-ksr sign' errors on missing ksr file
    cmd = ksr(
        "common.test",
        "common",
        "sign",
        options="-K ns1/offline -i now -e +1y",
        raise_on_exception=False,
    )
    assert "dnssec-ksr: fatal: 'sign' requires a KSR file" in cmd.err


def test_ksr_common(ns1):
    # common test cases (1)
    zone = "common.test"
    policy = "common"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +1y -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    cmd = ksr(zone, policy, "keygen", options="-i now -e +1y")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out)
    assert len(zsks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    # in the given key directory
    zskdir = "ns1"
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +1y")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
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
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {now} -e +1y",
        to_file=ksr_fname,
    )
    check_keysigningrequest(ksr_fname, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {now} -e +1y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(skr_fname, zone, ksks, zsks, now, until, refresh)

    # common test cases (2)
    n = 2

    # check that 'dnssec-ksr keygen' selects pregenerated keys for
    # the same time bundle
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {now} -e +1y")
    selected_zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(selected_zsks) == 2
    for index, key in enumerate(selected_zsks):
        assert zsks[index] == key
        isctest.check.file_contents_equal(
            f"{key.path}.private", f"{key.path}.private.backup"
        )
        isctest.check.file_contents_equal(f"{key.path}.key", f"{key.path}.key.backup")
        isctest.check.file_contents_equal(
            f"{key.path}.state", f"{key.path}.state.backup"
        )

    # check that 'dnssec-ksr keygen' generates only necessary keys for
    # overlapping time bundle
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {now} -e +2y -v 1")
    overlapping_zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(overlapping_zsks) == 4

    selected = len(re.findall("Selecting key pair", cmd.err))
    generated = len(re.findall("Generating key pair", cmd.err)) - len(
        re.findall("collide", cmd.err)
    )

    assert selected == 2
    assert generated == 2
    for index, key in enumerate(overlapping_zsks):
        if index < 2:
            assert zsks[index] == key
            isctest.check.file_contents_equal(
                f"{key.path}.private", f"{key.path}.private.backup"
            )
            isctest.check.file_contents_equal(
                f"{key.path}.key", f"{key.path}.key.backup"
            )
            isctest.check.file_contents_equal(
                f"{key.path}.state", f"{key.path}.state.backup"
            )

    # run 'dnssec-ksr keygen' again with verbosity 0
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {now} -e +2y")
    overlapping_zsks2 = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(overlapping_zsks2) == 4
    check_keys(overlapping_zsks2, lifetime)
    for index, key in enumerate(overlapping_zsks2):
        assert overlapping_zsks[index] == key

    # check that 'dnssec-ksr request' creates correct ksr if the
    # interval is shorter
    ksr_fname = f"{zone}.ksr.{n}.shorter"
    ksr(zone, policy, "request", options=f"-K ns1 -i {now} -e +1y", to_file=ksr_fname)
    check_keysigningrequest(ksr_fname, zsks, now, until)

    # check that 'dnssec-ksr request' creates correct ksr with new interval
    until = now + timedelta(days=365 * 2)
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(zone, policy, "request", options=f"-K ns1 -i {now} -e +2y", to_file=ksr_fname)
    check_keysigningrequest(ksr_fname, overlapping_zsks, now, until)

    # check that 'dnssec-ksr request' errors if there are not enough keys
    cmd = ksr(
        zone,
        policy,
        "request",
        options=f"-K ns1 -i {now} -e +3y",
        raise_on_exception=False,
    )
    errmsg = (
        f"dnssec-ksr: fatal: no {zone}/ECDSAP256SHA256 zsk key pair found for bundle"
    )
    assert errmsg in cmd.err

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K ns1/offline -f {ksr_fname} -i {now} -e +2y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(
        skr_fname,
        zone,
        ksks,
        overlapping_zsks,
        now,
        until,
        refresh,
    )

    # add zone
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};"
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    ns1.rndc(f"skr -import {skr_fname} {zone}")

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, overlapping_zsks, policy=policy)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(overlapping_zsks, lifetime, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, overlapping_zsks, offline_ksk=True)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, overlapping_zsks, offline_ksk=True)


def test_ksr_lastbundle(ns1):
    zone = "last-bundle.test"
    policy = "common"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    offset = -timedelta(days=365)
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i -1y -e +1d -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None, offset=offset)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i -1y -e +1d")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(zsks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime, offset=offset)

    # check that 'dnssec-ksr request' creates correct ksr
    then = zsks[0].get_timing("Created") + offset
    until = then + timedelta(days=366)
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {then} -e +1d",
        to_file=ksr_fname,
    )
    check_keysigningrequest(ksr_fname, zsks, then, until)

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {then} -e +1d",
        to_file=skr_fname,
    )
    check_signedkeyresponse(skr_fname, zone, ksks, zsks, then, until, refresh)

    # add zone
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    ns1.rndc(f"skr -import {skr_fname} {zone}")

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, lifetime, offset=offset, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks, offline_ksk=True)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks, offline_ksk=True)

    # check that last bundle warning is logged
    warning = "last bundle in skr, please import new skr file"
    assert f"zone {zone}/IN (signed): zone_rekey: {warning}" in ns1.log


def test_ksr_inthemiddle(ns1):
    zone = "in-the-middle.test"
    policy = "common"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    offset = -timedelta(days=365)
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i -1y -e +1y -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None, offset=offset)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i -1y -e +1y")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(zsks) == 4

    lifetime = timedelta(days=31 * 6)
    check_keys(zsks, lifetime, offset=offset)

    # check that 'dnssec-ksr request' creates correct ksr
    then = zsks[0].get_timing("Created")
    then = then + offset
    until = then + timedelta(days=365 * 2)
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {then} -e +1y",
        to_file=ksr_fname,
    )
    check_keysigningrequest(ksr_fname, zsks, then, until)

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {then} -e +1y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(skr_fname, zone, ksks, zsks, then, until, refresh)

    # add zone
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    ns1.rndc(f"skr -import {skr_fname} {zone}")

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, lifetime, offset=offset, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks, offline_ksk=True)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks, offline_ksk=True)

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
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i {then} -e {until} -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
    assert len(ksks) == 1

    # key generation
    zskdir = "ns1"
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i {then} -e {until}")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(zsks) == 2

    # create request
    now = zsks[0].get_timing("Created")
    then = now + offset
    until = now + end
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {then} -e {until}",
        to_file=ksr_fname,
    )

    # sign request
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {then} -e {until}",
        to_file=skr_fname,
    )

    # add zone
    server.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    server.rndc(f"skr -import {skr_fname} {zone}")

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


def test_ksr_rekey_logs_error(ns1):
    # check that an SKR that is too old logs error
    check_ksr_rekey_logs_error(ns1, "past.test", "common", -63072000, -31536000)
    # check that an SKR that is too new logs error
    check_ksr_rekey_logs_error(ns1, "future.test", "common", 2592000, 31536000)


def test_ksr_unlimited(ns1):
    zone = "unlimited.test"
    policy = "unlimited"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +2y -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
    assert len(ksks) == 1

    check_keys(ksks, None)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +2y")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(zsks) == 1

    lifetime = None
    check_keys(zsks, lifetime)

    # check that 'dnssec-ksr request' creates correct ksr
    now = zsks[0].get_timing("Created")
    until = now + timedelta(days=365 * 4)
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {now} -e +4y",
        to_file=ksr_fname,
    )
    check_keysigningrequest(ksr_fname, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr without cdnskey
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.no-cdnskey.skr.{n}"
    ksr(
        zone,
        "no-cdnskey",
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {now} -e +4y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(
        skr_fname,
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
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.no-cds.skr.{n}"
    ksr(
        zone,
        "no-cds",
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {now} -e +4y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(
        skr_fname,
        zone,
        ksks,
        zsks,
        now,
        until,
        refresh,
        cds="",
    )

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.{policy}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {now} -e +4y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(skr_fname, zone, ksks, zsks, now, until, refresh)

    # add zone
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    ns1.rndc(f"skr -import {skr_fname} {zone}")

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, lifetime, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks, offline_ksk=True)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks, offline_ksk=True)


def test_ksr_twotone(ns1):
    zone = "two-tone.test"
    policy = "two-tone"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +1y -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
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
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +1y")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
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
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {now} -e +1y",
        to_file=ksr_fname,
    )
    check_keysigningrequest(ksr_fname, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -timedelta(days=5)
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {now} -e +1y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(skr_fname, zone, ksks, zsks, now, until, refresh)

    # add zone
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    ns1.rndc(f"skr -import {skr_fname} {zone}")

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
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
    isctest.kasp.check_apex(ns1, zone, ksks, zsks, offline_ksk=True)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks, offline_ksk=True)


def test_ksr_kskroll(ns1):
    zone = "ksk-roll.test"
    policy = "ksk-roll"
    n = 1

    # create ksk
    kskdir = "ns1/offline"
    cmd = ksr(zone, policy, "keygen", options=f"-K {kskdir} -i now -e +1y -o")
    ksks = isctest.kasp.keystr_to_keylist(cmd.out, kskdir)
    assert len(ksks) == 2

    lifetime = timedelta(days=31 * 6)
    check_keys(ksks, lifetime)

    # check that 'dnssec-ksr keygen' pregenerates right amount of keys
    zskdir = "ns1"
    cmd = ksr(zone, policy, "keygen", options=f"-K {zskdir} -i now -e +1y")
    zsks = isctest.kasp.keystr_to_keylist(cmd.out, zskdir)
    assert len(zsks) == 1

    check_keys(zsks, None)

    # check that 'dnssec-ksr request' creates correct ksr
    now = zsks[0].get_timing("Created")
    until = now + timedelta(days=365)
    ksr_fname = f"{zone}.ksr.{n}"
    ksr(
        zone,
        policy,
        "request",
        options=f"-K {zskdir} -i {now} -e +1y",
        to_file=ksr_fname,
    )
    check_keysigningrequest(ksr_fname, zsks, now, until)

    # check that 'dnssec-ksr sign' creates correct skr
    refresh = -432000  # 5 days
    skr_fname = f"{zone}.skr.{n}"
    ksr(
        zone,
        policy,
        "sign",
        options=f"-K {kskdir} -f {ksr_fname} -i {now} -e +1y",
        to_file=skr_fname,
    )
    check_signedkeyresponse(skr_fname, zone, ksks, zsks, now, until, refresh)

    # add zone
    ns1.rndc(
        f"addzone {zone} "
        + "{ type primary; file "
        + f'"{zone}.db"; dnssec-policy {policy}; '
        + "};",
    )

    # import skr
    shutil.copyfile(skr_fname, f"ns1/{skr_fname}")
    ns1.rndc(f"skr -import {skr_fname} {zone}")

    # test zone is correctly signed
    # - check rndc dnssec -status output
    isctest.kasp.check_dnssecstatus(ns1, zone, zsks, policy=policy)
    # - dnssec_verify
    isctest.kasp.check_dnssec_verify(ns1, zone)
    # - check keys
    check_keys(zsks, None, with_state=True)
    # - check apex
    isctest.kasp.check_apex(ns1, zone, ksks, zsks, offline_ksk=True)
    # - check subdomain
    isctest.kasp.check_subdomain(ns1, zone, ksks, zsks, offline_ksk=True)
