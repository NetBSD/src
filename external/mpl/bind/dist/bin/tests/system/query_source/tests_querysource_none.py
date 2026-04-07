#!/usr/bin/python3

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
        "ns*/named.pid",
        "ns*/managed-keys.bind*",
    ]
)


def test_querysource_none():
    msg = isctest.query.create("example.", "A", dnssec=False)

    res = isctest.query.udp(msg, "10.53.0.2")
    isctest.check.noerror(res)

    res = isctest.query.udp(msg, "10.53.0.3")
    isctest.check.noerror(res)

    res = isctest.query.udp(msg, "10.53.0.4")
    isctest.check.servfail(res)

    res = isctest.query.udp(msg, "10.53.0.5")
    isctest.check.servfail(res)

    # using a different name below to make sure we don't use the
    # resolver cache

    msg = isctest.query.create("exampletwo.", "A", dnssec=False)

    res = isctest.query.udp(msg, "fd92:7065:b8e:ffff::2")
    isctest.check.noerror(res)

    res = isctest.query.udp(msg, "fd92:7065:b8e:ffff::3")
    isctest.check.noerror(res)

    res = isctest.query.udp(msg, "fd92:7065:b8e:ffff::4")
    isctest.check.servfail(res)

    res = isctest.query.udp(msg, "fd92:7065:b8e:ffff::5")
    isctest.check.servfail(res)
