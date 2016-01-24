# Python hooks for gdb for debugging GCC
# Copyright (C) 2013-2015 Free Software Foundation, Inc.

# Contributed by David Malcolm <dmalcolm@redhat.com>

# This file is part of GCC.

# GCC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3, or (at your option) any later
# version.

# GCC is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.

"""
Enabling the debugging hooks
----------------------------
gcc/configure (from configure.ac) generates a .gdbinit within the "gcc"
subdirectory of the build directory, and when run by gdb, this imports
gcc/gdbhooks.py from the source directory, injecting useful Python code
into gdb.

You may see a message from gdb of the form:
  "path-to-build/gcc/.gdbinit" auto-loading has been declined by your `auto-load safe-path'
as a protection against untrustworthy python scripts.  See
  http://sourceware.org/gdb/onlinedocs/gdb/Auto_002dloading-safe-path.html

The fix is to mark the paths of the build/gcc directory as trustworthy.
An easy way to do so is by adding the following to your ~/.gdbinit script:
  add-auto-load-safe-path /absolute/path/to/build/gcc
for the build directories for your various checkouts of gcc.

If it's working, you should see the message:
  Successfully loaded GDB hooks for GCC
as gdb starts up.

During development, I've been manually invoking the code in this way, as a
precanned way of printing a variety of different kinds of value:

  gdb \
    -ex "break expand_gimple_stmt" \
    -ex "run" \
    -ex "bt" \
    --args \
      ./cc1 foo.c -O3

Examples of output using the pretty-printers
--------------------------------------------
Pointer values are generally shown in the form:
  <type address extra_info>

For example, an opt_pass* might appear as:
  (gdb) p pass
  $2 = <opt_pass* 0x188b600 "expand"(170)>

The name of the pass is given ("expand"), together with the
static_pass_number.

Note that you can dereference the pointer in the normal way:
  (gdb) p *pass
  $4 = {type = RTL_PASS, name = 0x120a312 "expand",
  [etc, ...snipped...]

and you can suppress pretty-printers using /r (for "raw"):
  (gdb) p /r pass
  $3 = (opt_pass *) 0x188b600

Basic blocks are shown with their index in parentheses, apart from the
CFG's entry and exit blocks, which are given as "ENTRY" and "EXIT":
  (gdb) p bb
  $9 = <basic_block 0x7ffff041f1a0 (2)>
  (gdb) p cfun->cfg->x_entry_block_ptr
  $10 = <basic_block 0x7ffff041f0d0 (ENTRY)>
  (gdb) p cfun->cfg->x_exit_block_ptr
  $11 = <basic_block 0x7ffff041f138 (EXIT)>

CFG edges are shown with the src and dest blocks given in parentheses:
  (gdb) p e
  $1 = <edge 0x7ffff043f118 (ENTRY -> 6)>

Tree nodes are printed using Python code that emulates print_node_brief,
running in gdb, rather than in the inferior:
  (gdb) p cfun->decl
  $1 = <function_decl 0x7ffff0420b00 foo>
For usability, the type is printed first (e.g. "function_decl"), rather
than just "tree".

RTL expressions use a kludge: they are pretty-printed by injecting
calls into print-rtl.c into the inferior:
  Value returned is $1 = (note 9 8 10 [bb 3] NOTE_INSN_BASIC_BLOCK)
  (gdb) p $1
  $2 = (note 9 8 10 [bb 3] NOTE_INSN_BASIC_BLOCK)
  (gdb) p /r $1
  $3 = (rtx_def *) 0x7ffff043e140
This won't work for coredumps, and probably in other circumstances, but
it's a quick way of getting lots of debuggability quickly.

Callgraph nodes are printed with the name of the function decl, if
available:
  (gdb) frame 5
  #5  0x00000000006c288a in expand_function (node=<cgraph_node* 0x7ffff0312720 "foo">) at ../../src/gcc/cgraphunit.c:1594
  1594	  execute_pass_list (g->get_passes ()->all_passes);
  (gdb) p node
  $1 = <cgraph_node* 0x7ffff0312720 "foo">

vec<> pointers are printed as the address followed by the elements in
braces.  Here's a length 2 vec:
  (gdb) p bb->preds
  $18 = 0x7ffff0428b68 = {<edge 0x7ffff044d380 (3 -> 5)>, <edge 0x7ffff044d3b8 (4 -> 5)>}

and here's a length 1 vec:
  (gdb) p bb->succs
  $19 = 0x7ffff0428bb8 = {<edge 0x7ffff044d3f0 (5 -> EXIT)>}

You cannot yet use array notation [] to access the elements within the
vector: attempting to do so instead gives you the vec itself (for vec[0]),
or a (probably) invalid cast to vec<> for the memory after the vec (for
vec[1] onwards).

Instead (for now) you must access m_vecdata:
  (gdb) p bb->preds->m_vecdata[0]
  $20 = <edge 0x7ffff044d380 (3 -> 5)>
  (gdb) p bb->preds->m_vecdata[1]
  $21 = <edge 0x7ffff044d3b8 (4 -> 5)>
"""
import os.path
import re

