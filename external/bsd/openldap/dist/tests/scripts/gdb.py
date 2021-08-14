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
This GDB script sets up the debugger to run the program and see if it finishes
of its own accord or is terminated by a signal (like SIGABRT/SIGSEGV). In the
latter case, it saves a full backtrace and core file.

These signals are considered part of normal operation and will not trigger the
above handling:
- SIGPIPE: normal in a networked environment
- SIGHUP: normally used to tell a process to shut down
"""

import os
import os.path

import gdb


def format_program(inferior=None, thread=None):
    "Format program name and p(t)id"

    if thread:
        inferior = thread.inferior
    elif inferior is None:
        inferior = gdb.selected_inferior()

    try:
        name = os.path.basename(inferior.progspace.filename)
    except AttributeError:  # inferior has died already
        name = "unknown"

    if thread:
        pid = ".".join(tid for tid in thread.ptid if tid)
    else:
        pid = inferior.pid

    return "{}.{}".format(name, pid)


def stop_handler(event):
    "Inferior stopped on a signal, record core, backtrace and exit"

    if not isinstance(event, gdb.SignalEvent):
        # Ignore breakpoints
        return

    thread = event.inferior_thread

    identifier = format_program(thread=thread)
    prefix = os.path.expandvars("${TESTDIR}/") + identifier

    if event.stop_signal == "SIGHUP":
        # TODO: start a timer to catch shutdown issues/deadlocks
        gdb.execute("continue")
        return

    gdb.execute('generate-core-file {}.core'.format(prefix))

    with open(prefix + ".backtrace", "w") as bt_file:
        backtrace = gdb.execute("thread apply all backtrace full",
                                to_string=True)
        bt_file.write(backtrace)

    gdb.execute("continue")


# We or we could allow the runner to disable randomisation
gdb.execute("set disable-randomization off")

gdb.execute("handle SIGPIPE noprint")
gdb.execute("handle SIGINT pass")
gdb.events.stop.connect(stop_handler)
gdb.execute("run")
