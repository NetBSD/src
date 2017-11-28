# Copyright (C) 2013-2016 Free Software Foundation, Inc.

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

import re
import gdb

class pp_s (object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        m = self.val["m"]
        return "m=<" + str(self.val["m"]) + ">"

class pp_ss (object):
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "super struct"

    def children (self):
        yield 'a', self.val['a']
        yield 'b', self.val['b']


def lookup_function (val):
    "Look-up and return a pretty-printer that can print val."

    # Get the type.
    type = val.type

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified ().strip_typedefs ()

    # Get the type name.    
    typename = type.tag
    if typename == None:
        return None

    # Iterate over local dictionary of types to determine
    # if a printer is registered for that type.  Return an
    # instantiation of the printer if found.
    for function in pretty_printers_dict:
        if function.match (typename):
            return pretty_printers_dict[function] (val)
        
    # Cannot find a pretty printer.  Return None.
    return None


def register_pretty_printers ():
    pretty_printers_dict[re.compile ('^s$')] = pp_s
    pretty_printers_dict[re.compile ('^ss$')] = pp_ss

pretty_printers_dict = {}

register_pretty_printers ()
gdb.pretty_printers.append (lookup_function)
