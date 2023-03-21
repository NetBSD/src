# Copyright (C) 2020 Free Software Foundation, Inc.
#
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

# A TUI window implemented in Python.

import gdb

the_window = None

class TestWindow:
    def __init__(self, win):
        global the_window
        the_window = win
        self.count = 0
        self.win = win
        win.title = "This Is The Title"

    def render(self):
        self.win.erase()
        w = self.win.width
        h = self.win.height
        self.win.write("Test: " + str(self.count) + " " + str(w) + "x" + str(h))
        self.count = self.count + 1

gdb.register_window_type("test", TestWindow)

# A TUI window "constructor" that always fails.
def failwin(win):
    raise RuntimeError("Whoops")

gdb.register_window_type("fail", failwin)
