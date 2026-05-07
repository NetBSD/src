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

pytestmark = pytest.mark.extra_artifacts(
    [
        "dig.out.*",
        "dsset-*",
        "keyfromlabel.err.*",
        "keyfromlabel.out.*",
        "pkcs11-tool.err.*",
        "pkcs11-tool.out.*",
        "signer.out.*",
        "ns*/dig.out.*",
        "ns*/K*",
        "ns*/keygen.out.*",
        "ns*/update.cmd.*",
        "ns*/update.log.*",
        "ns*/verify.out.*",
        "ns*/pin",
        "ns*/zone.*.jbk",
        "ns*/zone.*.jnl",
        "ns*/*.kskid1",
        "ns*/*.kskid2",
        "ns*/*.zskid1",
        "ns*/*.zskid2",
        "ns1/keys",
        "ns1/named.args",
        "ns1/*.example.db",
        "ns1/*.example.db.signed",
        "ns1/*.kasp.db",
        "ns1/*.kasp.db.signed",
        "ns1/*.split.db",
        "ns1/*.split.db.signed",
        "ns1/*.weird.db",
        "ns1/*.weird.db.signed",
        "ns2/keys",
        "ns2/named.args",
        "ns2/*.view*.db",
        "ns2/*.view*.db.signed",
    ]
)


def bootstrap():
    return {
        "ENGINE_ARG": os.getenv("ENGINE_ARG", ""),
    }


@pytest.mark.flaky(max_runs=5)  # GL#4605
def test_enginepkcs11(run_tests_sh):
    run_tests_sh()
