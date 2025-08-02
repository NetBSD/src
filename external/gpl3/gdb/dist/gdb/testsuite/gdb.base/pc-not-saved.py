# Copyright (C) 2024 Free Software Foundation, Inc.

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
from gdb.unwinder import FrameId, Unwinder

# Cached FrameId.  See set_break_bt_here_frame_id for details.
break_bt_here_frame_id = None


def set_break_bt_here_frame_id(pc, cfa):
    """Call this to pre-calculate the FrameId for the frame our unwinder
    is going to claim, this avoids us having to actually figure out a
    frame-id within the unwinder, something which is going to be hard
    to do in a cross-target way.

    Instead we first run the test without the Python unwinder in
    place, use 'maint print frame-id' to record the frame-id, then,
    after loading this Python script, we all this function to record
    the frame-id that the unwinder should use."""
    global break_bt_here_frame_id
    break_bt_here_frame_id = FrameId(cfa, pc)


class break_unwinding(Unwinder):
    """An unwinder for the function 'break_bt_here'.  This unwinder will
    claim any frame for the function in question, but doesn't provide
    any unwound register values.  Importantly, we don't provide a
    previous $pc value, this means that if we are stopped in
    'break_bt_here' then we should fail to unwind beyond frame #0."""

    def __init__(self):
        Unwinder.__init__(self, "break unwinding")

    def __call__(self, pending_frame):
        pc_desc = pending_frame.architecture().registers().find("pc")
        pc = pending_frame.read_register(pc_desc)

        if pc.is_optimized_out:
            return None

        block = gdb.block_for_pc(pc)
        if block == None:
            return None
        func = block.function
        if func == None:
            return None
        if str(func) != "break_bt_here":
            return None

        global break_bt_here_frame_id
        if break_bt_here_frame_id is None:
            return None

        return pending_frame.create_unwind_info(break_bt_here_frame_id)


gdb.unwinder.register_unwinder(None, break_unwinding(), True)
