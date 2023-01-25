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
import os
import random
import signal
import subprocess
from string import ascii_lowercase as letters
import time

import pytest

pytest.importorskip("dns")
import dns.exception
import dns.resolver


def do_work(named_proc, resolver, rndc_cmd, kill_method, n_workers, n_queries):
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
    POSIX systems or by TerminateProcess() on Windows systems.

    :param named_proc: named process instance
    :type named_proc: subprocess.Popen

    :param resolver: target resolver
    :type resolver: dns.resolver.Resolver

    :param rndc_cmd: rndc command with default arguments
    :type rndc_cmd: list of strings, e.g. ["rndc", "-p", "23750"]

    :kill_method: "rndc" or "sigterm"
    :type kill_method: str

    :param n_workers: Number of worker threads to create
    :type n_workers: int

    :param n_queries: Total number of queries to send
    :type n_queries: int
    """
    # pylint: disable-msg=too-many-arguments
    # pylint: disable-msg=too-many-locals

    # helper function, args must be a list or tuple with arguments to rndc.
    def launch_rndc(args):
        return subprocess.call(rndc_cmd + args, timeout=10)

    # We're going to execute queries in parallel by means of a thread pool.
    # dnspython functions block, so we need to circunvent that.
    with ThreadPoolExecutor(n_workers + 1) as executor:

        # Helper dict, where keys=Future objects and values are tags used
        # to process results later.
        futures = {}

        # 50% of work will be A queries.
        # 1 work will be rndc stop.
        # Remaining work will be rndc status (so we test parallel control
        #  connections that were crashing named).
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
                futures[executor.submit(resolver.query, qname, "A")] = tag
            elif shutdown:  # We attempt to stop named in the middle
                shutdown = False
                if kill_method == "rndc":
                    futures[executor.submit(launch_rndc, ["stop"])] = "stop"
                else:
                    futures[executor.submit(named_proc.terminate)] = "kill"
            else:
                # We attempt to send couple rndc commands while named is
                # being shutdown
                futures[executor.submit(launch_rndc, ["status"])] = "status"

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

            except (
                dns.resolver.NXDOMAIN,
                dns.resolver.NoNameservers,
                dns.exception.Timeout,
            ):
                pass

        if kill_method == "rndc":
            assert ret_code == 0


def test_named_shutdown(named_port, control_port):
    # pylint: disable-msg=too-many-locals
    cfg_dir = os.path.join(os.getcwd(), "resolver")
    assert os.path.isdir(cfg_dir)

    cfg_file = os.path.join(cfg_dir, "named.conf")
    assert os.path.isfile(cfg_file)

    named = os.getenv("NAMED")
    assert named is not None

    rndc = os.getenv("RNDC")
    assert rndc is not None

    systest_dir = os.getenv("SYSTEMTESTTOP")
    assert systest_dir is not None

    # rndc configuration resides in $SYSTEMTESTTOP/common/rndc.conf
    rndc_cfg = os.path.join(systest_dir, "common", "rndc.conf")
    assert os.path.isfile(rndc_cfg)

    # rndc command with default arguments.
    rndc_cmd = [rndc, "-c", rndc_cfg, "-p", str(control_port), "-s", "10.53.0.3"]

    # We create a resolver instance that will be used to send queries.
    resolver = dns.resolver.Resolver()
    resolver.nameservers = ["10.53.0.3"]
    resolver.port = named_port

    # We test named shutting down using two methods:
    # Method 1: using rndc ctop
    # Method 2: killing with SIGTERM
    # In both methods named should exit gracefully.
    for kill_method in ("rndc", "sigterm"):
        named_cmdline = [named, "-c", cfg_file, "-f"]
        with subprocess.Popen(named_cmdline, cwd=cfg_dir) as named_proc:
            # Ensure named is running
            assert named_proc.poll() is None
            # wait for named to finish loading
            for _ in range(10):
                try:
                    resolver.query("version.bind", "TXT", "CH")
                    break
                except (dns.resolver.NoNameservers, dns.exception.Timeout):
                    time.sleep(1)

            do_work(
                named_proc, resolver, rndc_cmd, kill_method, n_workers=12, n_queries=16
            )

            # Wait named to exit for a maximum of MAX_TIMEOUT seconds.
            MAX_TIMEOUT = 10
            is_dead = False
            for _ in range(MAX_TIMEOUT):
                if named_proc.poll() is not None:
                    is_dead = True
                    break
                time.sleep(1)

            if not is_dead:
                named_proc.send_signal(signal.SIGABRT)
                for _ in range(MAX_TIMEOUT):
                    if named_proc.poll() is not None:
                        is_dead = True
                        break
                    time.sleep(1)
                if not is_dead:
                    named_proc.kill()

            assert is_dead
            # Ensures that named exited gracefully.
            # If it crashed (abort()) exitcode will be non zero.
            assert named_proc.returncode == 0
