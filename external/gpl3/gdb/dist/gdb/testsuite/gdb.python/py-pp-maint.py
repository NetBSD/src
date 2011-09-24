# Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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

# This file is part of the GDB testsuite.  It tests python pretty
# printers.

import re
import gdb.types
import gdb.printing


def lookup_function_lookup_test(val):
    class PrintFunctionLookup(object):
        def __init__(self, val):
            self.val = val

        def to_string(self):
            return ("x=<" + str(self.val["x"]) +
                    "> y=<" + str(self.val["y"]) + ">")

    typename = gdb.types.get_basic_type(val.type).tag
    # Note: typename could be None.
    if typename == "function_lookup_test":
        return PrintFunctionLookup(val)
    return None


class pp_s:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = self.val["a"]
        b = self.val["b"]
        if a.address != b:
            raise Exception("&a(%s) != b(%s)" % (str(a.address), str(b)))
        return "a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


class pp_ss:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("pp-test")

    pp.add_printer('struct s', '^struct s$', pp_s)
    pp.add_printer('s', '^s$', pp_s)

    # Use a lambda this time to exercise doing things this way.
    pp.add_printer('struct ss', '^struct ss$', lambda val: pp_ss(val))
    pp.add_printer('ss', '^ss$', lambda val: pp_ss(val))

    return pp


gdb.printing.register_pretty_printer(gdb, lookup_function_lookup_test)
gdb.printing.register_pretty_printer(gdb, build_pretty_printer())
