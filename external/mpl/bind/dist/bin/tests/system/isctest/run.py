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

from pathlib import Path

import os
import subprocess
import time

import isctest.log
import isctest.text


class CmdResult:
    def __init__(self, proc=None):
        self.proc = proc
        self.rc = self.proc.returncode
        self.out = isctest.text.Text("")
        self.err = isctest.text.Text("")
        if self.proc.stdout:
            self.out = isctest.text.Text(self.proc.stdout.decode("utf-8"))
        if self.proc.stderr:
            self.err = isctest.text.Text(self.proc.stderr.decode("utf-8"))


def cmd(
    args,
    cwd=None,
    timeout=60,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    log_stdout=True,
    log_stderr=True,
    input_text: bytes | None = None,
    raise_on_exception=True,
    env: dict | None = None,
) -> CmdResult:
    """Execute a command with given args as subprocess."""
    isctest.log.debug(f"isctest.run.cmd(): {' '.join(args)}")

    def print_debug_logs(procdata):
        if procdata:
            if log_stdout and procdata.stdout:
                isctest.log.debug(
                    f"isctest.run.cmd(): (stdout)\n{procdata.stdout.decode('utf-8')}"
                )
            if log_stderr and procdata.stderr:
                isctest.log.debug(
                    f"isctest.run.cmd(): (stderr)\n{procdata.stderr.decode('utf-8')}"
                )

    if env is None:
        env = dict(os.environ)

    try:
        proc = subprocess.run(
            args,
            stdout=stdout,
            stderr=stderr,
            input=input_text,
            check=True,
            cwd=cwd,
            timeout=timeout,
            env=env,
        )
        print_debug_logs(proc)
        return CmdResult(proc)
    except subprocess.CalledProcessError as exc:
        print_debug_logs(exc)
        isctest.log.debug(f"isctest.run.cmd(): (return code) {exc.returncode}")
        if raise_on_exception:
            raise exc
        return CmdResult(exc)


class EnvCmd:
    """Helper for executing binaries from env with optional base parameters."""

    def __init__(self, name: str, base_params: str = ""):
        self.bin_path = os.environ[name]
        self.base_params = base_params.split()

    def __call__(self, params: str, **kwargs) -> CmdResult:
        """Call the command. Keyword arguments from isctest.run.cmd() are supported."""
        args = self.base_params + params.split()
        return cmd([self.bin_path] + args, **kwargs)


def _run_script(
    interpreter: str,
    script: str,
    args: list[str] | None = None,
):
    if args is None:
        args = []
    path = Path(script)
    script = str(path)
    cwd = os.getcwd()
    if not path.exists():
        raise FileNotFoundError(f"script {script} not found in {cwd}")
    isctest.log.debug("running script: %s %s %s", interpreter, script, " ".join(args))
    isctest.log.debug("  workdir: %s", cwd)
    returncode = 1

    command = [interpreter, script] + args
    with subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        bufsize=1,
        universal_newlines=True,
        errors="backslashreplace",
    ) as proc:
        if proc.stdout:
            for line in proc.stdout:
                isctest.log.info("    %s", line.rstrip("\n"))
        proc.communicate()
        returncode = proc.returncode
        if returncode:
            raise subprocess.CalledProcessError(returncode, command)
        isctest.log.debug("  exited with %d", returncode)


def shell(script: str, args: list[str] | None = None) -> None:
    """Run a given script with system's shell interpreter."""
    _run_script(os.environ["SHELL"], script, args)


def perl(script: str, args: list[str] | None = None) -> None:
    """Run a given script with system's perl interpreter."""
    _run_script(os.environ["PERL"], script, args)


def retry_with_timeout(func, timeout, delay=1, msg=None):
    start_time = time.monotonic()
    exc_msg = None
    fname = f"{func.__module__}.{func.__qualname__}()"
    while time.monotonic() < start_time + timeout:
        exc_msg = None
        isctest.log.debug(f"retry_with_timeout: {fname} called")
        try:
            if func():
                isctest.log.debug(f"retry_with_timeout: {fname} succeeded")
                return
        except AssertionError as exc:
            exc_msg = str(exc)
        isctest.log.debug(f"retry_with_timeout: {fname} failed, sleep {delay}s")
        time.sleep(delay)
    if exc_msg is not None:
        isctest.log.error(exc_msg)
    if msg is None:
        if exc_msg is not None:
            msg = exc_msg
        else:
            msg = f"{fname} timed out after {timeout} s"
    assert False, msg


def get_named_cmdline(cfg_dir, cfg_file="named.conf"):
    cfg_dir = os.path.join(os.getcwd(), cfg_dir)
    assert os.path.isdir(cfg_dir)

    cfg_file = os.path.join(cfg_dir, cfg_file)
    assert os.path.isfile(cfg_file)

    named = os.getenv("NAMED")
    assert named is not None

    named_cmdline = [named, "-c", cfg_file, "-d", "99", "-g"]

    return named_cmdline
