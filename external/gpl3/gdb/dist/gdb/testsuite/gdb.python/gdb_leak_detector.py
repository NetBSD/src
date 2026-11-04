# Copyright (C) 2021-2025 Free Software Foundation, Inc.
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

# Defines a base class, which can be sub-classed, in order to run
# memory leak tests on some aspects of GDB's Python API.  See the
# comments on the gdb_leak_detector class for more details.

import os
import tracemalloc

import gdb


# This class must be sub-classed to create a memory leak test.  The
# sub-classes __init__ method should call the parent classes __init__
# method, and the sub-class should override allocate() and
# deallocate().  See the comments on the various methods below for
# more details of required arguments and expected usage.
class gdb_leak_detector:

    # Class initialisation.  FILENAME is the file in which the
    # sub-class is defined, usually passed as just '__file__'.  This
    # is used when looking for memory allocations; only allocations in
    # FILENAME are considered.
    def __init__(self, filename):
        self.filters = [tracemalloc.Filter(True, "*" + os.path.basename(filename))]

    # Internal helper function to actually run the test.  Calls the
    # allocate() method to allocate an object from GDB's Python API.
    # When CLEAR is True the object will then be deallocated by
    # calling deallocate(), otherwise, deallocate() is not called.
    #
    # Finally, this function checks for any memory allocatios
    # originating from 'self.filename' that have not been freed, and
    # returns the total (in bytes) of the memory that has been
    # allocated, but not freed.
    def _do_test(self, clear):
        # Start tracing, and take a snapshot of the current allocations.
        tracemalloc.start()
        snapshot1 = tracemalloc.take_snapshot()

        # Generate the GDB Python API object by calling the allocate
        # method.
        self.allocate()

        # Possibly clear the reference to the allocated object.
        if clear:
            self.deallocate()

        # Now grab a second snapshot of memory allocations, and stop
        # tracing memory allocations.
        snapshot2 = tracemalloc.take_snapshot()
        tracemalloc.stop()

        # Filter the snapshots; we only care about allocations originating
        # from this file.
        snapshot1 = snapshot1.filter_traces(self.filters)
        snapshot2 = snapshot2.filter_traces(self.filters)

        # Compare the snapshots, this leaves only things that were
        # allocated, but not deallocated since the first snapshot.
        stats = snapshot2.compare_to(snapshot1, "traceback")

        # Total up all the allocated things.
        total = 0
        for stat in stats:
            total += stat.size_diff
        return total

    # Run the memory leak test.  Prints 'PASS' if successful,
    # otherwise, raises an exception (of type GdbError).
    def run(self):
        # The first time we run this some global state will be allocated which
        # shows up as memory that is allocated, but not released.  So, run the
        # test once and discard the result.
        self._do_test(True)

        # Now run the test twice, the first time we clear our global reference
        # to the allocated object, which should allow Python to deallocate the
        # object.  The second time we hold onto the global reference, preventing
        # Python from performing the deallocation.
        bytes_with_clear = self._do_test(True)
        bytes_without_clear = self._do_test(False)

        # If there are any allocations left over when we cleared the reference
        # (and expected deallocation) then this indicates a leak.
        if bytes_with_clear > 0:
            raise gdb.GdbError("memory leak when object reference was released")

        # If there are no allocations showing when we hold onto a reference,
        # then this likely indicates that the testing infrastructure is broken,
        # and we're no longer spotting the allocations at all.
        if bytes_without_clear == 0:
            raise gdb.GdbError("object is unexpectedly not showing as allocated")

        # Print a PASS message that the TCL script can see.
        print("PASS")

    # Sub-classes must override this method.  Allocate an object (or
    # multiple objects) from GDB's Python API.  Store references to
    # these objects within SELF.
    def allocate(self):
        raise NotImplementedError("allocate() not implemented")

    # Sub-classes must override this method.  Deallocate the object(s)
    # allocated by the allocate() method.  All that is required is for
    # the references created in allocate() to be set to None.
    def deallocate(self):
        raise NotImplementedError("allocate() not implemented")
