# Copyright (C) 2025 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.

import gdb


class PyCommandsBreakpoint(gdb.Breakpoint):
    bp_dict = dict()

    def __init__(self, *args, **kwargs):
        gdb.Breakpoint.__init__(self, *args, **kwargs)
        PyCommandsBreakpoint.bp_dict[self.number] = self
        self.commands = ""

    @staticmethod
    def run_py_commands(num):
        bp = PyCommandsBreakpoint.bp_dict[num]
        if hasattr(bp, "py_commands"):
            bp.py_commands()

    def __setattr__(self, name, value):
        if name == "commands":
            l = ["python PyCommandsBreakpoint.run_py_commands(%d)" % self.number]
            if value != "":
                l.append(value)
            value = "\n".join(l)

        super().__setattr__(name, value)


class TestBreakpoint(PyCommandsBreakpoint):
    def __init__(self):
        PyCommandsBreakpoint.__init__(self, spec="test")
        self.var = 1
        self.commands = "continue"
        self.silent = True

    def stop(self):
        if self.var == 3:
            self.commands = ""
        return True

    def py_commands(self):
        print("VAR: %d" % self.var)
        self.var += 1
