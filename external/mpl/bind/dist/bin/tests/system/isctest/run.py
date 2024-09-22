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

import subprocess
import time
from typing import Optional

import isctest.log


def cmd(  # pylint: disable=too-many-arguments
    args,
    cwd=None,
    timeout=60,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    log_stdout=False,
    log_stderr=True,
    input_text: Optional[bytes] = None,
    raise_on_exception=True,
):
    """Execute a command with given args as subprocess."""
    isctest.log.debug(f"command: {' '.join(args)}")

    def print_debug_logs(procdata):
        if procdata:
            if log_stdout and procdata.stdout:
                isctest.log.debug(
                    f"~~~ cmd stdout ~~~\n{procdata.stdout.decode('utf-8')}\n~~~~~~~~~~~~~~~~~~"
                )
            if log_stderr and procdata.stderr:
                isctest.log.debug(
                    f"~~~ cmd stderr ~~~\n{procdata.stderr.decode('utf-8')}\n~~~~~~~~~~~~~~~~~~"
                )

    try:
        proc = subprocess.run(
            args,
            stdout=stdout,
            stderr=stderr,
            input=input_text,
            check=True,
            cwd=cwd,
            timeout=timeout,
        )
        print_debug_logs(proc)
        return proc
    except subprocess.CalledProcessError as exc:
        print_debug_logs(exc)
        isctest.log.debug(f"  return code: {exc.returncode}")
        if raise_on_exception:
            raise exc
        return exc


def retry_with_timeout(func, timeout, delay=1, msg=None):
    start_time = time.time()
    while time.time() < start_time + timeout:
        if func():
            return
        time.sleep(delay)
    if msg is None:
        msg = f"{func.__module__}.{func.__qualname__} timed out after {timeout} s"
    assert False, msg