import gdb
import gdb.printing
import gdb.types

# Convert "enum tree_code" (tree.def and tree.h) to a dict:
tree_code_dict = gdb.types.make_enum_dict(gdb.lookup_type('enum tree_code'))

# ...and look up specific values for use later:
IDENTIFIER_NODE = tree_code_dict['IDENTIFIER_NODE']
TYPE_DECL = tree_code_dict['TYPE_DECL']

# Similarly for "enum tree_code_class" (tree.h):
tree_code_class_dict = gdb.types.make_enum_dict(gdb.lookup_type('enum tree_code_class'))
tcc_type = tree_code_class_dict['tcc_type']
tcc_declaration = tree_code_class_dict['tcc_declaration']

class Tree:
    """
    Wrapper around a gdb.Value for a tree, with various methods
    corresponding to macros in gcc/tree.h
    """
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def is_nonnull(self):
        return long(self.gdbval)

    def TREE_CODE(self):
        """
        Get gdb.Value corresponding to TREE_CODE (self)
        as per:
          #define TREE_CODE(NODE) ((enum tree_code) (NODE)->base.code)
        """
        return self.gdbval['base']['code']

    def DECL_NAME(self):
        """
        Get Tree instance corresponding to DECL_NAME (self)
        """
        return Tree(self.gdbval['decl_minimal']['name'])

    def TYPE_NAME(self):
        """
        Get Tree instance corresponding to result of TYPE_NAME (self)
        """
        return Tree(self.gdbval['type_common']['name'])

    def IDENTIFIER_POINTER(self):
        """
        Get str correspoinding to result of IDENTIFIER_NODE (self)
        """
        return self.gdbval['identifier']['id']['str'].string()

class TreePrinter:
    "Prints a tree"

    def __init__ (self, gdbval):
        self.gdbval = gdbval
        self.node = Tree(gdbval)

    def to_string (self):
        # like gcc/print-tree.c:print_node_brief
        # #define TREE_CODE(NODE) ((enum tree_code) (NODE)->base.code)
        # tree_code_name[(int) TREE_CODE (node)])
        if long(self.gdbval) == 0:
            return '<tree 0x0>'

        val_TREE_CODE = self.node.TREE_CODE()

        # extern const enum tree_code_class tree_code_type[];
        # #define TREE_CODE_CLASS(CODE)	tree_code_type[(int) (CODE)]

        val_tree_code_type = gdb.parse_and_eval('tree_code_type')
        val_tclass = val_tree_code_type[val_TREE_CODE]

        val_tree_code_name = gdb.parse_and_eval('tree_code_name')
        val_code_name = val_tree_code_name[long(val_TREE_CODE)]
        #print val_code_name.string()

        result = '<%s 0x%x' % (val_code_name.string(), long(self.gdbval))
        if long(val_tclass) == tcc_declaration:
            tree_DECL_NAME = self.node.DECL_NAME()
            if tree_DECL_NAME.is_nonnull():
                 result += ' %s' % tree_DECL_NAME.IDENTIFIER_POINTER()
            else:
                pass # TODO: labels etc
        elif long(val_tclass) == tcc_type:
            tree_TYPE_NAME = Tree(self.gdbval['type_common']['name'])
            if tree_TYPE_NAME.is_nonnull():
                if tree_TYPE_NAME.TREE_CODE() == IDENTIFIER_NODE:
                    result += ' %s' % tree_TYPE_NAME.IDENTIFIER_POINTER()
                elif tree_TYPE_NAME.TREE_CODE() == TYPE_DECL:
                    if tree_TYPE_NAME.DECL_NAME().is_nonnull():
                        result += ' %s' % tree_TYPE_NAME.DECL_NAME().IDENTIFIER_POINTER()
        if self.node.TREE_CODE() == IDENTIFIER_NODE:
            result += ' %s' % self.node.IDENTIFIER_POINTER()
        # etc
        result += '>'
        return result

