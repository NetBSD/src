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

import platform

import isctest.mark


MAX_RUNS = 2 if platform.system() == "FreeBSD" else 1  # GL#3846


@isctest.mark.flaky(max_runs=MAX_RUNS)
def test_nsupdate(run_tests_sh):
    run_tests_sh()
