# Copyright (C) 2025 Free Software Foundation, Inc.
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

import sys

# Some text to print that includes styling.
OUT = "\033[35;1mOUTPUT\033[m"


def print_it():
    # Print to stderr avoids any buffering, showing the bug.
    for c in OUT:
        print(c, end="", file=sys.stderr)
    print(file=sys.stderr)
