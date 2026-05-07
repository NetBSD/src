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

from isctest.instance import NamedInstance
from isctest.mark import live_internet_test


@live_internet_test
def test_mirror_root_zone(ns1: NamedInstance):
    """
    This test pulls the root zone from the Internet, so let's only run
    it when CI_ENABLE_LIVE_INTERNET_TESTS is set.
    """
    with ns1.watch_log_from_start() as watch_log:
        # TimeoutError is raised if the line is not found and the test will fail.
        watch_log.wait_for_line("Transfer status: success")
