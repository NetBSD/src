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

from typing import NamedTuple

import os


class Algorithm(NamedTuple):
    name: str
    number: int
    bits: int

    @classmethod
    def default(cls):
        return cls(
            os.environ["DEFAULT_ALGORITHM"],
            int(os.environ["DEFAULT_ALGORITHM_NUMBER"]),
            int(os.environ["DEFAULT_BITS"]),
        )


RSASHA1 = Algorithm("RSASHA1", 5, 2048)
NSEC3RSASHA1 = Algorithm("NSEC3RSASHA1", 7, 2048)
RSASHA256 = Algorithm("RSASHA256", 8, 2048)
RSASHA512 = Algorithm("RSASHA512", 10, 2048)
ECDSAP256SHA256 = Algorithm("ECDSAP256SHA256", 13, 256)
ECDSAP384SHA384 = Algorithm("ECDSAP384SHA384", 14, 384)
ED25519 = Algorithm("ED25519", 15, 256)
ED448 = Algorithm("ED448", 16, 456)

ALL_ALGORITHMS = [
    RSASHA1,
    NSEC3RSASHA1,
    RSASHA256,
    RSASHA512,
    ECDSAP256SHA256,
    ECDSAP384SHA384,
    ED25519,
    ED448,
]

ALL_ALGORITHMS_BY_NUM = {alg.number: alg for alg in ALL_ALGORITHMS}
