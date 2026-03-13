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

import gdb

from .frames import frame_for_id
from .globalvars import get_global_scope
from .server import export_line, request
from .sources import make_source
from .startup import in_gdb_thread
from .varref import BaseReference

# Map DAP frame IDs to scopes.  This ensures that scopes are re-used.
frame_to_scope = {}


# If the most recent stop was due to a 'finish', and the return value
# could be computed, then this holds that value.  Otherwise it holds
# None.
_last_return_value = None


# When the inferior is re-started, we erase all scope references.  See
# the section "Lifetime of Objects References" in the spec.
@in_gdb_thread
def clear_scopes(event):
    global frame_to_scope
    frame_to_scope = {}
    global _last_return_value
    _last_return_value = None


gdb.events.cont.connect(clear_scopes)


@in_gdb_thread
def set_finish_value(val):
    """Set the current 'finish' value on a stop."""
    global _last_return_value
    _last_return_value = val


# A helper function to compute the value of a symbol.  SYM is either a
# gdb.Symbol, or an object implementing the SymValueWrapper interface.
# FRAME is a frame wrapper, as produced by a frame filter.  Returns a
# tuple of the form (NAME, VALUE), where NAME is the symbol's name and
# VALUE is a gdb.Value.
@in_gdb_thread
def symbol_value(sym, frame):
    inf_frame = frame.inferior_frame()
    # Make sure to select the frame first.  Ideally this would not
    # be needed, but this is a way to set the current language
    # properly so that language-dependent APIs will work.
    inf_frame.select()
    name = str(sym.symbol())
    val = sym.value()
    if val is None:
        # No synthetic value, so must read the symbol value
        # ourselves.
        val = sym.symbol().value(inf_frame)
    elif not isinstance(val, gdb.Value):
        val = gdb.Value(val)
    return (name, val)


class _ScopeReference(BaseReference):
    def __init__(self, name, hint, frameId: int, var_list):
        super().__init__(name)
        self.hint = hint
        self.frameId = frameId
        # VAR_LIST might be any kind of iterator, but it's convenient
        # here if it is just a collection.
        self.var_list = tuple(var_list)

    def to_object(self):
        result = super().to_object()
        result["presentationHint"] = self.hint
        # How would we know?
        result["expensive"] = False
        result["namedVariables"] = self.child_count()
        frame = frame_for_id(self.frameId)
        if frame.line() is not None:
            result["line"] = export_line(frame.line())
        filename = frame.filename()
        if filename is not None:
            result["source"] = make_source(filename)
        return result

    def has_children(self):
        return True

    def child_count(self):
        return len(self.var_list)

    @in_gdb_thread
    def fetch_one_child(self, idx):
        return symbol_value(self.var_list[idx], frame_for_id(self.frameId))


# A _ScopeReference that wraps the 'finish' value.  Note that this
# object is only created if such a value actually exists.
class _FinishScopeReference(_ScopeReference):
    def __init__(self, frameId):
        super().__init__("Return", "returnValue", frameId, ())

    def child_count(self):
        return 1

    def fetch_one_child(self, idx):
        assert idx == 0
        global _last_return_value
        return ("(return)", _last_return_value)


class _RegisterReference(_ScopeReference):
    def __init__(self, name, frameId):
        super().__init__(
            name,
            "registers",
            frameId,
            frame_for_id(frameId).inferior_frame().architecture().registers(),
        )

    @in_gdb_thread
    def fetch_one_child(self, idx):
        return (
            self.var_list[idx].name,
            frame_for_id(self.frameId)
            .inferior_frame()
            .read_register(self.var_list[idx]),
        )


@request("scopes")
def scopes(*, frameId: int, **extra):
    global _last_return_value
    global frame_to_scope
    if frameId in frame_to_scope:
        scopes = frame_to_scope[frameId]
    else:
        frame = frame_for_id(frameId)
        scopes = []
        # Make sure to handle the None case as well as the empty
        # iterator case.
        args = tuple(frame.frame_args() or ())
        if args:
            scopes.append(_ScopeReference("Arguments", "arguments", frameId, args))
        has_return_value = frameId == 0 and _last_return_value is not None
        # Make sure to handle the None case as well as the empty
        # iterator case.
        locs = tuple(frame.frame_locals() or ())
        if locs:
            scopes.append(_ScopeReference("Locals", "locals", frameId, locs))
        scopes.append(_RegisterReference("Registers", frameId))
        if has_return_value:
            scopes.append(_FinishScopeReference(frameId))
        frame_to_scope[frameId] = scopes
        global_scope = get_global_scope(frame)
        if global_scope is not None:
            scopes.append(global_scope)
    return {"scopes": [x.to_object() for x in scopes]}
