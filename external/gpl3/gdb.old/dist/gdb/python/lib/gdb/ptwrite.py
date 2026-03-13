# Ptwrite utilities.
# Copyright (C) 2023 Free Software Foundation, Inc.

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

"""Utilities for working with ptwrite filters."""

import gdb

# _ptwrite_filter contains the per thread copies of the filter function.
# The keys are tuples of inferior id and thread id.
# The filter functions are created for each thread by calling the
# _ptwrite_filter_factory.
_ptwrite_filter = {}
_ptwrite_filter_factory = None


def _ptwrite_exit_handler(event):
    """Exit handler to prune _ptwrite_filter on thread exit."""
    _ptwrite_filter.pop(event.inferior_thread.ptid, None)


gdb.events.thread_exited.connect(_ptwrite_exit_handler)


def _clear_traces():
    """Helper function to clear the trace of all threads."""
    current_thread = gdb.selected_thread()

    for inferior in gdb.inferiors():
        for thread in inferior.threads():
            thread.switch()
            recording = gdb.current_recording()
            if recording is not None:
                recording.clear()

    current_thread.switch()


def register_filter_factory(filter_factory_):
    """Register the ptwrite filter factory."""
    if filter_factory_ is not None and not callable(filter_factory_):
        raise TypeError("The filter factory must be callable or 'None'.")

    # Clear the traces of all threads of all inferiors to force
    # re-decoding with the new filter.
    _clear_traces()

    _ptwrite_filter.clear()
    global _ptwrite_filter_factory
    _ptwrite_filter_factory = filter_factory_


def get_filter():
    """Returns the filter of the current thread."""
    thread = gdb.selected_thread()
    key = thread.ptid

    # Create a new filter for new threads.
    if key not in _ptwrite_filter:
        if _ptwrite_filter_factory is not None:
            _ptwrite_filter[key] = _ptwrite_filter_factory(thread)
        else:
            return None

    return _ptwrite_filter[key]
