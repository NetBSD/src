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

import isctest


# The query answer sent with compression disabled should have a size that is
# about twice as large as the answer with compression enabled, while
# maintaining identical content.
def test_names():
    msg = isctest.query.create("example.", "MX")
    # Getting message size with compression enabled
    res_enabled = isctest.query.tcp(msg, ip="10.53.0.1", source="10.53.0.1")
    # Getting message size with compression disabled
    res_disabled = isctest.query.tcp(msg, ip="10.53.0.1", source="10.53.0.2")
    # Checking if responses are identical content-wise
    isctest.check.rrsets_equal(res_enabled.answer, res_disabled.answer)
    # Checking if message with compression disabled is significantly (say 70%) larger
    assert len(res_disabled.wire) > len(res_enabled.wire) * 1.7
