#!/usr/bin/env python3

# Copyright (C) 2013-2023 Free Software Foundation, Inc.
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

# Usage:
#    make-target-delegates.py

import re
import gdbcopyright


# The line we search for in target.h that marks where we should start
# looking for methods.
TRIGGER = re.compile(r"^struct target_ops$")
# The end of the methods part.
ENDER = re.compile(r"^\s*};$")

# Match a C symbol.
SYMBOL = "[a-zA-Z_][a-zA-Z0-9_]*"
# Match the name part of a method in struct target_ops.
NAME_PART = r"(?P<name>" + SYMBOL + ")\s"
# Match the arguments to a method.
ARGS_PART = r"(?P<args>\(.*\))"
# We strip the indentation so here we only need the caret.
INTRO_PART = r"^"

POINTER_PART = r"\s*(\*)?\s*"

# Match a C++ symbol, including scope operators and template
# parameters.  E.g., 'std::vector<something>'.
CP_SYMBOL = r"[a-zA-Z_][a-zA-Z0-9_<>:]*"
# Match the return type when it is "ordinary".
SIMPLE_RETURN_PART = r"((struct|class|enum|union)\s+)?" + CP_SYMBOL

# Match a return type.
RETURN_PART = r"((const|volatile)\s+)?(" + SIMPLE_RETURN_PART + ")" + POINTER_PART

# Match "virtual".
VIRTUAL_PART = r"virtual\s"

# Match the TARGET_DEFAULT_* attribute for a method.
TARGET_DEFAULT_PART = r"TARGET_DEFAULT_(?P<style>[A-Z_]+)\s*\((?P<default_arg>.*)\)"

# Match the arguments and trailing attribute of a method definition.
# Note we don't match the trailing ";".
METHOD_TRAILER = r"\s*" + TARGET_DEFAULT_PART + "$"

# Match an entire method definition.
METHOD = re.compile(
    INTRO_PART
    + VIRTUAL_PART
    + "(?P<return_type>"
    + RETURN_PART
    + ")"
    + NAME_PART
    + ARGS_PART
    + METHOD_TRAILER
)

# Regular expression used to dissect argument types.
ARGTYPES = re.compile(
    "^("
    + r"(?P<E>enum\s+"
    + SYMBOL
    + r"\s*)("
    + SYMBOL
    + ")?"
    + r"|(?P<T>.*(enum\s+)?"
    + SYMBOL
    + r".*(\s|\*|&))"
    + SYMBOL
    + ")$"
)

# Match TARGET_DEBUG_PRINTER in an argument type.
# This must match the whole "sub-expression" including the parens.
TARGET_DEBUG_PRINTER = r"\s*TARGET_DEBUG_PRINTER\s*\((?P<arg>[^)]*)\)\s*"


def scan_target_h():
    found_trigger = False
    all_the_text = ""
    with open("target.h", "r") as target_h:
        for line in target_h:
            line = line.strip()
            if not found_trigger:
                if TRIGGER.match(line):
                    found_trigger = True
            elif "{" in line:
                # Skip the open brace.
                pass
            elif ENDER.match(line):
                break
            else:
                # Strip // comments.
                line = re.split("//", line)[0]
                all_the_text = all_the_text + " " + line
    if not found_trigger:
        raise "Could not find trigger line"
    # Now strip out the C comments.
    all_the_text = re.sub(r"/\*(.*?)\*/", "", all_the_text)
    # Replace sequences whitespace with a single space character.
    # We need the space because the method may have been split
    # between multiple lines, like e.g.:
    #
    #  virtual std::vector<long_type_name>
    #    my_long_method_name ()
    #    TARGET_DEFAULT_IGNORE ();
    #
    # If we didn't preserve the space, then we'd end up with:
    #
    #  virtual std::vector<long_type_name>my_long_method_name ()TARGET_DEFAULT_IGNORE ()
    #
    # ... which wouldn't later be parsed correctly.
    all_the_text = re.sub(r"\s+", " ", all_the_text)
    return all_the_text.split(";")


# Parse arguments into a list.
def parse_argtypes(typestr):
    # Remove the outer parens.
    typestr = re.sub(r"^\((.*)\)$", r"\1", typestr)
    result = []
    for item in re.split(r",\s*", typestr):
        if item == "void" or item == "":
            continue
        m = ARGTYPES.match(item)
        if m:
            if m.group("E"):
                onetype = m.group("E")
            else:
                onetype = m.group("T")
        else:
            onetype = item
        result.append(onetype.strip())
    return result


# Write function header given name, return type, and argtypes.
# Returns a list of actual argument names.
def write_function_header(f, decl, name, return_type, argtypes):
    print(return_type, file=f, end="")
    if decl:
        if not return_type.endswith("*"):
            print(" ", file=f, end="")
    else:
        print("", file=f)
    print(name + " (", file=f, end="")
    argdecls = []
    actuals = []
    for i in range(len(argtypes)):
        val = re.sub(TARGET_DEBUG_PRINTER, "", argtypes[i])
        if not val.endswith("*") and not val.endswith("&"):
            val = val + " "
        vname = "arg" + str(i)
        val = val + vname
        argdecls.append(val)
        actuals.append(vname)
    print(", ".join(argdecls) + ")", file=f, end="")
    if decl:
        print(" override;", file=f)
    else:
        print("\n{", file=f)
    return actuals


