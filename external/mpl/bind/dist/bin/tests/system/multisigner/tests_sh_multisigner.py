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
        "*.created",
        "cdnskey.ns*",
        "cds.ns*",
        "dig.out.*",
        "rndc.dnssec.status.out.*",
        "secondary.cdnskey.ns*",
        "secondary.cds.ns*",
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


def test_multisigner(run_tests_sh):
    run_tests_sh()
