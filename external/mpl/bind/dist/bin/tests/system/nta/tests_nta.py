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

from re import compile as Re

import os
import time

import isctest


def active(blob):
    return len([x for x in blob.splitlines() if " expiry" in x])


# global start-time variable
# pylint: disable=global-statement
START = 0


def test_initial():
    m = isctest.query.create("a.bogus.example.", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.servfail(res)

    m = isctest.query.create("badds.example.", "SOA")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.servfail(res)

    m = isctest.query.create("a.secure.example.", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)


def test_nta_validate_except(servers):
    ns4 = servers["ns4"]
    response = ns4.rndc("secroots -")
    assert Re("^corp: permanent") in response.out

    # check insecure local domain works with validate-except
    m = isctest.query.create("www.corp", "NS")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)


def test_nta_bogus_lifetimes(servers):
    ns4 = servers["ns4"]

    # no nta lifetime specified:
    response = ns4.rndc("nta -l '' foo", raise_on_exception=False)
    assert "'nta' failed: bad ttl" in response.err

    # bad nta lifetime:
    response = ns4.rndc("nta -l garbage foo", raise_on_exception=False)
    assert "'nta' failed: bad ttl" in response.err

    # excessive nta lifetime:
    response = ns4.rndc("nta -l 7d1h foo", raise_on_exception=False)
    assert "'nta' failed: out of range" in response.err


def test_nta_install(servers):
    global START

    ns4 = servers["ns4"]
    ns4.rndc("nta -f -l 20s bogus.example")
    ns4.rndc("nta badds.example")

    # NTAs should persist after reconfig
    ns4.reconfigure()

    response = ns4.rndc("nta -d")
    assert len(response.out.splitlines()) == 3

    ns4.rndc("nta secure.example")
    ns4.rndc("nta fakenode.secure.example")
    with ns4.watch_log_from_here() as watcher:
        ns4.rndc("reload")
        watcher.wait_for_line("all zones loaded")

    response = ns4.rndc("nta -d")
    assert len(response.out.splitlines()) == 5

    START = time.time()


