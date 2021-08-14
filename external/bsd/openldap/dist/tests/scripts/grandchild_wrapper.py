#!/usr/bin/env python3
# $OpenLDAP$
## This work is part of OpenLDAP Software <http://www.openldap.org/>.
##
## Copyright 2020-2021 The OpenLDAP Foundation.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted only as authorized by the OpenLDAP
## Public License.
##
## A copy of this license is available in the file LICENSE in the
## top-level directory of the distribution or, alternatively, at
## <http://www.OpenLDAP.org/license.html>.
"""
Running slapd under GDB in our testsuite, KILLPIDS would record gdb's PID
rather than slapd's. When we want the server to shut down, SIGHUP is sent to
KILLPIDS but GDB cannot handle being signalled directly and the entire thing is
terminated immediately. There might be tests that rely on slapd being given the
chance to shut down gracefully, to do this, we need to make sure the signal is
actually sent to slapd.

This script attempts to address this shortcoming in our test suite, serving as
the front for gdb/other wrappers, catching SIGHUPs and redirecting them to the
oldest living grandchild. The way we start up gdb, that process should be
slapd, our intended target.

This requires the pgrep utility provided by the procps package on Debian
systems.
"""

import asyncio
import os
import signal
import sys


async def signal_to_grandchild(child):
    # Get the first child, that should be the one we're after
    pgrep = await asyncio.create_subprocess_exec(
            "pgrep", "-o", "--parent", str(child.pid),
            stdout=asyncio.subprocess.PIPE)

    stdout, _ = await pgrep.communicate()
    if not stdout:
        return

    grandchild = [int(pid) for pid in stdout.split()][0]

    os.kill(grandchild, signal.SIGHUP)


def sighup_handler(child):
    asyncio.create_task(signal_to_grandchild(child))


async def main(args=None):
    if args is None:
        args = sys.argv[1:]

    child = await asyncio.create_subprocess_exec(*args)

    # If we got a SIGHUP before we got the child fully started, there's no
    # point signalling anyway
    loop = asyncio.get_running_loop()
    loop.add_signal_handler(signal.SIGHUP, sighup_handler, child)

    raise SystemExit(await child.wait())


if __name__ == '__main__':
    asyncio.run(main())
