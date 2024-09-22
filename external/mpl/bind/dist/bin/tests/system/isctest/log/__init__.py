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

from .basic import (
    deinit_module_logger,
    deinit_test_logger,
    init_module_logger,
    init_test_logger,
    debug,
    info,
    warning,
    error,
    critical,
)

from .watchlog import LogFile, WatchLogFromStart, WatchLogFromHere
