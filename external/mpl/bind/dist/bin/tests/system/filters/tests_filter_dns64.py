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

from filters.common import ARTIFACTS

import isctest

pytestmark = pytest.mark.extra_artifacts(ARTIFACTS)


def test_filter_dns64():
    # This configuration doesn't make sense. The AAAA is wanted by
    # filter-aaaa, but discarded by the dns64 configuration. We just
    # need to ensure that the server keeps running.
    msg = isctest.query.create("aaaa-only.unsigned", "aaaa")
    res = isctest.query.tcp(msg, "10.53.0.5")
    isctest.check.noerror(res)
