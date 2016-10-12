# Copyright (C) 2009-2016 Free Software Foundation, Inc.
#
# This file is part of GDB.
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
import os.path

class TypeFlag:
    """A class that allows us to store a flag name, its short name,
    and its value.

    In the GDB sources, struct type has a component called instance_flags
    in which the value is the addition of various flags.  These flags are
    defined by two enumerates: type_flag_value, and type_instance_flag_value.
    This class helps us recreate a list with all these flags that is
    easy to manipulate and sort.  Because all flag names start with either
    TYPE_FLAG_ or TYPE_INSTANCE_FLAG_, a short_name attribute is provided
    that strips this prefix.

    ATTRIBUTES
      name:  The enumeration name (eg: "TYPE_FLAG_UNSIGNED").
      value: The associated value.
      short_name: The enumeration name, with the suffix stripped.
    """
    def __init__(self, name, value):
        self.name = name
        self.value = value
        self.short_name = name.replace("TYPE_FLAG_", '')
        if self.short_name == name:
            self.short_name = name.replace("TYPE_INSTANCE_FLAG_", '')
    def __cmp__(self, other):
        """Sort by value order."""
        return self.value.__cmp__(other.value)

# A list of all existing TYPE_FLAGS_* and TYPE_INSTANCE_FLAGS_*
# enumerations, stored as TypeFlags objects.  Lazy-initialized.
TYPE_FLAGS = None

class TypeFlagsPrinter:
    """A class that prints a decoded form of an instance_flags value.

    This class uses a global named TYPE_FLAGS, which is a list of
    all defined TypeFlag values.  Using a global allows us to compute
    this list only once.

    This class relies on a couple of enumeration types being defined.
    If not, then printing of the instance_flag is going to be degraded,
    but it's not a fatal error.
    """
    def __init__(self, val):
        self.val = val
    def __str__(self):
        global TYPE_FLAGS
        if TYPE_FLAGS is None:
            self.init_TYPE_FLAGS()
        if not self.val:
            return "0"
        if TYPE_FLAGS:
            flag_list = [flag.short_name for flag in TYPE_FLAGS
                         if self.val & flag.value]
        else:
            flag_list = ["???"]
        return "0x%x [%s]" % (self.val, "|".join(flag_list))
    def init_TYPE_FLAGS(self):
        """Initialize the TYPE_FLAGS global as a list of TypeFlag objects.
        This operation requires the search of a couple of enumeration types.
        If not found, a warning is printed on stdout, and TYPE_FLAGS is
        set to the empty list.

        The resulting list is sorted by increasing value, to facilitate
        printing of the list of flags used in an instance_flags value.
        """
        global TYPE_FLAGS
        TYPE_FLAGS = []
        try:
            flags = gdb.lookup_type("enum type_flag_value")
        except:
            print("Warning: Cannot find enum type_flag_value type.")
            print("         `struct type' pretty-printer will be degraded")
            return
        try:
            iflags = gdb.lookup_type("enum type_instance_flag_value")
        except:
            print("Warning: Cannot find enum type_instance_flag_value type.")
            print("         `struct type' pretty-printer will be degraded")
            return
        # Note: TYPE_FLAG_MIN is a duplicate of TYPE_FLAG_UNSIGNED,
        # so exclude it from the list we are building.
        TYPE_FLAGS = [TypeFlag(field.name, field.enumval)
                      for field in flags.fields()
                      if field.name != 'TYPE_FLAG_MIN']
        TYPE_FLAGS += [TypeFlag(field.name, field.enumval)
                       for field in iflags.fields()]
        TYPE_FLAGS.sort()

class StructTypePrettyPrinter:
    """Pretty-print an object of type struct type"""
    def __init__(self, val):
        self.val = val
    def to_string(self):
        fields = []
        fields.append("pointer_type = %s" % self.val['pointer_type'])
        fields.append("reference_type = %s" % self.val['reference_type'])
        fields.append("chain = %s" % self.val['reference_type'])
        fields.append("instance_flags = %s"
                      % TypeFlagsPrinter(self.val['instance_flags']))
        fields.append("length = %d" % self.val['length'])
        fields.append("main_type = %s" % self.val['main_type'])
        return "\n{" + ",\n ".join(fields) + "}"

