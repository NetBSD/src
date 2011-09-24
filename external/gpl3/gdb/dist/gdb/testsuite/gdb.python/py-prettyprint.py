# Copyright (C) 2008, 2009, 2010, 2011 Free Software Foundation, Inc.

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

# Test returning a Value from a printer.
class string_print:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['whybother']['contents']

# Test a class-based printer.
class ContainerPrinter:
    class _iterator:
        def __init__ (self, pointer, len):
            self.start = pointer
            self.pointer = pointer
            self.end = pointer + len

        def __iter__(self):
            return self

        def next(self):
            if self.pointer == self.end:
                raise StopIteration
            result = self.pointer
            self.pointer = self.pointer + 1
            return ('[%d]' % int (result - self.start), result.dereference())

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return 'container %s with %d elements' % (self.val['name'], self.val['len'])

    def children(self):
        return self._iterator(self.val['elements'], self.val['len'])

# Flag to make NoStringContainerPrinter throw an exception.
exception_flag = False

# Test a printer where to_string is None
class NoStringContainerPrinter:
    class _iterator:
        def __init__ (self, pointer, len):
            self.start = pointer
            self.pointer = pointer
            self.end = pointer + len

        def __iter__(self):
            return self

        def next(self):
            if self.pointer == self.end:
                raise StopIteration
            if exception_flag:
                raise gdb.MemoryError, 'hi bob'
            result = self.pointer
            self.pointer = self.pointer + 1
            return ('[%d]' % int (result - self.start), result.dereference())

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return None

    def children(self):
        return self._iterator(self.val['elements'], self.val['len'])

class pp_s:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        a = self.val["a"]
        b = self.val["b"]
        if a.address != b:
            raise Exception("&a(%s) != b(%s)" % (str(a.address), str(b)))
        return " a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"

class pp_ss:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "a=<" + str(self.val["a"]) + "> b=<" + str(self.val["b"]) + ">"

class pp_sss:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "a=<" + str(self.val['a']) + "> b=<" + str(self.val["b"]) + ">"

class pp_multiple_virtual:
    def __init__ (self, val):
        self.val = val

    def to_string (self):
        return "pp value variable is: " + str (self.val['value'])

class pp_vbase1:
    def __init__ (self, val):
        self.val = val

    def to_string (self):
        return "pp class name: " + self.val.type.tag

class pp_nullstr:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return self.val['s'].string(gdb.target_charset())

class pp_ns:
    "Print a std::basic_string of some kind"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        len = self.val['length']
        return self.val['null_str'].string (gdb.target_charset(), length = len)

    def display_hint (self):
        return 'string'

pp_ls_encoding = None

class pp_ls:
    "Print a std::basic_string of some kind"

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if pp_ls_encoding is not None:
            return self.val['lazy_str'].lazy_string(encoding = pp_ls_encoding)
        else:
            return self.val['lazy_str'].lazy_string()

    def display_hint (self):
        return 'string'

class pp_outer:
    "Print struct outer"

    def __init__ (self, val):
        self.val = val

    def to_string (self):
        return "x = %s" % self.val['x']

    def children (self):
        yield 's', self.val['s']
        yield 'x', self.val['x']

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

def disable_lookup_function ():
    lookup_function.enabled = False

def enable_lookup_function ():
    lookup_function.enabled = True

def register_pretty_printers ():
    pretty_printers_dict[re.compile ('^struct s$')]   = pp_s
    pretty_printers_dict[re.compile ('^s$')]   = pp_s
    pretty_printers_dict[re.compile ('^S$')]   = pp_s

    pretty_printers_dict[re.compile ('^struct ss$')]  = pp_ss
    pretty_printers_dict[re.compile ('^ss$')]  = pp_ss
    pretty_printers_dict[re.compile ('^const S &$')]   = pp_s
    pretty_printers_dict[re.compile ('^SSS$')]  = pp_sss
    
    pretty_printers_dict[re.compile ('^VirtualTest$')] =  pp_multiple_virtual
    pretty_printers_dict[re.compile ('^Vbase1$')] =  pp_vbase1

    pretty_printers_dict[re.compile ('^struct nullstr$')] = pp_nullstr
    pretty_printers_dict[re.compile ('^nullstr$')] = pp_nullstr
    
    # Note that we purposely omit the typedef names here.
    # Printer lookup is based on canonical name.
    # However, we do need both tagged and untagged variants, to handle
    # both the C and C++ cases.
    pretty_printers_dict[re.compile ('^struct string_repr$')] = string_print
    pretty_printers_dict[re.compile ('^struct container$')] = ContainerPrinter
    pretty_printers_dict[re.compile ('^struct justchildren$')] = NoStringContainerPrinter
    pretty_printers_dict[re.compile ('^string_repr$')] = string_print
    pretty_printers_dict[re.compile ('^container$')] = ContainerPrinter
    pretty_printers_dict[re.compile ('^justchildren$')] = NoStringContainerPrinter
    
    pretty_printers_dict[re.compile ('^struct ns$')]  = pp_ns
    pretty_printers_dict[re.compile ('^ns$')]  = pp_ns

    pretty_printers_dict[re.compile ('^struct lazystring$')]  = pp_ls
    pretty_printers_dict[re.compile ('^lazystring$')]  = pp_ls

    pretty_printers_dict[re.compile ('^struct outerstruct$')]  = pp_outer
    pretty_printers_dict[re.compile ('^outerstruct$')]  = pp_outer

pretty_printers_dict = {}

register_pretty_printers ()
gdb.pretty_printers.append (lookup_function)
