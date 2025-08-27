# Copyright 2022-2024 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import re

# These are deprecated in 3.9, but required in older versions.
from typing import Mapping, Optional, Sequence

import gdb

from .events import exec_and_expect_stop, expect_process, expect_stop
from .server import (
    DeferredRequest,
    call_function_later,
    capability,
    request,
    send_gdb,
    send_gdb_with_response,
)
from .startup import DAPException, exec_and_log, in_dap_thread, in_gdb_thread

# A launch or attach promise that that will be fulfilled after a
# configurationDone request has been processed.
_launch_or_attach_promise = None


# A DeferredRequest that handles either a "launch" or "attach"
# request.
class _LaunchOrAttachDeferredRequest(DeferredRequest):
    def __init__(self, callback):
        self._callback = callback
        global _launch_or_attach_promise
        if _launch_or_attach_promise is not None:
            raise DAPException("launch or attach already specified")
        _launch_or_attach_promise = self

    # Invoke the callback and return the result.
    @in_dap_thread
    def invoke(self):
        return self._callback()

    # Override this so we can clear the global when rescheduling.
    @in_dap_thread
    def reschedule(self):
        global _launch_or_attach_promise
        _launch_or_attach_promise = None
        super().reschedule()


# A wrapper for the 'file' command that correctly quotes its argument.
@in_gdb_thread
def file_command(program):
    # Handle whitespace, quotes, and backslashes here.  Exactly what
    # to quote depends on libiberty's buildargv and safe-ctype.
    program = re.sub("[ \t\n\r\f\v\\\\'\"]", "\\\\\\g<0>", program)
    exec_and_log("file " + program)


# Any parameters here are necessarily extensions -- DAP requires this
# from implementations.  Any additions or changes here should be
# documented in the gdb manual.
@request("launch", on_dap_thread=True)
def launch(
    *,
    program: Optional[str] = None,
    cwd: Optional[str] = None,
    args: Sequence[str] = (),
    env: Optional[Mapping[str, str]] = None,
    stopAtBeginningOfMainSubprogram: bool = False,
    stopOnEntry: bool = False,
    **extra,
):
    # Launch setup is handled here.  This is done synchronously so
    # that errors can be reported in a natural way.
    @in_gdb_thread
    def _setup_launch():
        if cwd is not None:
            exec_and_log("cd " + cwd)
        if program is not None:
            file_command(program)
        inf = gdb.selected_inferior()
        inf.arguments = args
        if env is not None:
            inf.clear_env()
            for name, value in env.items():
                inf.set_env(name, value)

    # Actual launching done here.  See below for more info.
    @in_gdb_thread
    def _do_launch():
        expect_process("process")
        if stopAtBeginningOfMainSubprogram:
            cmd = "start"
        elif stopOnEntry:
            cmd = "starti"
        else:
            cmd = "run"
        exec_and_expect_stop(cmd)

    @in_dap_thread
    def _launch_impl():
        send_gdb_with_response(_setup_launch)
        # We do not wait for the result here.  It might be a little
        # nicer if we did -- perhaps the various thread events would
        # occur in a more logical sequence -- but if the inferior does
        # not stop, then the launch response will not be seen either,
        # which seems worse.
        send_gdb(_do_launch)
        # Launch response does not have a body.
        return None

    # The launch itself is deferred until the configurationDone
    # request.
    return _LaunchOrAttachDeferredRequest(_launch_impl)


@request("attach", on_dap_thread=True)
def attach(
    *,
    program: Optional[str] = None,
    pid: Optional[int] = None,
    target: Optional[str] = None,
    **args,
):
    # The actual attach is handled by this function.
    @in_gdb_thread
    def _do_attach():
        if program is not None:
            file_command(program)
        if pid is not None:
            cmd = "attach " + str(pid)
        elif target is not None:
            cmd = "target remote " + target
        else:
            raise DAPException("attach requires either 'pid' or 'target'")
        expect_process("attach")
        expect_stop("attach")
        exec_and_log(cmd)
        # Attach response does not have a body.
        return None

    @in_dap_thread
    def _attach_impl():
        return send_gdb_with_response(_do_attach)

    # The attach itself is deferred until the configurationDone
    # request.
    return _LaunchOrAttachDeferredRequest(_attach_impl)


@capability("supportsConfigurationDoneRequest")
@request("configurationDone", on_dap_thread=True)
def config_done(**args):
    # Handle the launch or attach.
    global _launch_or_attach_promise
    if _launch_or_attach_promise is None:
        raise DAPException("launch or attach not specified")
    # Resolve the launch or attach, but only after the
    # configurationDone response has been sent.
    call_function_later(_launch_or_attach_promise.reschedule)