######################################################################
# Callgraph pretty-printers
######################################################################

class CGraphNodePrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        result = '<cgraph_node* 0x%x' % long(self.gdbval)
        if long(self.gdbval):
            # symtab_node::name calls lang_hooks.decl_printable_name
            # default implementation (lhd_decl_printable_name) is:
            #    return IDENTIFIER_POINTER (DECL_NAME (decl));
            tree_decl = Tree(self.gdbval['decl'])
            result += ' "%s"' % tree_decl.DECL_NAME().IDENTIFIER_POINTER()
        result += '>'
        return result

######################################################################
# Dwarf DIE pretty-printers
######################################################################

class DWDieRefPrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        if long(self.gdbval) == 0:
            return '<dw_die_ref 0x0>'
        result = '<dw_die_ref 0x%x' % long(self.gdbval)
        result += ' %s' % self.gdbval['die_tag']
        if long(self.gdbval['die_parent']) != 0:
            result += ' <parent=0x%x %s>' % (long(self.gdbval['die_parent']),
                                             self.gdbval['die_parent']['die_tag'])
                                             
        result += '>'
        return result

######################################################################

class GimplePrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        if long(self.gdbval) == 0:
            return '<gimple 0x0>'
        val_gimple_code = self.gdbval['code']
        val_gimple_code_name = gdb.parse_and_eval('gimple_code_name')
        val_code_name = val_gimple_code_name[long(val_gimple_code)]
        result = '<%s 0x%x' % (val_code_name.string(),
                               long(self.gdbval))
        result += '>'
        return result

######################################################################
# CFG pretty-printers
######################################################################

def bb_index_to_str(index):
    if index == 0:
        return 'ENTRY'
    elif index == 1:
        return 'EXIT'
    else:
        return '%i' % index

class BasicBlockPrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        result = '<basic_block 0x%x' % long(self.gdbval)
        if long(self.gdbval):
            result += ' (%s)' % bb_index_to_str(long(self.gdbval['index']))
        result += '>'
        return result

class CfgEdgePrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        result = '<edge 0x%x' % long(self.gdbval)
        if long(self.gdbval):
            src = bb_index_to_str(long(self.gdbval['src']['index']))
            dest = bb_index_to_str(long(self.gdbval['dest']['index']))
            result += ' (%s -> %s)' % (src, dest)
        result += '>'
        return result

######################################################################

class Rtx:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def GET_CODE(self):
        return self.gdbval['code']

def GET_RTX_LENGTH(code):
    val_rtx_length = gdb.parse_and_eval('rtx_length')
    return long(val_rtx_length[code])

def GET_RTX_NAME(code):
    val_rtx_name = gdb.parse_and_eval('rtx_name')
    return val_rtx_name[code].string()

def GET_RTX_FORMAT(code):
    val_rtx_format = gdb.parse_and_eval('rtx_format')
    return val_rtx_format[code].string()

class RtxPrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval
        self.rtx = Rtx(gdbval)

    def to_string (self):
        """
        For now, a cheap kludge: invoke the inferior's print
        function to get a string to use the user, and return an empty
        string for gdb
        """
        # We use print_inline_rtx to avoid a trailing newline
        gdb.execute('call print_inline_rtx (stderr, (const_rtx) %s, 0)'
                    % long(self.gdbval))
        return ''

        # or by hand; based on gcc/print-rtl.c:print_rtx
        result = ('<rtx_def 0x%x'
                  % (long(self.gdbval)))
        code = self.rtx.GET_CODE()
        result += ' (%s' % GET_RTX_NAME(code)
        format_ = GET_RTX_FORMAT(code)
        for i in range(GET_RTX_LENGTH(code)):
            print format_[i]
        result += ')>'
        return result

######################################################################

class PassPrinter:
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        result = '<opt_pass* 0x%x' % long(self.gdbval)
        if long(self.gdbval):
            result += (' "%s"(%i)'
                       % (self.gdbval['name'].string(),
                          long(self.gdbval['static_pass_number'])))
        result += '>'
        return result

######################################################################

class VecPrinter:
    #    -ex "up" -ex "p bb->preds"
    def __init__(self, gdbval):
        self.gdbval = gdbval

    def display_hint (self):
        return 'array'

    def to_string (self):
        # A trivial implementation; prettyprinting the contents is done
        # by gdb calling the "children" method below.
        return '0x%x' % long(self.gdbval)

    def children (self):
        if long(self.gdbval) == 0:
            return
        m_vecpfx = self.gdbval['m_vecpfx']
        m_num = m_vecpfx['m_num']
        m_vecdata = self.gdbval['m_vecdata']
        for i in range(m_num):
            yield ('[%d]' % i, m_vecdata[i])

######################################################################

# TODO:
#   * hashtab
#   * location_t

class GdbSubprinter(gdb.printing.SubPrettyPrinter):
    def __init__(self, name, class_):
        super(GdbSubprinter, self).__init__(name)
        self.class_ = class_

    def handles_type(self, str_type):
        raise NotImplementedError

class GdbSubprinterTypeList(GdbSubprinter):
    """
    A GdbSubprinter that handles a specific set of types
    """
    def __init__(self, str_types, name, class_):
        super(GdbSubprinterTypeList, self).__init__(name, class_)
        self.str_types = frozenset(str_types)

    def handles_type(self, str_type):
        return str_type in self.str_types

class GdbSubprinterRegex(GdbSubprinter):
    """
    A GdbSubprinter that handles types that match a regex
    """
    def __init__(self, regex, name, class_):
        super(GdbSubprinterRegex, self).__init__(name, class_)
        self.regex = re.compile(regex)

    def handles_type(self, str_type):
        return self.regex.match(str_type)

class GdbPrettyPrinters(gdb.printing.PrettyPrinter):
    def __init__(self, name):
        super(GdbPrettyPrinters, self).__init__(name, [])

    def add_printer_for_types(self, name, class_, types):
        self.subprinters.append(GdbSubprinterTypeList(name, class_, types))

    def add_printer_for_regex(self, name, class_, regex):
        self.subprinters.append(GdbSubprinterRegex(name, class_, regex))

    def __call__(self, gdbval):
        type_ = gdbval.type.unqualified()
        str_type = str(type_)
        for printer in self.subprinters:
            if printer.enabled and printer.handles_type(str_type):
                return printer.class_(gdbval)

        # Couldn't find a pretty printer (or it was disabled):
        return None


