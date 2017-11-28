# Copyright (C) 2015-2017 Free Software Foundation, Inc.

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
from gdb.unwinder import Unwinder

class FrameId(object):

    def __init__(self, sp, pc):
        self._sp = sp
        self._pc = pc

    @property
    def sp(self):
        return self._sp

    @property
    def pc(self):
        return self._pc


class TestUnwinder(Unwinder):
    AMD64_RBP = 6
    AMD64_RSP = 7
    AMD64_RIP = 16

    def __init__(self):
        Unwinder.__init__(self, "test unwinder")
        self.char_ptr_t = gdb.lookup_type("unsigned char").pointer()
        self.char_ptr_ptr_t = self.char_ptr_t.pointer()

    def _read_word(self, address):
        return address.cast(self.char_ptr_ptr_t).dereference()

    def __call__(self, pending_frame):
        """Test unwinder written in Python.

        This unwinder can unwind the frames that have been deliberately
        corrupted in a specific way (functions in the accompanying
        py-unwind.c file do that.)
        This code is only on AMD64.
        On AMD64 $RBP points to the innermost frame (unless the code
        was compiled with -fomit-frame-pointer), which contains the
        address of the previous frame at offset 0. The functions
        deliberately corrupt their frames as follows:
                     Before                 After
                   Corruption:           Corruption:
                +--------------+       +--------------+
        RBP-8   |              |       | Previous RBP |
                +--------------+       +--------------+
        RBP     + Previous RBP |       |    RBP       |
                +--------------+       +--------------+
        RBP+8   | Return RIP   |       | Return  RIP  |
                +--------------+       +--------------+
        Old SP  |              |       |              |

        This unwinder recognizes the corrupt frames by checking that
        *RBP == RBP, and restores previous RBP from the word above it.
        """
        try:
            # NOTE: the registers in Unwinder API can be referenced
            # either by name or by number. The code below uses both
            # to achieve more coverage.
            bp = pending_frame.read_register("rbp").cast(self.char_ptr_t)
            if self._read_word(bp) != bp:
                return None
            # Found the frame that the test program has corrupted for us.
            # The correct BP for the outer frame has been saved one word
            # above, previous IP and SP are at the expected places.
            previous_bp = self._read_word(bp - 8)
            previous_ip = self._read_word(bp + 8)
            previous_sp = bp + 16

            frame_id = FrameId(
                pending_frame.read_register(TestUnwinder.AMD64_RSP),
                pending_frame.read_register(TestUnwinder.AMD64_RIP))
            unwind_info = pending_frame.create_unwind_info(frame_id)
            unwind_info.add_saved_register(TestUnwinder.AMD64_RBP,
                                           previous_bp)
            unwind_info.add_saved_register("rip", previous_ip)
            unwind_info.add_saved_register("rsp", previous_sp)
            return unwind_info
        except (gdb.error, RuntimeError):
            return None

gdb.unwinder.register_unwinder(None, TestUnwinder(), True)
print("Python script imported")
