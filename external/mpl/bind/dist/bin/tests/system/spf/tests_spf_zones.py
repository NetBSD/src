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


@pytest.mark.requires_zones_loaded("ns1")
def test_spf_log(ns1):
    for msg in (
        "zone spf/IN: 'y.spf' found type SPF record but no SPF TXT record found",
        "zone warn/IN: 'y.warn' found type SPF record but no SPF TXT record found",
        "zone spf/IN: loaded serial 0",
        "zone warn/IN: loaded serial 0",
        "zone nowarn/IN: loaded serial 0",
    ):
        assert msg in ns1.log

    for msg in (
        "zone nowarn/IN: 'y.nowarn' found type SPF record but no SPF TXT record found",
        "zone spf/IN: 'spf' found type SPF record but no SPF TXT record found",
        "zone warn/IN: 'warn' found type SPF record but no SPF TXT record found",
        "zone nowarn/IN: 'nowarn' found type SPF record but no SPF TXT record found",
    ):
        assert msg not in ns1.log
