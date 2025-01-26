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
        "badnsec3owner.out",
        "badrecordname.out",
        "dupans.out",
        "dupquestion.out",
        "keyclass.out",
        "malformeddeltype.out",
        "malformedrrsig.out",
        "nametoolong.out",
        "noquestions.out",
        "optwrongname.out",
        "qtypeasanswer.out",
        "questionclass.out",
        "shortquestion.out",
        "shortrecord.out",
        "tsignotlast.out",
        "tsigwrongclass.out",
        "twoquestionnames.out",
        "twoquestiontypes.out",
        "wrongclass.out",
    ]
)


def test_formerr(run_tests_sh):
    run_tests_sh()
