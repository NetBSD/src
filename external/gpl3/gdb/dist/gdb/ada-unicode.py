#!/usr/bin/env python3

# Generate Unicode case-folding table for Ada.

# Copyright (C) 2022-2024 Free Software Foundation, Inc.

# This file is part of GDB.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This generates the ada-casefold.h header.
# Usage:
#   python ada-unicode.py

import gdbcopyright


class Range:
    def __init__(self, range_start: int, upper_delta: int, lower_delta: int):
        self._range_start = range_start
        self._range_end = range_start
        self._upper_delta = upper_delta
        self._lower_delta = lower_delta

    # The start of the range.
    @property
    def range_start(self):
        return self._range_start

    # The end of the range.
    @property
    def range_end(self):
        return self._range_end

    @range_end.setter
    def range_end(self, val: int):
        self._range_end = val

    # The delta between RANGE_START and the upper-case variant of that
    # character.
    @property
    def upper_delta(self):
        return self._upper_delta

    # The delta between RANGE_START and the lower-case variant of that
    # character.
    @property
    def lower_delta(self):
        return self._lower_delta


# The current range we are processing.  If None,  then we're outside of a range.
current_range: Range | None = None

# All the ranges found and completed so far.
all_ranges: list[Range] = []


def finish_range():
    global current_range

    if current_range is not None:
        all_ranges.append(current_range)
        current_range = None


def process_codepoint(val: int):
    global current_range

    c = chr(val)
    low = c.lower()
    up = c.upper()
    # U+00DF ("LATIN SMALL LETTER SHARP S", aka eszsett) traditionally
    # upper-cases to the two-character string "SS" (the capital form
    # is a relatively recent addition -- 2017).  Our simple scheme
    # can't handle this, so we skip it.  Also, because our approach
    # just represents runs of characters with identical folding
    # deltas, this change must terminate the current run.
    if (c == low and c == up) or len(low) != 1 or len(up) != 1:
        finish_range()
        return
    updelta = ord(up) - val
    lowdelta = ord(low) - val

    if current_range is not None and (
        updelta != current_range.upper_delta or lowdelta != current_range.lower_delta
    ):
        finish_range()

    if current_range is None:
        current_range = Range(val, updelta, lowdelta)

    current_range.range_end = val


for c in range(0, 0x10FFFF):
    process_codepoint(c)

with open("ada-casefold.h", "w") as f:
    print(
        gdbcopyright.copyright("ada-unicode.py", "UTF-32 case-folding for GDB"),
        file=f,
    )
    print("", file=f)
    for r in all_ranges:
        print(
            f"   {{{r.range_start}, {r.range_end}, {r.upper_delta}, {r.lower_delta}}},",
            file=f,
        )
