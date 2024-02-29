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

import pytest_custom_markers


# The qmin test is inherently unstable, see GL #904 for details.
@pytest_custom_markers.flaky(max_runs=3)
def test_qmin(run_tests_sh):
    run_tests_sh()