# Write out a declaration.
def write_declaration(f, name, return_type, argtypes):
    write_function_header(f, True, name, return_type, argtypes)


# Write out a delegation function.
def write_delegator(f, name, return_type, argtypes):
    names = write_function_header(
        f, False, "target_ops::" + name, return_type, argtypes
    )
    print("  ", file=f, end="")
    if return_type != "void":
        print("return ", file=f, end="")
    print("this->beneath ()->" + name + " (", file=f, end="")
    print(", ".join(names), file=f, end="")
    print(");", file=f)
    print("}\n", file=f)


# Write out a default function.
def write_tdefault(f, content, style, name, return_type, argtypes):
    name = "dummy_target::" + name
    names = write_function_header(f, False, name, return_type, argtypes)
    if style == "FUNC":
        print("  ", file=f, end="")
        if return_type != "void":
            print("return ", file=f, end="")
        print(content + " (", file=f, end="")
        names.insert(0, "this")
        print(", ".join(names) + ");", file=f)
    elif style == "RETURN":
        print("  return " + content + ";", file=f)
    elif style == "NORETURN":
        print("  " + content + ";", file=f)
    elif style == "IGNORE":
        # Nothing.
        pass
    else:
        raise "unrecognized style: " + style
    print("}\n", file=f)


def munge_type(typename):
    m = re.search(TARGET_DEBUG_PRINTER, typename)
    if m:
        return m.group("arg")
    typename = typename.rstrip()
    typename = re.sub("[ ()<>:]", "_", typename)
    typename = re.sub("[*]", "p", typename)
    typename = re.sub("&", "r", typename)
    # Identifers with double underscores are reserved to the C++
    # implementation.
    typename = re.sub("_+", "_", typename)
    # Avoid ending the function name with underscore, for
    # cosmetics.  Trailing underscores appear after munging types
    # with template parameters, like e.g. "foo<int>".
    typename = re.sub("_+$", "", typename)
    return "target_debug_print_" + typename


# Write out a debug method.
def write_debugmethod(f, content, name, return_type, argtypes):
    debugname = "debug_target::" + name
    names = write_function_header(f, False, debugname, return_type, argtypes)
    if return_type != "void":
        print("  " + return_type + " result;", file=f)
    print(
        '  gdb_printf (gdb_stdlog, "-> %s->'
        + name
        + ' (...)\\n", this->beneath ()->shortname ());',
        file=f,
    )

    # Delegate to the beneath target.
    print("  ", file=f, end="")
    if return_type != "void":
        print("result = ", file=f, end="")
    print("this->beneath ()->" + name + " (", file=f, end="")
    print(", ".join(names), file=f, end="")
    print(");", file=f)

    # Now print the arguments.
    print(
        '  gdb_printf (gdb_stdlog, "<- %s->'
        + name
        + ' (", this->beneath ()->shortname ());',
        file=f,
    )
    for i in range(len(argtypes)):
        if i > 0:
            print('  gdb_puts (", ", gdb_stdlog);', file=f)
        printer = munge_type(argtypes[i])
        print("  " + printer + " (" + names[i] + ");", file=f)
    if return_type != "void":
        print('  gdb_puts (") = ", gdb_stdlog);', file=f)
        printer = munge_type(return_type)
        print("  " + printer + " (result);", file=f)
        print('  gdb_puts ("\\n", gdb_stdlog);', file=f)
    else:
        print('  gdb_puts (")\\n", gdb_stdlog);', file=f)

    if return_type != "void":
        print("  return result;", file=f)

    print("}\n", file=f)


def print_class(f, class_name, delegators, entries):
    print("struct " + class_name + " : public target_ops", file=f)
    print("{", file=f)
    print("  const target_info &info () const override;", file=f)
    print("", file=f)
    print("  strata stratum () const override;", file=f)
    print("", file=f)

    for name in delegators:
        return_type = entries[name]["return_type"]
        argtypes = entries[name]["argtypes"]
        print("  ", file=f, end="")
        write_declaration(f, name, return_type, argtypes)

    print("};\n", file=f)


delegators = []
entries = {}
for current_line in scan_target_h():
    # See comments in scan_target_h.  Here we strip away the leading
    # and trailing whitespace.
    current_line = current_line.strip()
    m = METHOD.match(current_line)
    if not m:
        continue
    data = m.groupdict()
    data["argtypes"] = parse_argtypes(data["args"])
    data["return_type"] = data["return_type"].strip()
    entries[data["name"]] = data

    delegators.append(data["name"])

with open("target-delegates.c", "w") as f:
    print(
        gdbcopyright.copyright(
            "make-target-delegates.py", "Boilerplate target methods for GDB"
        ),
        file=f,
    )
    print_class(f, "dummy_target", delegators, entries)
    print_class(f, "debug_target", delegators, entries)

    for name in delegators:
        tdefault = entries[name]["default_arg"]
        return_type = entries[name]["return_type"]
        style = entries[name]["style"]
        argtypes = entries[name]["argtypes"]

        write_delegator(f, name, return_type, argtypes)
        write_tdefault(f, tdefault, style, name, return_type, argtypes)
        write_debugmethod(f, tdefault, name, return_type, argtypes)
