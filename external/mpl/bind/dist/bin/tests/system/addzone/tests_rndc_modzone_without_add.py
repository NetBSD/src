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
        "ns*/*.nzf*",
        "ns*/*.nzd*",
        "ns1/redirect.db",
        "ns2/new-zones",
        "ns2/redirect.db",
        "ns3/redirect.db",
    ]
)


def test_rndc_modzone_without_add(ns3):
    """
    Confirm "rndc modzone" works for a zone that was not added by "addzone".
    """
    # We begin with a zone that has a normal configuration, and then modify it
    # by rndc modzone. This should succeed and shouldn't cause any disruption.
    # Previously, it triggered an assertion failure unless LMDB was enabled.
    cmd = ns3.rndc(
        'modzone . {type primary; file "redirect.db"; allow-query {none;};};',
        raise_on_exception=False,
    )
    assert cmd.rc == 0

    # Confirm that the modzone took effect in 'rndc showzone'.
    cmd = ns3.rndc("showzone .", raise_on_exception=False)
    assert cmd.rc == 0
    assert 'allow-query { "none"; }' in cmd.out

    # Confirm that 'rndc modzone' still works after the first modzone.
    # This was not the case before as the zone config was incorrectly
    # removed in-memory after the first modzone.
    cmd = ns3.rndc(
        'modzone . {type primary; file "redirect.db"; allow-query {any;};};',
        raise_on_exception=False,
    )
    assert cmd.rc == 0

    # Confirm that the second modzone took effect in 'rndc showzone'.
    cmd = ns3.rndc("showzone .", raise_on_exception=False)
    assert cmd.rc == 0
    assert 'allow-query { "any"; }' in cmd.out
