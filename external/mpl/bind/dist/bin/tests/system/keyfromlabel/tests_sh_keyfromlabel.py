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
        "*.example.db",
        "*.example.db.signed",
        "K*",
        "dsset-*",
        "keyfromlabel.out.*",
        "pin",
        "pkcs11-tool.out.*",
        "signer.out.*",
    ]
)


def test_keyfromlabel(run_tests_sh):
    run_tests_sh()
