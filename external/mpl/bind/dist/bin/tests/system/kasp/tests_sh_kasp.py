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

import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "K*.private",
        "K*.key",
        "K*.state",
        "K*.cmp",
        "*.created",
        "dig.out*",
        "keyevent.out.*",
        "keygen.out.*",
        "keys",
        "published.test*",
        "python.out.*",
        "retired.test*",
        "rndc.dnssec.*.out.*",
        "rndc.zonestatus.out.*",
        "rrsig.out.*",
        "created.key-*",
        "unused.key-*",
        "verify.out.*",
        "zone.out.*",
        "ns*/K*.private",
        "ns*/K*.key",
        "ns*/K*.state",
        "ns*/*.db",
        "ns*/*.db.infile",
        "ns*/*.db.signed",
        "ns*/*.jbk",
        "ns*/*.jnl",
        "ns*/dsset-*",
        "ns*/keygen.out.*",
        "ns*/keys",
        "ns*/ksk",
        "ns*/ksk/K*",
        "ns*/zsk",
        "ns*/zsk",
        "ns*/zsk/K*",
        "ns*/named-fips.conf",
        "ns*/settime.out.*",
        "ns*/signer.out.*",
        "ns*/zones",
        "ns*/policies/*.conf",
        "ns*/*.zsk1",
        "ns*/*.zsk2",
        "ns3/legacy-keys.*",
        "ns3/dynamic-signed-inline-signing.kasp.db.signed.signed",
    ]
)


def test_kasp(run_tests_sh):
    run_tests_sh()
