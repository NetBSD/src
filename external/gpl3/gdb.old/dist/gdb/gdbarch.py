#!/usr/bin/env python3

# Architecture commands for GDB, the GNU debugger.
#
# Copyright (C) 1998-2023 Free Software Foundation, Inc.
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

import textwrap
import gdbcopyright

# All the components created in gdbarch-components.py.
components = []


def indentation(n_columns):
    """Return string with tabs and spaces to indent line to N_COLUMNS."""
    return "\t" * (n_columns // 8) + " " * (n_columns % 8)


def join_type_and_name(t, n):
    "Combine the type T and the name N into a C declaration."
    if t.endswith("*") or t.endswith("&"):
        return t + n
    else:
        return t + " " + n


def join_params(params):
    """Given a sequence of (TYPE, NAME) pairs, generate a comma-separated
    list of declarations."""
    params = [join_type_and_name(p[0], p[1]) for p in params]
    return ", ".join(params)


class _Component:
    "Base class for all components."

    def __init__(self, **kwargs):
        for key in kwargs:
            setattr(self, key, kwargs[key])
        components.append(self)

        # It doesn't make sense to have a check of the result value
        # for a function or method with void return type.
        if self.type == "void" and self.result_checks:
            raise Exception("can't have result checks with a void return type")

    def get_predicate(self):
        "Return the expression used for validity checking."
        assert self.predicate and not isinstance(self.invalid, str)
        if self.predefault:
            predicate = f"gdbarch->{self.name} != {self.predefault}"
        elif isinstance(c, Value):
            predicate = f"gdbarch->{self.name} != 0"
        else:
            predicate = f"gdbarch->{self.name} != NULL"
        return predicate


class Info(_Component):
    "An Info component is copied from the gdbarch_info."

    def __init__(self, *, name, type, printer=None):
        super().__init__(name=name, type=type, printer=printer)
        # This little hack makes the generator a bit simpler.
        self.predicate = None


class Value(_Component):
    "A Value component is just a data member."

    def __init__(
        self,
        *,
        name,
        type,
        comment=None,
        predicate=None,
        predefault=None,
        postdefault=None,
        invalid=None,
        printer=None,
    ):
        super().__init__(
            comment=comment,
            name=name,
            type=type,
            predicate=predicate,
            predefault=predefault,
            postdefault=postdefault,
            invalid=invalid,
            printer=printer,
        )


class Function(_Component):
    "A Function component is a function pointer member."

    def __init__(
        self,
        *,
        name,
        type,
        params,
        comment=None,
        predicate=None,
        predefault=None,
        postdefault=None,
        invalid=None,
        printer=None,
        param_checks=None,
        result_checks=None,
    ):
        super().__init__(
            comment=comment,
            name=name,
            type=type,
            predicate=predicate,
            predefault=predefault,
            postdefault=postdefault,
            invalid=invalid,
            printer=printer,
            params=params,
            param_checks=param_checks,
            result_checks=result_checks,
        )

    def ftype(self):
        "Return the name of the function typedef to use."
        return f"gdbarch_{self.name}_ftype"

    def param_list(self):
        "Return the formal parameter list as a string."
        return join_params(self.params)

    def set_list(self):
        """Return the formal parameter list of the caller function,
        as a string.  This list includes the gdbarch."""
        arch_arg = ("struct gdbarch *", "gdbarch")
        arch_tuple = [arch_arg]
        return join_params(arch_tuple + list(self.params))

    def actuals(self):
        "Return the actual parameters to forward, as a string."
        return ", ".join([p[1] for p in self.params])


class Method(Function):
    "A Method is like a Function but passes the gdbarch through."

    def param_list(self):
        "See superclass."
        return self.set_list()

    def actuals(self):
        "See superclass."
        result = ["gdbarch"] + [p[1] for p in self.params]
        return ", ".join(result)


# Read the components.
with open("gdbarch-components.py") as fd:
    exec(fd.read())

copyright = gdbcopyright.copyright(
    "gdbarch.py", "Dynamic architecture support for GDB, the GNU debugger."
)


def info(c):
    "Filter function to only allow Info components."
    return type(c) is Info


def not_info(c):
    "Filter function to omit Info components."
    return type(c) is not Info


with open("gdbarch-gen.h", "w") as f:
    print(copyright, file=f)
    print(file=f)
    print(file=f)
    print("/* The following are pre-initialized by GDBARCH.  */", file=f)

    # Do Info components first.
    for c in filter(info, components):
        print(file=f)
        print(
            f"""extern {c.type} gdbarch_{c.name} (struct gdbarch *gdbarch);
/* set_gdbarch_{c.name}() - not applicable - pre-initialized.  */""",
            file=f,
        )

    print(file=f)
    print(file=f)
    print("/* The following are initialized by the target dependent code.  */", file=f)

    # Generate decls for accessors, setters, and predicates for all
    # non-Info components.
    for c in filter(not_info, components):
        if c.comment:
            print(file=f)
            comment = c.comment.split("\n")
            if comment[0] == "":
                comment = comment[1:]
            if comment[-1] == "":
                comment = comment[:-1]
            print("/* ", file=f, end="")
            print(comment[0], file=f, end="")
            if len(comment) > 1:
                print(file=f)
                print(
                    textwrap.indent("\n".join(comment[1:]), prefix="   "),
                    end="",
                    file=f,
                )
            print(" */", file=f)

        if c.predicate:
            print(file=f)
            print(f"extern bool gdbarch_{c.name}_p (struct gdbarch *gdbarch);", file=f)

        print(file=f)
        if isinstance(c, Value):
            print(
                f"extern {c.type} gdbarch_{c.name} (struct gdbarch *gdbarch);",
                file=f,
            )
            print(
                f"extern void set_gdbarch_{c.name} (struct gdbarch *gdbarch, {c.type} {c.name});",
                file=f,
            )
        else:
            assert isinstance(c, Function)
            print(
                f"typedef {c.type} ({c.ftype()}) ({c.param_list()});",
                file=f,
            )
            print(
                f"extern {c.type} gdbarch_{c.name} ({c.set_list()});",
                file=f,
            )
            print(
                f"extern void set_gdbarch_{c.name} (struct gdbarch *gdbarch, {c.ftype()} *{c.name});",
                file=f,
            )

with open("gdbarch.c", "w") as f:
    print(copyright, file=f)
    print(file=f)
    print("/* Maintain the struct gdbarch object.  */", file=f)
    print(file=f)
    #
    # The struct definition body.
    #
    print("struct gdbarch", file=f)
    print("{", file=f)
    print("  /* Has this architecture been fully initialized?  */", file=f)
    print("  bool initialized_p = false;", file=f)
    print(file=f)
    print("  /* An obstack bound to the lifetime of the architecture.  */", file=f)
    print("  auto_obstack obstack;", file=f)
    print("  /* Registry.  */", file=f)
    print("  registry<gdbarch> registry_fields;", file=f)
    print(file=f)
    print("  /* basic architectural information.  */", file=f)
    for c in filter(info, components):
        print(f"  {c.type} {c.name};", file=f)
    print(file=f)
    print("  /* target specific vector.  */", file=f)
    print("  struct gdbarch_tdep_base *tdep = nullptr;", file=f)
    print("  gdbarch_dump_tdep_ftype *dump_tdep = nullptr;", file=f)
    print(file=f)
    print("  /* per-architecture data-pointers.  */", file=f)
    print("  unsigned nr_data = 0;", file=f)
    print("  void **data = nullptr;", file=f)
    print(file=f)
    for c in filter(not_info, components):
        if isinstance(c, Function):
            print(f"  gdbarch_{c.name}_ftype *", file=f, end="")
        else:
            print(f"  {c.type} ", file=f, end="")
        print(f"{c.name} = ", file=f, end="")
        if c.predefault is not None:
            print(f"{c.predefault};", file=f)
        elif isinstance(c, Value):
            print("0;", file=f)
        else:
            assert isinstance(c, Function)
            print("nullptr;", file=f)
    print("};", file=f)
    print(file=f)
    #
    # Initialization.
    #
    print("/* Create a new ``struct gdbarch'' based on information provided by", file=f)
    print("   ``struct gdbarch_info''.  */", file=f)
    print(file=f)
    print("struct gdbarch *", file=f)
    print("gdbarch_alloc (const struct gdbarch_info *info,", file=f)
    print("	       struct gdbarch_tdep_base *tdep)", file=f)
    print("{", file=f)
    print("  struct gdbarch *gdbarch;", file=f)
    print("", file=f)
    print("  gdbarch = new struct gdbarch;", file=f)
    print(file=f)
    print("  gdbarch->tdep = tdep;", file=f)
    print(file=f)
    for c in filter(info, components):
        print(f"  gdbarch->{c.name} = info->{c.name};", file=f)
    print(file=f)
    print("  return gdbarch;", file=f)
    print("}", file=f)
    print(file=f)
    print(file=f)
    print(file=f)
    #
    # Post-initialization validation and updating
    #
    print("/* Ensure that all values in a GDBARCH are reasonable.  */", file=f)
    print(file=f)
    print("static void", file=f)
    print("verify_gdbarch (struct gdbarch *gdbarch)", file=f)
    print("{", file=f)
    print("  string_file log;", file=f)
    print(file=f)
    print("  /* fundamental */", file=f)
    print("  if (gdbarch->byte_order == BFD_ENDIAN_UNKNOWN)", file=f)
    print("""    log.puts ("\\n\\tbyte-order");""", file=f)
    print("  if (gdbarch->bfd_arch_info == NULL)", file=f)
    print("""    log.puts ("\\n\\tbfd_arch_info");""", file=f)
    print(
        "  /* Check those that need to be defined for the given multi-arch level.  */",
        file=f,
    )
    for c in filter(not_info, components):
        if c.invalid is False:
            print(f"  /* Skip verify of {c.name}, invalid_p == 0 */", file=f)
        elif c.predicate:
            print(f"  /* Skip verify of {c.name}, has predicate.  */", file=f)
        elif isinstance(c.invalid, str) and c.postdefault is not None:
            print(f"  if ({c.invalid})", file=f)
            print(f"    gdbarch->{c.name} = {c.postdefault};", file=f)
        elif c.predefault is not None and c.postdefault is not None:
            print(f"  if (gdbarch->{c.name} == {c.predefault})", file=f)
            print(f"    gdbarch->{c.name} = {c.postdefault};", file=f)
        elif c.postdefault is not None:
            print(f"  if (gdbarch->{c.name} == 0)", file=f)
            print(f"    gdbarch->{c.name} = {c.postdefault};", file=f)
        elif isinstance(c.invalid, str):
            print(f"  if ({c.invalid})", file=f)
            print(f"""    log.puts ("\\n\\t{c.name}");""", file=f)
        elif c.predefault is not None:
            print(f"  if (gdbarch->{c.name} == {c.predefault})", file=f)
            print(f"""    log.puts ("\\n\\t{c.name}");""", file=f)
        elif c.invalid is True:
            print(f"  if (gdbarch->{c.name} == 0)", file=f)
            print(f"""    log.puts ("\\n\\t{c.name}");""", file=f)
        else:
            # We should not allow ourselves to simply do nothing here
            # because no other case applies.  If we end up here then
            # either the input data needs adjusting so one of the
            # above cases matches, or we need additional cases adding
            # here.
            raise Exception("unhandled case when generating gdbarch validation")
    print("  if (!log.empty ())", file=f)
    print(
        """    internal_error (_("verify_gdbarch: the following are invalid ...%s"),""",
        file=f,
    )
    print("		    log.c_str ());", file=f)
    print("}", file=f)
    print(file=f)
    print(file=f)
    #
    # Dumping.
    #
    print("/* Print out the details of the current architecture.  */", file=f)
    print(file=f)
    print("void", file=f)
    print("gdbarch_dump (struct gdbarch *gdbarch, struct ui_file *file)", file=f)
    print("{", file=f)
    print("""  const char *gdb_nm_file = "<not-defined>";""", file=f)
    print(file=f)
    print("#if defined (GDB_NM_FILE)", file=f)
    print("  gdb_nm_file = GDB_NM_FILE;", file=f)
    print("#endif", file=f)
    print("  gdb_printf (file,", file=f)
    print("""	      "gdbarch_dump: GDB_NM_FILE = %s\\n",""", file=f)
    print("	      gdb_nm_file);", file=f)
    for c in components:
        if c.predicate:
            print("  gdb_printf (file,", file=f)
            print(
                f"""	      "gdbarch_dump: gdbarch_{c.name}_p() = %d\\n",""",
                file=f,
            )
            print(f"	      gdbarch_{c.name}_p (gdbarch));", file=f)
        if isinstance(c, Function):
            print("  gdb_printf (file,", file=f)
            print(f"""	      "gdbarch_dump: {c.name} = <%s>\\n",""", file=f)
            print(
                f"	      host_address_to_string (gdbarch->{c.name}));",
                file=f,
            )
        else:
            if c.printer:
                printer = c.printer
            elif c.type == "CORE_ADDR":
                printer = f"core_addr_to_string_nz (gdbarch->{c.name})"
            else:
                printer = f"plongest (gdbarch->{c.name})"
            print("  gdb_printf (file,", file=f)
            print(f"""	      "gdbarch_dump: {c.name} = %s\\n",""", file=f)
            print(f"	      {printer});", file=f)
    print("  if (gdbarch->dump_tdep != NULL)", file=f)
    print("    gdbarch->dump_tdep (gdbarch, file);", file=f)
    print("}", file=f)
    print(file=f)
    #
    # Bodies of setter, accessor, and predicate functions.
    #
    for c in components:
        if c.predicate:
            print(file=f)
            print("bool", file=f)
            print(f"gdbarch_{c.name}_p (struct gdbarch *gdbarch)", file=f)
            print("{", file=f)
            print("  gdb_assert (gdbarch != NULL);", file=f)
            print(f"  return {c.get_predicate()};", file=f)
            print("}", file=f)
        if isinstance(c, Function):
            print(file=f)
            print(f"{c.type}", file=f)
            print(f"gdbarch_{c.name} ({c.set_list()})", file=f)
            print("{", file=f)
            print("  gdb_assert (gdbarch != NULL);", file=f)
            print(f"  gdb_assert (gdbarch->{c.name} != NULL);", file=f)
            if c.predicate and c.predefault:
                # Allow a call to a function with a predicate.
                print(
                    f"  /* Do not check predicate: {c.get_predicate()}, allow call.  */",
                    file=f,
                )
            if c.param_checks:
                for rule in c.param_checks:
                    print(f"  gdb_assert ({rule});", file=f)
            print("  if (gdbarch_debug >= 2)", file=f)
            print(
                f"""    gdb_printf (gdb_stdlog, "gdbarch_{c.name} called\\n");""",
                file=f,
            )
            print("  ", file=f, end="")
            if c.type != "void":
                if c.result_checks:
                    print("auto result = ", file=f, end="")
                else:
                    print("return ", file=f, end="")
            print(f"gdbarch->{c.name} ({c.actuals()});", file=f)
            if c.type != "void" and c.result_checks:
                for rule in c.result_checks:
                    print(f"  gdb_assert ({rule});", file=f)
                print("  return result;", file=f)
            print("}", file=f)
            print(file=f)
            print("void", file=f)
            setter_name = f"set_gdbarch_{c.name}"
            ftype_name = f"gdbarch_{c.name}_ftype"
            print(f"{setter_name} (struct gdbarch *gdbarch,", file=f)
            indent_columns = len(f"{setter_name} (")
            print(f"{indentation(indent_columns)}{ftype_name} {c.name})", file=f)
            print("{", file=f)
            print(f"  gdbarch->{c.name} = {c.name};", file=f)
            print("}", file=f)
        elif isinstance(c, Value):
            print(file=f)
            print(f"{c.type}", file=f)
            print(f"gdbarch_{c.name} (struct gdbarch *gdbarch)", file=f)
            print("{", file=f)
            print("  gdb_assert (gdbarch != NULL);", file=f)
            if c.invalid is False:
                print(f"  /* Skip verify of {c.name}, invalid_p == 0 */", file=f)
            elif isinstance(c.invalid, str):
                print("  /* Check variable is valid.  */", file=f)
                print(f"  gdb_assert (!({c.invalid}));", file=f)
            elif c.predefault:
                print("  /* Check variable changed from pre-default.  */", file=f)
                print(f"  gdb_assert (gdbarch->{c.name} != {c.predefault});", file=f)
            print("  if (gdbarch_debug >= 2)", file=f)
            print(
                f"""    gdb_printf (gdb_stdlog, "gdbarch_{c.name} called\\n");""",
                file=f,
            )
            print(f"  return gdbarch->{c.name};", file=f)
            print("}", file=f)
            print(file=f)
            print("void", file=f)
            setter_name = f"set_gdbarch_{c.name}"
            print(f"{setter_name} (struct gdbarch *gdbarch,", file=f)
            indent_columns = len(f"{setter_name} (")
            print(f"{indentation(indent_columns)}{c.type} {c.name})", file=f)
            print("{", file=f)
            print(f"  gdbarch->{c.name} = {c.name};", file=f)
            print("}", file=f)
        else:
            assert isinstance(c, Info)
            print(file=f)
            print(f"{c.type}", file=f)
            print(f"gdbarch_{c.name} (struct gdbarch *gdbarch)", file=f)
            print("{", file=f)
            print("  gdb_assert (gdbarch != NULL);", file=f)
            print("  if (gdbarch_debug >= 2)", file=f)
            print(
                f"""    gdb_printf (gdb_stdlog, "gdbarch_{c.name} called\\n");""",
                file=f,
            )
            print(f"  return gdbarch->{c.name};", file=f)
            print("}", file=f)
