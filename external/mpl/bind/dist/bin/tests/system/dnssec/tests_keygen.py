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

import pytest

import isctest
import isctest.mark

pytestmark = pytest.mark.extra_artifacts(
    [
        "*/K*",
        "*/NSEC*",
        "*/dsset-*",
        "*/*.bk",
        "*/*.conf",
        "*/*.db",
        "*/*.id",
        "*/*.jnl",
        "*/*.jbk",
        "*/*.key",
        "*/*.signed",
        "*/settime.out.*",
        "ans*/ans.run",
        "*/trusted.keys",
        "*/*.bad",
        "*/*.next",
        "*/*.stripped",
        "*/*.tmp",
        "*/*.stage?",
        "*/*.patched",
        "*/*.lower",
        "*/*.upper",
        "*/*.unsplit",
        "kasp.conf",
    ]
)


# run named-checkconf
def checkconf(cfgfile):
    checkconf_cmd = [os.environ.get("CHECKCONF"), "-k", cfgfile]
    return isctest.run.cmd(checkconf_cmd, raise_on_exception=False)


# run dnssec-keygen
def keygen(*args):
    keygen_cmd = [os.environ.get("KEYGEN")]
    keygen_cmd.extend(args)
    return isctest.run.cmd(keygen_cmd, raise_on_exception=False)


def test_keygen_keystore_keydirectory():
    zone = "keystore-keydirectory.example."
    conf = "conf/keystore-keydirectory.conf"
    policy = "csk"

    # named-checkconf should complain about the key-store name.
    result = checkconf(conf)
    assert "name 'key-directory' not allowed" in result.out

    # dnssec-keygen should also complain about the key-store name.
    result = keygen("-k", policy, "-l", conf, zone)
    assert "key-store: duplicate key-store found 'key-directory'" in result.err
