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

from datetime import datetime, timedelta
import os


# ISO datetime format without msec
fmt = "%Y-%m-%dT%H:%M:%SZ"

# The constants were taken from BIND 9 source code (lib/dns/zone.c)
max_refresh = timedelta(seconds=2419200)  # 4 weeks
max_expires = timedelta(seconds=14515200)  # 24 weeks
now = datetime.utcnow().replace(microsecond=0)
dayzero = datetime.utcfromtimestamp(0).replace(microsecond=0)


# Generic helper functions
def check_expires(expires, min_time, max_time):
    assert expires >= min_time
    assert expires <= max_time


def check_refresh(refresh, min_time, max_time):
    assert refresh >= min_time
    assert refresh <= max_time


def check_loaded(loaded, expected):
    # Sanity check the zone timers values
    assert loaded == expected
    assert loaded < now


def check_zone_timers(loaded, expires, refresh, loaded_exp):
    # Sanity checks the zone timers values
    if expires is not None:
        check_expires(expires, now, now + max_expires)
    if refresh is not None:
        check_refresh(refresh, now, now + max_refresh)
    check_loaded(loaded, loaded_exp)


#
# The output is gibberish, but at least make sure it does not crash.
#
def check_manykeys(name, zone=None):
    # pylint: disable=unused-argument
    assert name == "manykeys"


def zone_mtime(zonedir, name):
    try:
        si = os.stat(os.path.join(zonedir, "{}.db".format(name)))
    except FileNotFoundError:
        return dayzero

    mtime = datetime.utcfromtimestamp(si.st_mtime).replace(microsecond=0)

    return mtime


def test_zone_timers_primary(fetch_zones, load_timers, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]
    zonedir = kwargs["zonedir"]

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        (name, loaded, expires, refresh) = load_timers(zone, True)
        mtime = zone_mtime(zonedir, name)
        check_zone_timers(loaded, expires, refresh, mtime)


def test_zone_timers_secondary(fetch_zones, load_timers, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]
    zonedir = kwargs["zonedir"]

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        (name, loaded, expires, refresh) = load_timers(zone, False)
        mtime = zone_mtime(zonedir, name)
        check_zone_timers(loaded, expires, refresh, mtime)


def test_zone_with_many_keys(fetch_zones, load_zone, **kwargs):
    statsip = kwargs["statsip"]
    statsport = kwargs["statsport"]

    zones = fetch_zones(statsip, statsport)

    for zone in zones:
        name = load_zone(zone)
        if name == "manykeys":
            check_manykeys(name)
