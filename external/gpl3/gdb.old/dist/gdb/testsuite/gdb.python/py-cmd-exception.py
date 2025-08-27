# Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

import gdb


class MyListener:
    def __init__(self):
        gdb.events.new_objfile.connect(self.handle_new_objfile_event)
        self.processed_objfile = False

    def handle_new_objfile_event(self, event):
        if self.processed_objfile:
            return

        print("loading " + event.new_objfile.filename)
        self.processed_objfile = True

        # There is no variable 'a'.  The command raises an exception.
        gdb.execute("print a")


the_listener = MyListener()
