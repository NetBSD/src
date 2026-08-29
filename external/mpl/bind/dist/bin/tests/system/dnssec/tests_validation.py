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


from dns import rdatatype

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
    ]
)


def test_positive_validation_dname_at_apex():
    # an apex DNAME is signed by the DNSKEY living at the DNAME owner
    # name itself; fetching that key must not be mistaken for a
    # non-advancing alias chain (GL #6176)
    msg = isctest.query.create("a.dname-at-apex-nsec3.example", "A")
    res = isctest.query.tcp(msg, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)
    answers = {(str(rr.name), rr.rdtype) for rr in res.answer}
    assert ("dname-at-apex-nsec3.example.", rdatatype.DNAME) in answers
    assert ("a.example.", rdatatype.A) in answers
