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

import glob
import os
import subprocess

import pytest

from filters.common import ARTIFACTS

import isctest

pytestmark = pytest.mark.extra_artifacts(ARTIFACTS)


# FUTURE: move this to checkconf test - it doesn't need nsX servers
def test_filters_checkconf():
    for filename in glob.glob("conf/good*.conf"):
        isctest.run.cmd([os.environ["CHECKCONF"], filename])
    for filename in glob.glob("conf/bad*.conf"):
        with pytest.raises(subprocess.CalledProcessError):
            isctest.run.cmd([os.environ["CHECKCONF"], filename])
