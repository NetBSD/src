#!/usr/bin/python3

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

from concurrent.futures import ThreadPoolExecutor, as_completed
from string import ascii_lowercase as letters

import os
import random
import signal
import subprocess
import time

import dns.exception
import pytest

import isctest

pytestmark = pytest.mark.extra_artifacts(
    [
        "resolver/named.conf",
        "resolver/named.run",
    ]
)


def do_work(named_proc, resolver_ip, instance, kill_method, n_workers, n_queries):
    """Creates a number of A queries to run in parallel
    in order simulate a slightly more realistic test scenario.

    The main idea of this function is to create and send a bunch
    of A queries to a target named instance and during this process
    a request for shutting down named will be issued.

    In the process of shutting down named, a couple control connections
    are created (by launching rndc) to ensure that the crash was fixed.

    if kill_method=="rndc" named will be asked to shutdown by
    means of rndc stop.
    if kill_method=="sigterm" named will be killed by SIGTERM on
    POSIX systems.

    :param named_proc: named process instance
    :type named_proc: subprocess.Popen

    :param resolver_ip: target resolver's IP address
    :type resolver_ip: str

    :param instance: the named instance to send RNDC commands to
    :type instance: isctest.instance.NamedInstance

    :kill_method: "rndc" or "sigterm"
    :type kill_method: str

    :param n_workers: Number of worker threads to create
    :type n_workers: int

    :param n_queries: Total number of queries to send
    :type n_queries: int
    """

    # helper function, 'command' is the rndc command to run
    def launch_rndc(command):
        ret = instance.rndc(command, raise_on_exception=False)
        return 0 if ret.rc == 0 else -1

    # We're going to execute queries in parallel by means of a thread pool.
    # dnspython functions block, so we need to circumvent that.
    with ThreadPoolExecutor(n_workers + 1) as executor:
        # Helper dict, where keys=Future objects and values are tags used
        # to process results later.
        futures = {}

        # 50% of work will be A queries.
        # 1 work will be rndc stop.
        # Remaining work will be rndc status (so we test parallel control
        # connections that were crashing named).
        shutdown = True
        for i in range(n_queries):
            if i < (n_queries // 2):
                # Half work will be standard A queries.
                # Among those we split 50% queries relname='www',
                # 50% queries relname=random characters
                if random.randrange(2) == 1:
                    tag = "good"
                    relname = "www"
                else:
                    tag = "bad"
                    length = random.randint(4, 10)
                    relname = "".join(
                        letters[random.randrange(len(letters))] for i in range(length)
                    )

                qname = relname + ".test"
                msg = isctest.query.create(qname, "A")
                futures[
                    executor.submit(
                        isctest.query.udp, msg, resolver_ip, timeout=1, attempts=1
                    )
                ] = tag
            elif shutdown:  # We attempt to stop named in the middle
                shutdown = False
                if kill_method == "rndc":
                    futures[executor.submit(launch_rndc, "stop")] = "stop"
                else:
                    futures[executor.submit(named_proc.terminate)] = "kill"
            else:
                # We attempt to send couple rndc commands while named is
                # being shutdown
                futures[executor.submit(launch_rndc, "-t 5 status")] = "status"

        ret_code = -1
        for future in as_completed(futures):
            try:
                result = future.result()
                # If tag is "stop", result is an instance of
                # subprocess.CompletedProcess, then we check returncode
                # attribute to know if rncd stop command finished successfully.
                #
                # if tag is "kill" then the main function will check if
                # named process exited gracefully after SIGTERM signal.
                if futures[future] == "stop":
                    ret_code = result
            except dns.exception.Timeout:
                pass

        if kill_method == "rndc":
            assert ret_code == 0


def wait_for_proc_termination(proc, max_timeout=10):
    for _ in range(max_timeout):
        if proc.poll() is not None:
            return True
        time.sleep(1)

    proc.send_signal(signal.SIGABRT)
    for _ in range(max_timeout):
        if proc.poll() is not None:
            return True
        time.sleep(1)

    return False


# We test named shutting down using two methods:
# Method 1: using rndc ctop
# Method 2: killing with SIGTERM
# In both methods named should exit gracefully.
@pytest.mark.parametrize(
    "kill_method",
    ["rndc", "sigterm"],
)
def test_named_shutdown(kill_method):
    resolver_ip = "10.53.0.3"

    cfg_dir = "resolver"

    named_cmdline = isctest.run.get_named_cmdline(cfg_dir)
    instance = isctest.instance.NamedInstance("resolver", num=3)

    with open(os.path.join(cfg_dir, "named.run"), "ab") as named_log:
        with subprocess.Popen(
            named_cmdline, cwd=cfg_dir, stderr=named_log
        ) as named_proc:
            try:
                isctest.check.named_alive(named_proc, resolver_ip)
                do_work(
                    named_proc,
                    resolver_ip,
                    instance,
                    kill_method,
                    n_workers=12,
                    n_queries=16,
                )
                assert wait_for_proc_termination(named_proc)
                assert named_proc.returncode == 0, "named crashed"
            finally:  # Ensure named is terminated in case of an exception
                named_proc.kill()
