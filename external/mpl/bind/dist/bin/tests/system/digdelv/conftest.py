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

"""
Fixtures shared by the digdelv test modules.
"""

from dataclasses import dataclass

import os

import pytest


@dataclass
class Zsk:
    keyid: str
    keydata: str
    rrcomment: str


@pytest.fixture(name="zsk")
def zsk_fixture():
    """Key id and rdata of the ZSK generated for the example zone."""
    with open("ns2/keyid", encoding="utf-8") as keyid_file:
        keyid = keyid_file.read().strip()
    with open("ns2/keydata", encoding="utf-8") as keydata_file:
        keydata = keydata_file.read().strip()
    return Zsk(
        keyid=keyid,
        keydata=keydata,
        rrcomment=f"; ZSK; alg = {os.environ['DEFAULT_ALGORITHM']} ; key id = {keyid}",
    )