def build_pretty_printer():
    pp = GdbPrettyPrinters('gcc')
    pp.add_printer_for_types(['tree'],
                             'tree', TreePrinter)
    pp.add_printer_for_types(['cgraph_node *'],
                             'cgraph_node', CGraphNodePrinter)
    pp.add_printer_for_types(['dw_die_ref'],
                             'dw_die_ref', DWDieRefPrinter)
    pp.add_printer_for_types(['gimple', 'gimple_statement_base *',

                              # Keep this in the same order as gimple.def:
                              'gimple_cond', 'const_gimple_cond',
                              'gimple_statement_cond *',
                              'gimple_debug', 'const_gimple_debug',
                              'gimple_statement_debug *',
                              'gimple_label', 'const_gimple_label',
                              'gimple_statement_label *',
                              'gimple_switch', 'const_gimple_switch',
                              'gimple_statement_switch *',
                              'gimple_assign', 'const_gimple_assign',
                              'gimple_statement_assign *',
                              'gimple_bind', 'const_gimple_bind',
                              'gimple_statement_bind *',
                              'gimple_phi', 'const_gimple_phi',
                              'gimple_statement_phi *'],

                             'gimple',
                             GimplePrinter)
    pp.add_printer_for_types(['basic_block', 'basic_block_def *'],
                             'basic_block',
                             BasicBlockPrinter)
    pp.add_printer_for_types(['edge', 'edge_def *'],
                             'edge',
                             CfgEdgePrinter)
    pp.add_printer_for_types(['rtx_def *'], 'rtx_def', RtxPrinter)
    pp.add_printer_for_types(['opt_pass *'], 'opt_pass', PassPrinter)

    pp.add_printer_for_regex(r'vec<(\S+), (\S+), (\S+)> \*',
                             'vec',
                             VecPrinter)

    return pp

gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    build_pretty_printer())

def find_gcc_source_dir():
    # Use location of global "g" to locate the source tree
    sym_g = gdb.lookup_global_symbol('g')
    path = sym_g.symtab.filename # e.g. '../../src/gcc/context.h'
    srcdir = os.path.split(path)[0] # e.g. '../../src/gcc'
    return srcdir

class PassNames:
    """Parse passes.def, gathering a list of pass class names"""
    def __init__(self):
        srcdir = find_gcc_source_dir()
        self.names = []
        with open(os.path.join(srcdir, 'passes.def')) as f:
            for line in f:
                m = re.match('\s*NEXT_PASS \((.+)\);', line)
                if m:
                    self.names.append(m.group(1))

class BreakOnPass(gdb.Command):
    """
    A custom command for putting breakpoints on the execute hook of passes.
    This is largely a workaround for issues with tab-completion in gdb when
    setting breakpoints on methods on classes within anonymous namespaces.

    Example of use: putting a breakpoint on "final"
      (gdb) break-on-pass
    Press <TAB>; it autocompletes to "pass_":
      (gdb) break-on-pass pass_
    Press <TAB>:
      Display all 219 possibilities? (y or n)
    Press "n"; then type "f":
      (gdb) break-on-pass pass_f
    Press <TAB> to autocomplete to pass classnames beginning with "pass_f":
      pass_fast_rtl_dce              pass_fold_builtins
      pass_feedback_split_functions  pass_forwprop
      pass_final                     pass_fre
      pass_fixup_cfg                 pass_free_cfg
    Type "in<TAB>" to complete to "pass_final":
      (gdb) break-on-pass pass_final
    ...and hit <RETURN>:
      Breakpoint 6 at 0x8396ba: file ../../src/gcc/final.c, line 4526.
    ...and we have a breakpoint set; continue execution:
      (gdb) cont
      Continuing.
      Breakpoint 6, (anonymous namespace)::pass_final::execute (this=0x17fb990) at ../../src/gcc/final.c:4526
      4526	  virtual unsigned int execute (function *) { return rest_of_handle_final (); }
    """
    def __init__(self):
        gdb.Command.__init__(self, 'break-on-pass', gdb.COMMAND_BREAKPOINTS)
        self.pass_names = None

    def complete(self, text, word):
        # Lazily load pass names:
        if not self.pass_names:
            self.pass_names = PassNames()

        return [name
                for name in sorted(self.pass_names.names)
                if name.startswith(text)]

    def invoke(self, arg, from_tty):
        sym = '(anonymous namespace)::%s::execute' % arg
        breakpoint = gdb.Breakpoint(sym)

BreakOnPass()

print('Successfully loaded GDB hooks for GCC')
