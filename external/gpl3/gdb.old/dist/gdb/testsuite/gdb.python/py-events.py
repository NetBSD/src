# Copyright (C) 2010-2014 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests python pretty
# printers.
import gdb

def signal_stop_handler (event):
    if (isinstance (event, gdb.StopEvent)):
        print ("event type: stop")
    if (isinstance (event, gdb.SignalEvent)):
        print ("stop reason: signal")
        print ("stop signal: %s" % (event.stop_signal))
        if ( event.inferior_thread is not None) :
            print ("thread num: %s" % (event.inferior_thread.num))

def breakpoint_stop_handler (event):
    if (isinstance (event, gdb.StopEvent)):
        print ("event type: stop")
    if (isinstance (event, gdb.BreakpointEvent)):
        print ("stop reason: breakpoint")
        print ("first breakpoint number: %s" % (event.breakpoint.number))
        for bp in event.breakpoints:
        	print ("breakpoint number: %s" % (bp.number))
        if ( event.inferior_thread is not None) :
            print ("thread num: %s" % (event.inferior_thread.num))
        else:
            print ("all threads stopped")

def exit_handler (event):
    assert (isinstance (event, gdb.ExitedEvent))
    print ("event type: exit")
    print ("exit code: %d" % (event.exit_code))
    print ("exit inf: %d" % (event.inferior.num))
    print ("dir ok: %s" % str('exit_code' in dir(event)))

def continue_handler (event):
    assert (isinstance (event, gdb.ContinueEvent))
    print ("event type: continue")
    if ( event.inferior_thread is not None) :
        print ("thread num: %s" % (event.inferior_thread.num))

def new_objfile_handler (event):
    assert (isinstance (event, gdb.NewObjFileEvent))
    print ("event type: new_objfile")
    print ("new objfile name: %s" % (event.new_objfile.filename))

class test_events (gdb.Command):
    """Test events."""

    def __init__ (self):
        gdb.Command.__init__ (self, "test-events", gdb.COMMAND_STACK)

    def invoke (self, arg, from_tty):
        gdb.events.stop.connect (signal_stop_handler)
        gdb.events.stop.connect (breakpoint_stop_handler)
        gdb.events.exited.connect (exit_handler)
        gdb.events.cont.connect (continue_handler)
        print ("Event testers registered.")

test_events ()

class test_newobj_events (gdb.Command):
    """NewObj events."""

    def __init__ (self):
        gdb.Command.__init__ (self, "test-objfile-events", gdb.COMMAND_STACK)

    def invoke (self, arg, from_tty):
        gdb.events.new_objfile.connect (new_objfile_handler)
        print ("Object file events registered.")

test_newobj_events ()
