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

from .. import log
from . import algorithms, autoconf, basic, dirs, features, openssl, ports
from .all import ALL

__all__ = [
    "ALL",
    "algorithms",
    "autoconf",
    "basic",
    "dirs",
    "features",
    "init_vars",
    "openssl",
    "ports",
]


def init_vars():
    """Initializes the environment variables."""
    features.init_features()
    algorithms.init_crypto_supported()
    algorithms.set_algorithm_set(os.getenv("ALGORITHM_SET"))
    openssl.parse_openssl_config(ALL["OPENSSL_CONF"])

    os.environ.update(ALL)
    log.debug("setting following env vars: %s", ", ".join([str(key) for key in ALL]))
