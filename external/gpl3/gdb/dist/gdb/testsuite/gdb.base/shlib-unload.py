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

# Breakpoint modification events will be recorded in this dictionary.
# The keys are the b/p numbers, and the values are the number of
# modification events seen.
bp_modified_counts = {}


# Record breakpoint modification events into the global
# bp_modified_counts dictionary.
def bp_modified(bp):
    global bp_modified_counts
    if bp.number not in bp_modified_counts:
        bp_modified_counts[bp.number] = 1
    else:
        bp_modified_counts[bp.number] += 1


# Register the event handler.
gdb.events.breakpoint_modified.connect(bp_modified)
