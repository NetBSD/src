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

import os
import tracemalloc

import gdb

# A global variable in which we store a reference to the memory buffer
# returned from gdb.Inferior.read_memory().
mem_buf = None


# A global filters list, we only care about memory allocations
# originating from this script.
filters = [tracemalloc.Filter(True, "*" + os.path.basename(__file__))]


# Run the test.  When CLEAR is True we clear the global INF variable
# before comparing the before and after memory allocation traces.
# When CLEAR is False we leave INF set to reference the gdb.Inferior
# object, thus preventing the gdb.Inferior from being deallocated.
def test(clear):
    global filters, mem_buf

    addr = gdb.parse_and_eval("px")
    inf = gdb.inferiors()[0]

    # Start tracing, and take a snapshot of the current allocations.
    tracemalloc.start()
    snapshot1 = tracemalloc.take_snapshot()

    # Read from the inferior, this allocate a memory buffer object.
    mem_buf = inf.read_memory(addr, 4096)

    # Possibly clear the global INF variable.
    if clear:
        mem_buf = None

    # Now grab a second snapshot of memory allocations, and stop
    # tracing memory allocations.
    snapshot2 = tracemalloc.take_snapshot()
    tracemalloc.stop()

    # Filter the snapshots; we only care about allocations originating
    # from this file.
    snapshot1 = snapshot1.filter_traces(filters)
    snapshot2 = snapshot2.filter_traces(filters)

    # Compare the snapshots, this leaves only things that were
    # allocated, but not deallocated since the first snapshot.
    stats = snapshot2.compare_to(snapshot1, "traceback")

    # Total up all the allocated things.
    total = 0
    for stat in stats:
        total += stat.size_diff
    return total


# The first time we run this some global state will be allocated which
# shows up as memory that is allocated, but not released.  So, run the
# test once and discard the result.
test(True)

# Now run the test twice, the first time we clear our global reference
# to the memory buffer object, which should allow Python to deallocate
# the object.  The second time we hold onto the global reference,
# preventing Python from performing the deallocation.
bytes_with_clear = test(True)
bytes_without_clear = test(False)

# The bug that used to exist in GDB was that even when we released the
# global reference the gdb.Inferior object would not be deallocated.
if bytes_with_clear > 0:
    raise gdb.GdbError("memory leak when memory buffer should be released")
if bytes_without_clear == 0:
    raise gdb.GdbError("memory buffer object is no longer allocated")

# Print a PASS message that the test script can see.
print("PASS")
