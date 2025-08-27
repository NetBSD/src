# Copyright 2024 Free Software Foundation, Inc.

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

from .sources import make_source
from .startup import in_gdb_thread
from .varref import BaseReference

# Map a block identifier to a scope object.
_id_to_scope = {}


# Arrange to clear the scope references when the inferior runs.
@in_gdb_thread
def clear(event):
    global _id_to_scope
    _id_to_scope = {}


gdb.events.cont.connect(clear)


# A scope that holds static and/or global variables.
class _Globals(BaseReference):
    def __init__(self, filename, var_list):
        super().__init__("Globals")
        self.filename = filename
        self.var_list = var_list

    def to_object(self):
        result = super().to_object()
        result["presentationHint"] = "globals"
        # How would we know?
        result["expensive"] = False
        result["namedVariables"] = self.child_count()
        if self.filename is not None:
            result["source"] = make_source(self.filename)
        return result

    def has_children(self):
        # This object won't even be created if there are no variables
        # to return.
        return True

    def child_count(self):
        return len(self.var_list)

    @in_gdb_thread
    def fetch_one_child(self, idx):
        sym = self.var_list[idx]
        return (sym.name, sym.value())


@in_gdb_thread
def get_global_scope(frame):
    """Given a frame decorator, return the corresponding global scope
    object.

    If the frame does not have a block, or if the CU does not have
    globals (that is, empty static and global blocks), return None."""
    inf_frame = frame.inferior_frame()
    # It's unfortunate that this API throws instead of returning None.
    try:
        block = inf_frame.block()
    except RuntimeError:
        return None

    global _id_to_scope
    block = block.static_block
    if block in _id_to_scope:
        return _id_to_scope[block]

    syms = []
    block_iter = block
    while block_iter is not None:
        syms += [sym for sym in block_iter if sym.is_variable and not sym.is_artificial]
        block_iter = block_iter.superblock

    if len(syms) == 0:
        return None

    result = _Globals(frame.filename(), syms)
    _id_to_scope[block] = result

    return result
