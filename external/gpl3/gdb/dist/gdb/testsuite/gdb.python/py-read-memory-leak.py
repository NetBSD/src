# Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

import sys

# Avoid generating
# src/gdb/testsuite/gdb.python/__pycache__/gdb_leak_detector.cpython-<n>.pyc.
sys.dont_write_bytecode = True

import gdb_leak_detector


class read_leak_detector(gdb_leak_detector.gdb_leak_detector):
    def __init__(self):
        super().__init__(__file__)
        self.mem_buf = None
        self.addr = gdb.parse_and_eval("px")
        self.inf = gdb.inferiors()[0]

    def allocate(self):
        self.mem_buf = self.inf.read_memory(self.addr, 4096)

    def deallocate(self):
        self.mem_buf = None


read_leak_detector().run()
