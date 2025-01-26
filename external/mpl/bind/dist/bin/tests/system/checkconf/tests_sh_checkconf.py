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

import pytest

pytestmark = pytest.mark.extra_artifacts(
    [
        "*.out",
        "bad-kasp-keydir1.conf",
        "bad-kasp-keydir2.conf",
        "bad-kasp-keydir3.conf",
        "bad-kasp-keydir4.conf",
        "bad-kasp-keydir5.conf",
        "bad-tsig.conf",
        "badzero.conf",
        "checkconf.out*",
        "diff.out*",
        "good-kasp.conf.in",
        "good-server-christmas-tree.conf",
        "good.conf",
        "good.conf.raw",
        "keys",
    ]
)


def test_checkconf(run_tests_sh):
    run_tests_sh()
