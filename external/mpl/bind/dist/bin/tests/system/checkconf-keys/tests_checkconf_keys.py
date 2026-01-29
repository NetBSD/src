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

import os

import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "bad-*.conf",
        "K*.key",
        "K*.private",
        "K*.state",
        "keygen.out.*",
        "named.conf",
        "*.db",
        "ksk/",
        "zsk/",
    ]
)

CHECKCONF = os.environ["CHECKCONF"]


def test_dnssecpolicy_keystore():
    # Good configuration.
    isctest.run.cmd([CHECKCONF, "-k", "named.conf"])

    # Superfluous key file.
    zone = "superfluous-keyfile.kz.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-superfluous-keyfile.conf"], raise_on_exception=False
    )
    assert f"zone '{zone}': wrong number of key files (3, expected 2)" in cmd.out

    # Missing key file.
    zone = "missing-keyfile.kz.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-missing-keyfile.conf"], raise_on_exception=False
    )
    assert f"zone '{zone}': wrong number of key files (1, expected 2)" in cmd.out

    # Mismatch algorithm.
    zone = "bad-algorithm.kz.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-algorithm.conf"], raise_on_exception=False
    )
    keys = isctest.kasp.keydir_to_keylist(zone)
    assert len(keys) == 2
    assert (
        f"zone '{zone}': key file '{zone}/ECDSAP256SHA256/{keys[0].tag}' does not match dnssec-policy alternative-kz"
        in cmd.out
    )
    assert (
        f"zone '{zone}': key file '{zone}/ECDSAP256SHA256/{keys[1].tag}' does not match dnssec-policy alternative-kz"
        in cmd.out
    )
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy alternative-kz key:'ksk algorithm:RSASHA256 length:2048 tag-range:0-65535'"
        in cmd.out
    )
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy alternative-kz key:'zsk algorithm:RSASHA256 length:2048 tag-range:0-65535'"
        in cmd.out
    )

    # Mismatch length
    zone = "bad-length.csk.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-length.conf"], raise_on_exception=False
    )
    keys = isctest.kasp.keydir_to_keylist(zone)
    assert len(keys) == 1
    assert (
        f"zone '{zone}': key file '{zone}/RSASHA256/{keys[0].tag}' does not match dnssec-policy alternative-csk"
        in cmd.out
    )
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy alternative-csk key:'csk algorithm:RSASHA256 length:2048 tag-range:0-65535'"
        in cmd.out
    )

    # Mismatch tag range
    zone = "bad-tagrange.csk.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-tagrange.conf"], raise_on_exception=False
    )
    keys = isctest.kasp.keydir_to_keylist(zone)
    assert len(keys) == 1
    assert (
        f"zone '{zone}': key file '{zone}/ECDSAP256SHA256/{keys[0].tag}' does not match dnssec-policy tagrange-csk"
        in cmd.out
    )
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy tagrange-csk key:'csk algorithm:ECDSAP256SHA256 length:256 tag-range:0-32767'"
        in cmd.out
    )

    # Mismatch role
    zone = "bad-role.kz.example"
    cmd = isctest.run.cmd([CHECKCONF, "-k", "bad-role.conf"], raise_on_exception=False)
    keys = isctest.kasp.keydir_to_keylist(zone)
    assert len(keys) == 2
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy default-kz key:'zsk algorithm:ECDSAP256SHA256 length:256 tag-range:0-65535'"
        in cmd.out
    )

    # Mismatch algorithm (default policy)
    zone = "bad-default-algorithm.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-default-algorithm.conf"], raise_on_exception=False
    )
    keys = isctest.kasp.keydir_to_keylist(zone)
    assert len(keys) == 1
    assert (
        f"zone '{zone}': key file '{zone}/RSASHA256/{keys[0].tag}' does not match dnssec-policy default"
        in cmd.out
    )
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy default key:'csk algorithm:ECDSAP256SHA256 length:256 tag-range:0-65535'"
        in cmd.out
    )

    # Mismatch role (default policy)
    zone = "bad-default-kz.example"
    cmd = isctest.run.cmd(
        [CHECKCONF, "-k", "bad-default-kz.conf"], raise_on_exception=False
    )
    keys = isctest.kasp.keydir_to_keylist(zone)
    assert len(keys) == 2
    assert (
        f"zone '{zone}': key file '{zone}/ECDSAP256SHA256/{keys[0].tag}' does not match dnssec-policy default"
        in cmd.out
    )
    assert (
        f"zone '{zone}': key file '{zone}/ECDSAP256SHA256/{keys[1].tag}' does not match dnssec-policy default"
        in cmd.out
    )
    assert (
        f"zone '{zone}': no key file found matching dnssec-policy default key:'csk algorithm:ECDSAP256SHA256 length:256 tag-range:0-65535'"
        in cmd.out
    )
    assert f"zone '{zone}': wrong number of key files (2, expected 1)" in cmd.out
