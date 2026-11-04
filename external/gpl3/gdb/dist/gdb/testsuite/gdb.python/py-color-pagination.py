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

import gdb

basic_colors = ["black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"]


def write(mode, text):
    if mode == "write":
        gdb.write(text)
    else:
        print(text, end="")


class ColorTester(gdb.Command):
    def __init__(self):
        super().__init__("color-fill", gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        mode = args
        str = "<" + "-" * 78 + ">"
        for i in range(0, 20):
            for color_name in basic_colors:
                c = gdb.Color(color_name)
                write(mode, c.escape_sequence(True))
                write(mode, str)

        default = gdb.Color("none")
        write(mode, default.escape_sequence(True))
        write(mode, "\n")


ColorTester()
