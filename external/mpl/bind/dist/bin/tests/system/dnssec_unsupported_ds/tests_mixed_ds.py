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

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "ns*/dsset-*",
        "ns*/keys",
        "ns*/keys/*.key",
        "ns*/keys/*.private",
        "ns*/trusted.conf",
        "ns*/zones/*.db",
        "ns*/zones/*.db.signed",
    ]
)


def test_mixed_ds():
    msg = isctest.query.create("child.example.", "DNSKEY")
    res = isctest.query.tcp(msg, "10.53.0.4")
    isctest.check.servfail(res)

    msg = isctest.query.create("child.example.", "A")
    res = isctest.query.tcp(msg, "10.53.0.4")
    isctest.check.servfail(res)