class StructMainTypePrettyPrinter:
    """Pretty-print an objet of type main_type"""
    def __init__(self, val):
        self.val = val
    def flags_to_string(self):
        """struct main_type contains a series of components that
        are one-bit ints whose name start with "flag_".  For instance:
        flag_unsigned, flag_stub, etc.  In essence, these components are
        really boolean flags, and this method prints a short synthetic
        version of the value of all these flags.  For instance, if
        flag_unsigned and flag_static are the only components set to 1,
        this function will return "unsigned|static".
        """
        fields = [field.name.replace("flag_", "")
                  for field in self.val.type.fields()
                  if field.name.startswith("flag_")
                     and self.val[field.name]]
        return "|".join(fields)
    def owner_to_string(self):
        """Return an image of component "owner".
        """
        if self.val['flag_objfile_owned'] != 0:
            return "%s (objfile)" % self.val['owner']['objfile']
        else:
            return "%s (gdbarch)" % self.val['owner']['gdbarch']
    def struct_field_location_img(self, field_val):
        """Return an image of the loc component inside the given field
        gdb.Value.
        """
        loc_val = field_val['loc']
        loc_kind = str(field_val['loc_kind'])
        if loc_kind == "FIELD_LOC_KIND_BITPOS":
            return 'bitpos = %d' % loc_val['bitpos']
        elif loc_kind == "FIELD_LOC_KIND_ENUMVAL":
            return 'enumval = %d' % loc_val['enumval']
        elif loc_kind == "FIELD_LOC_KIND_PHYSADDR":
            return 'physaddr = 0x%x' % loc_val['physaddr']
        elif loc_kind == "FIELD_LOC_KIND_PHYSNAME":
            return 'physname = %s' % loc_val['physname']
        elif loc_kind == "FIELD_LOC_KIND_DWARF_BLOCK":
            return 'dwarf_block = %s' % loc_val['dwarf_block']
        else:
            return 'loc = ??? (unsupported loc_kind value)'
    def struct_field_img(self, fieldno):
        """Return an image of the main_type field number FIELDNO.
        """
        f = self.val['flds_bnds']['fields'][fieldno]
        label = "flds_bnds.fields[%d]:" % fieldno
        if f['artificial']:
            label += " (artificial)"
        fields = []
        fields.append("name = %s" % f['name'])
        fields.append("type = %s" % f['type'])
        fields.append("loc_kind = %s" % f['loc_kind'])
        fields.append("bitsize = %d" % f['bitsize'])
        fields.append(self.struct_field_location_img(f))
        return label + "\n" + "  {" + ",\n   ".join(fields) + "}"
    def bounds_img(self):
        """Return an image of the main_type bounds.
        """
        b = self.val['flds_bnds']['bounds'].dereference()
        low = str(b['low'])
        if b['low_undefined'] != 0:
            low += " (undefined)"
        high = str(b['high'])
        if b['high_undefined'] != 0:
            high += " (undefined)"
        return "flds_bnds.bounds = {%s, %s}" % (low, high)
    def type_specific_img(self):
        """Return a string image of the main_type type_specific union.
        Only the relevant component of that union is printed (based on
        the value of the type_specific_kind field.
        """
        type_specific_kind = str(self.val['type_specific_field'])
        type_specific = self.val['type_specific']
        if type_specific_kind == "TYPE_SPECIFIC_NONE":
            img = 'type_specific_field = %s' % type_specific_kind
        elif type_specific_kind == "TYPE_SPECIFIC_CPLUS_STUFF":
            img = "cplus_stuff = %s" % type_specific['cplus_stuff']
        elif type_specific_kind == "TYPE_SPECIFIC_GNAT_STUFF":
            img = ("gnat_stuff = {descriptive_type = %s}"
                   % type_specific['gnat_stuff']['descriptive_type'])
        elif type_specific_kind == "TYPE_SPECIFIC_FLOATFORMAT":
            img = "floatformat[0..1] = %s" % type_specific['floatformat']
        elif type_specific_kind == "TYPE_SPECIFIC_FUNC":
            img = ("calling_convention = %d"
                   % type_specific['func_stuff']['calling_convention'])
            # tail_call_list is not printed.
        elif type_specific_kind == "TYPE_SPECIFIC_SELF_TYPE":
            img = "self_type = %s" % type_specific['self_type']
        else:
            img = ("type_specific = ??? (unknown type_secific_kind: %s)"
                   % type_specific_kind)
        return img

    def to_string(self):
        """Return a pretty-printed image of our main_type.
        """
        fields = []
        fields.append("name = %s" % self.val['name'])
        fields.append("tag_name = %s" % self.val['tag_name'])
        fields.append("code = %s" % self.val['code'])
        fields.append("flags = [%s]" % self.flags_to_string())
        fields.append("owner = %s" % self.owner_to_string())
        fields.append("target_type = %s" % self.val['target_type'])
        if self.val['nfields'] > 0:
            for fieldno in range(self.val['nfields']):
                fields.append(self.struct_field_img(fieldno))
        if self.val['code'] == gdb.TYPE_CODE_RANGE:
            fields.append(self.bounds_img())
        fields.append(self.type_specific_img())

        return "\n{" + ",\n ".join(fields) + "}"

def type_lookup_function(val):
    """A routine that returns the correct pretty printer for VAL
    if appropriate.  Returns None otherwise.
    """
    if val.type.tag == "type":
        return StructTypePrettyPrinter(val)
    elif val.type.tag == "main_type":
        return StructMainTypePrettyPrinter(val)
    return None

def register_pretty_printer(objfile):
    """A routine to register a pretty-printer against the given OBJFILE.
    """
    objfile.pretty_printers.append(type_lookup_function)

if __name__ == "__main__":
    if gdb.current_objfile() is not None:
        # This is the case where this script is being "auto-loaded"
        # for a given objfile.  Register the pretty-printer for that
        # objfile.
        register_pretty_printer(gdb.current_objfile())
    else:
        # We need to locate the objfile corresponding to the GDB
        # executable, and register the pretty-printer for that objfile.
        # FIXME: The condition used to match the objfile is too simplistic
        # and will not work on Windows.
        for objfile in gdb.objfiles():
            if os.path.basename(objfile.filename) == "gdb":
                objfile.pretty_printers.append(type_lookup_function)
