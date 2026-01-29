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

from collections import ChainMap

# pylint: disable=import-error
from .autoconf import AC_VARS  # type: ignore

# pylint: enable=import-error
from .algorithms import ALG_VARS, CRYPTO_SUPPORTED_VARS
from .basic import BASIC_VARS
from .dirs import DIR_VARS
from .features import FEATURE_VARS
from .openssl import OPENSSL_VARS
from .ports import PORT_VARS


class VarLookup(ChainMap):
    """A dictionary-like structure to coalesce the variables from different
    modules without making a copy (which would prevent updating these values
    from inside the modules). Values which are None are treated as unset when
    iterating."""

    def __init__(self, *maps):
        keys = set()
        for m in maps:
            overlap = keys.intersection(m.keys())
            if overlap:
                raise RuntimeError(f"key(s) are defined multiple times: {overlap}")
            keys = keys.union(m.keys())
        super().__init__(*maps)

    def __setitem__(self, *args, **kwargs):
        raise RuntimeError("read-only structure")

    def keys(self):
        result = set()
        for m in self.maps:
            for key, val in m.items():
                if val is None:  # treat None as unset
                    continue
                result.add(key)
        return list(result)

    def __iter__(self):
        return iter(self.keys())


ALL = VarLookup(
    AC_VARS,
    BASIC_VARS,
    FEATURE_VARS,
    OPENSSL_VARS,
    PORT_VARS,
    DIR_VARS,
    ALG_VARS,
    CRYPTO_SUPPORTED_VARS,
)
