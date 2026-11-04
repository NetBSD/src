# Copyright (C) 2008-2025 Free Software Foundation, Inc.

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

# Reduced copy of /usr/share/gcc-15/python/libstdcxx/v6/printers.py.

import gdb
import gdb.printing
import gdb.types


class FilteringTypePrinter(object):

    def __init__(self, template, name, targ1=None):
        self._template = template
        self.name = name
        self._targ1 = targ1
        self.enabled = True

    class _recognizer(object):
        def __init__(self, template, name, targ1):
            self._template = template
            self.name = name
            self._targ1 = targ1
            self._type_obj = None

        def recognize(self, type_obj):
            if type_obj.tag is None:
                return None

            if self._type_obj is None:
                if self._targ1 is not None:
                    s = "{}<{}".format(self._template, self._targ1)
                    if not type_obj.tag.startswith(s):
                        return None
                elif not type_obj.tag.startswith(self._template):
                    return None

                try:
                    self._type_obj = gdb.lookup_type(self.name).strip_typedefs()
                except:
                    pass

            if self._type_obj is None:
                return None

            t1 = gdb.types.get_basic_type(self._type_obj)
            t2 = gdb.types.get_basic_type(type_obj)
            if t1 == t2:
                return self.name

            if self._template.split("::")[-1] == "basic_string":
                s1 = self._type_obj.tag.replace("__cxx11::", "")
                s2 = type_obj.tag.replace("__cxx11::", "")
                if s1 == s2:
                    return self.name

            return None

    def instantiate(self):
        return self._recognizer(self._template, self.name, self._targ1)


def add_one_type_printer(obj, template, name, targ1=None):
    printer = FilteringTypePrinter("std::" + template, "std::" + name, targ1)
    gdb.types.register_type_printer(obj, printer)


def register_type_printers(obj):
    for ch in (("", "char"),):
        add_one_type_printer(obj, "__cxx11::basic_string", ch[0] + "string", ch[1])
