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
        "*.out",
        "rndc.out.*",
        "ns1/K*",
        "ns1/bigserial.db",
        "ns1/dsset-primary.example.",
        "ns1/duplicate.db",
        "ns1/primary.db",
        "ns1/primary.db.signed",
        "ns1/reload.db",
        "ns1/signer.err",
        "ns1/soa.db",
        "ns2/sec.db",
    ]
)


def test_zonechecks(run_tests_sh):
    run_tests_sh()
