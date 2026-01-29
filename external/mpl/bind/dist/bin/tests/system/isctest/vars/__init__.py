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

import os

from .all import ALL
from .algorithms import init_crypto_supported, set_algorithm_set
from .features import init_features
from .openssl import parse_openssl_config
from .. import log


def init_vars():
    """Initializes the environment variables."""
    init_features()
    init_crypto_supported()
    set_algorithm_set(os.getenv("ALGORITHM_SET"))
    parse_openssl_config(ALL["OPENSSL_CONF"])

    os.environ.update(ALL)
    log.debug("setting following env vars: %s", ", ".join([str(key) for key in ALL]))
