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

import logging

from . import log
from .vars import ALL, init_vars

if __name__ == "__main__":
    # use root logger as fallback - we're not interested in proper logs here
    log.basic.LOGGERS["conftest"] = logging.getLogger()

    init_vars()
    for name, value in ALL.items():
        print(f"export {name}={value}")
