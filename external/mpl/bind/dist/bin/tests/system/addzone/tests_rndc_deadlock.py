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

import concurrent.futures
import os
import subprocess
import time


def run_rndc(server, rndc_command):
    """
    Send the specified 'rndc_command' to 'server' with a timeout of 10 seconds
    """
    rndc = os.getenv("RNDC")
    port = os.getenv("CONTROLPORT")

    cmdline = [rndc, "-c", "../_common/rndc.conf", "-p", port, "-s", server]
    cmdline.extend(rndc_command)

    subprocess.check_output(cmdline, stderr=subprocess.STDOUT, timeout=10)


def rndc_loop(test_state, domain):
    """
    Run "rndc addzone", "rndc modzone", and "rndc delzone" in a tight loop
    until the test is considered finished, ignoring errors
    """
    rndc_commands = [
        ["addzone", domain, '{ type primary; file "example.db"; };'],
        [
            "modzone",
            domain,
            '{ type primary; file "example.db"; allow-transfer { any; }; };',
        ],
        ["delzone", domain],
    ]

    while not test_state["finished"]:
        for command in rndc_commands:
            try:
                run_rndc("10.53.0.3", command)
            except subprocess.SubprocessError:
                pass


def check_if_server_is_responsive():
    """
    Check if server status can be successfully retrieved using "rndc status"
    """
    try:
        run_rndc("10.53.0.3", ["status"])
        return True
    except subprocess.SubprocessError:
        return False


def test_rndc_deadlock():
    """
    Test whether running "rndc addzone", "rndc modzone", and "rndc delzone"
    commands concurrently does not trigger a deadlock
    """
    test_state = {"finished": False}

    # Create 4 worker threads running "rndc" commands in a loop.
    with concurrent.futures.ThreadPoolExecutor() as executor:
        for i in range(1, 5):
            domain = "example%d" % i
            executor.submit(rndc_loop, test_state, domain)

        # Run "rndc status" 10 times, with 1-second pauses between attempts.
        # Each "rndc status" invocation has a timeout of 10 seconds.  If any of
        # them fails, the loop will be interrupted.
        server_is_responsive = True
        attempts = 10
        while server_is_responsive and attempts > 0:
            server_is_responsive = check_if_server_is_responsive()
            attempts -= 1
            time.sleep(1)

        # Signal worker threads that the test is finished.
        test_state["finished"] = True

    # Check whether all "rndc status" commands succeeded.
    assert server_is_responsive