def test_nta_behavior(servers):
    assert START, "test_nta_behavior must be run as part of the full NTA test"

    m = isctest.query.create("a.bogus.example.", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    m = isctest.query.create("badds.example.", "SOA")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    m = isctest.query.create("a.secure.example.", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    m = isctest.query.create("a.fakenode.secure.example.", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noadflag(res)

    ns4 = servers["ns4"]
    response = ns4.rndc("secroots -")
    assert Re("^bogus.example: expiry") in response.out
    assert Re("^badds.example: expiry") in response.out
    assert Re("^secure.example: expiry") in response.out
    assert Re("^fakenode.secure.example: expiry") in response.out

    # secure.example and badds.example used the default nta-duration
    # (configured as 12s in ns4/named1.conf), but the nta recheck interval
    # is configured to 9s, so at t=10 the NTAs for secure.example and
    # fakenode.secure.example should both be lifted, while badds.example
    # should still be going.
    delay = START + 10 - time.time()
    if delay > 0:
        time.sleep(delay)

    m = isctest.query.create("b.secure.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)

    m = isctest.query.create("b.fakenode.secure.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.nxdomain(res)
    isctest.check.adflag(res)

    m = isctest.query.create("badds.example.", "SOA")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    # bogus.example was set to expire in 20s, so at t=13
    # it should still be NTA'd, but badds.example used the default
    # lifetime of 12s, so it should revert to SERVFAIL now.
    delay = START + 13 - time.time()
    if delay > 0:
        time.sleep(delay)

    response = ns4.rndc("nta -d")
    assert active(response.out) <= 2

    response = ns4.rndc("secroots -")
    assert Re("bogus.example: expiry") in response.out
    assert Re("badds.example: expiry") not in response.out

    m = isctest.query.create("b.bogus.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)

    m = isctest.query.create("a.badds.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.servfail(res)
    isctest.check.noadflag(res)

    m = isctest.query.create("c.secure.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)

    # at t=21, all the NTAs should have expired.
    delay = START + 21 - time.time()
    if delay > 0:
        time.sleep(delay)

    response = ns4.rndc("nta -d")
    assert active(response.out) == 0

    m = isctest.query.create("d.secure.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)

    m = isctest.query.create("c.bogus.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.servfail(res)
    isctest.check.noadflag(res)


def test_nta_removals(servers):
    ns4 = servers["ns4"]
    ns4.rndc("nta badds.example")

    response = ns4.rndc("nta -d")
    assert Re("^badds.example/_default: expiry") in response.out

    m = isctest.query.create("a.badds.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    response = ns4.rndc("nta -remove badds.example")
    assert "Negative trust anchor removed: badds.example" in response.out

    response = ns4.rndc("nta -d")
    assert Re("^badds.example/_default: expiry") not in response.out

    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.servfail(res)
    isctest.check.noadflag(res)

    # remove non-existent NTA three times
    ns4.rndc("nta -r foo")
    ns4.rndc("nta -remove foo")
    response = ns4.rndc("nta -r foo")
    assert "not found" in response.out


def test_nta_restarts(servers):
    global START
    assert START, "test_nta_restarts must be run as part of the full NTA test"

    # test NTA persistence across restarts
    ns4 = servers["ns4"]
    response = ns4.rndc("nta -d")
    assert active(response.out) == 0

    START = time.time()
    ns4.rndc("nta -f -l 30s bogus.example")
    ns4.rndc("nta -f -l 10s badds.example")
    response = ns4.rndc("nta -d")
    assert active(response.out) == 2

    # stop the server
    ns4.stop()

    # wait 14s before restarting. badds.example's NTA (lifetime=10s) should
    # have expired, and bogus.example should still be running.
    delay = START + 14 - time.time()
    if delay > 0:
        time.sleep(delay)
    ns4.start(["--noclean", "--restart", "--port", os.environ["PORT"]])

    response = ns4.rndc("nta -d")
    assert active(response.out) == 1
    assert Re("^bogus.example/_default: expiry") in response.out

    m = isctest.query.create("a.badds.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.servfail(res)

    m = isctest.query.create("a.bogus.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    ns4.rndc("nta -r bogus.example")


def test_nta_regular(servers):
    global START
    assert START, "test_nta_regular must be run as part of the full NTA test"

    # check "regular" attribute in NTA file
    ns4 = servers["ns4"]

    response = ns4.rndc("nta -d")
    assert active(response.out) == 0

    # secure.example validates with AD=1
    m = isctest.query.create("a.secure.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)

    # stop the server, update _default.nta, restart
    ns4.stop()
    now = time.localtime()
    future = str(now.tm_year + 20) + "0101010000"
    with open("ns4/_default.nta", "w", encoding="utf-8") as f:
        f.write(f"secure.example. regular {future}")

    ns4.start(["--noclean", "--restart", "--port", os.environ["PORT"]])

    # NTA active; secure.example. should now return an AD=0 answer.
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    # nta-recheck is configured as 9s, so at t=12 the NTA for
    # secure.example. should be lifted as it is not a "forced" NTA.
    START = time.mktime(now)
    delay = START + 12 - time.time()
    if delay > 0:
        time.sleep(delay)

    response = ns4.rndc("nta -d")
    assert active(response.out) == 0

    # NTA lifted; secure.example. flush the cache to trigger a new query,
    # and it should now return an AD=1 answer.
    ns4.rndc("flushtree secure.example")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)


def test_nta_forced(servers):
    global START
    assert START, "test_nta_regular must be run as part of the full NTA test"

    # check "forced" attribute in NTA file
    ns4 = servers["ns4"]

    # just to be certain, clean up any existing NTA first
    ns4.rndc("nta -r secure.example")

    response = ns4.rndc("nta -d")
    assert active(response.out) == 0

    # secure.example validates with AD=1
    m = isctest.query.create("a.secure.example", "A")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.adflag(res)

    # stop the server, update _default.nta, restart
    ns4.stop()
    now = time.localtime()
    future = str(now.tm_year + 20) + "0101010000"
    with open("ns4/_default.nta", "w", encoding="utf-8") as f:
        f.write(f"secure.example. forced {future}")

    ns4.start(["--noclean", "--restart", "--port", os.environ["PORT"]])

    # NTA active; secure.example. should now return an AD=0 answer
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)

    # nta-recheck is configured as 9s. at t=12 the NTA for
    # secure.example. should NOT be lifted as it is "forced".
    START = time.mktime(now)
    delay = START + 12 - time.time()
    if delay > 0:
        time.sleep(delay)

    # NTA lifted; secure.example. should still return an AD=0 answer
    ns4.rndc("flushtree secure.example")
    res = isctest.query.tcp(m, "10.53.0.4")
    isctest.check.noerror(res)
    isctest.check.noadflag(res)


def test_nta_clamping(servers):
    ns4 = servers["ns4"]

    # clean up any existing NTA
    ns4.rndc("nta -r secure.example")

    # stop the server, update _default.nta, restart
    ns4.stop()
    now = time.localtime()
    future = str(now.tm_year + 20) + "0101010000"
    with open("ns4/_default.nta", "w", encoding="utf-8") as f:
        f.write(f"secure.example. forced {future}")

    ns4.start(["--noclean", "--restart", "--port", os.environ["PORT"]])

    # check that NTA lifetime read from file is clamped to 1 week.
    response = ns4.rndc("nta -d")
    assert active(response.out) == 1

    nta = next((s for s in response.out.splitlines() if " expiry" in s), None)
    assert nta is not None

    nta = nta.split(" ")
    expiry = f"{nta[2]} {nta[3]}"
    then = time.mktime(time.strptime(expiry, "%d-%b-%Y %H:%M:%S.000"))
    nextweek = time.mktime(now) + (86400 * 7)

    # normally there's no more than a few seconds difference between the
    # clamped expiration date and the calculated date for next week,
    # but add a 3600 second fudge factor to allow for daylight savings
    # changes.
    assert abs(nextweek - then < 3610)

    # remove the NTA
    ns4.rndc("nta -r secure.example")


def test_nta_forward(servers):
    ns9 = servers["ns9"]

    m = isctest.query.create("badds.example", "SOA")
    res = isctest.query.tcp(m, "10.53.0.9")
    isctest.check.servfail(res)
    isctest.check.empty_answer(res)
    isctest.check.noadflag(res)

    # add NTA and expect resolution to succeed
    ns9.rndc("nta badds.example")
    res = isctest.query.tcp(m, "10.53.0.9")
    isctest.check.noerror(res)
    isctest.check.rr_count_eq(res.answer, 2)
    isctest.check.noadflag(res)

    # remove NTA and expect resolution to fail again
    ns9.rndc("nta -remove badds.example")
    res = isctest.query.tcp(m, "10.53.0.9")
    isctest.check.servfail(res)
    isctest.check.empty_answer(res)
    isctest.check.noadflag(res)
