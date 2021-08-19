############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

import os
import pytest


def pytest_configure(config):
    config.addinivalue_line(
        "markers", "dnspython: mark tests that need dnspython to function"
    )
    config.addinivalue_line(
        "markers", "dnspython2: mark tests that need dnspython >= 2.0.0"
    )


def pytest_collection_modifyitems(config, items):
    # pylint: disable=unused-argument,unused-import,too-many-branches
    # pylint: disable=import-outside-toplevel

    # Test for dnspython module
    skip_dnspython = pytest.mark.skip(
        reason="need dnspython module to run")
    try:
        import dns.query  # noqa: F401
    except ModuleNotFoundError:
        for item in items:
            if "dnspython" in item.keywords:
                item.add_marker(skip_dnspython)

    # Test for dnspython >= 2.0.0 module
    skip_dnspython2 = pytest.mark.skip(
        reason="need dnspython >= 2.0.0 module to run")
    try:
        from dns.query import udp_with_fallback  # noqa: F401
    except ImportError:
        for item in items:
            if "dnspython2" in item.keywords:
                item.add_marker(skip_dnspython2)


@pytest.fixture
def named_port(request):
    # pylint: disable=unused-argument
    port = os.getenv("PORT")
    if port is None:
        port = 5301
    else:
        port = int(port)

    return port


@pytest.fixture
def control_port(request):
    # pylint: disable=unused-argument
    port = os.getenv("CONTROLPORT")
    if port is None:
        port = 5301
    else:
        port = int(port)

    return port
