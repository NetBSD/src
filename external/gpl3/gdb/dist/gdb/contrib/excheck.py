#   Copyright 2011, 2013 Free Software Foundation, Inc.
#
#   This is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see
#   <http://www.gnu.org/licenses/>.

# This is a GCC plugin that computes some exception-handling data for
# gdb.  This data can then be summarized and checked by the
# exsummary.py script.

# To use:
# * First, install the GCC Python plugin.  See
#   https://fedorahosted.org/gcc-python-plugin/
# * export PYTHON_PLUGIN=/full/path/to/plugin/directory
#   This should be the directory holding "python.so".
# * cd build/gdb; make mostlyclean
# * make CC=.../gcc-with-excheck
#   This will write a number of .py files in the build directory.
# * python .../exsummary.py
#   This will show the violations.

import gcc
import gccutils
import sys

# Where our output goes.
output_file = None

# Cleanup functions require special treatment, because they take a
# function argument, but in theory the function must be nothrow.
cleanup_functions = {
    'make_cleanup': 1,
    'make_cleanup_dtor': 1,
    'make_final_cleanup': 1,
    'make_my_cleanup2': 1,
    'make_my_cleanup': 1
}

# Functions which may throw but which we want to ignore.
ignore_functions = {
    # This one is super special.
    'exceptions_state_mc': 1,
    # gdb generally pretends that internal_error cannot throw, even
    # though it can.
    'internal_error': 1,
    # do_cleanups and friends are supposedly nothrow but we don't want
    # to run afoul of the indirect function call logic.
    'do_cleanups': 1,
    'do_final_cleanups': 1
}

# Functions which take a function argument, but which are not
# interesting, usually because the argument is not called in the
# current context.
non_passthrough_functions = {
    'signal': 1,
    'add_internal_function': 1
}

# Return True if the type is from Python.
def type_is_pythonic(t):
    if isinstance(t, gcc.ArrayType):
        t = t.type
    if not isinstance(t, gcc.RecordType):
        return False
    # Hack.
    return str(t).find('struct Py') == 0

# Examine all the fields of a struct.  We don't currently need any
# sort of recursion, so this is simple for now.
def examine_struct_fields(initializer):
    global output_file
    for idx2, value2 in initializer.elements:
        if isinstance(idx2, gcc.Declaration):
            if isinstance(value2, gcc.AddrExpr):
                value2 = value2.operand
                if isinstance(value2, gcc.FunctionDecl):
                    output_file.write("declare_nothrow(%s)\n"
                                      % repr(str(value2.name)))

# Examine all global variables looking for pointers to functions in
# structures whose types were defined by Python.
def examine_globals():
    global output_file
    vars = gcc.get_variables()
    for var in vars:
        if not isinstance(var.decl, gcc.VarDecl):
            continue
        output_file.write("################\n")
        output_file.write("# Analysis for %s\n" % var.decl.name)
        if not var.decl.initial:
            continue
        if not type_is_pythonic(var.decl.type):
            continue

        if isinstance(var.decl.type, gcc.ArrayType):
            for idx, value in var.decl.initial.elements:
                examine_struct_fields(value)
        else:
            gccutils.check_isinstance(var.decl.type, gcc.RecordType)
            examine_struct_fields(var.decl.initial)

# Called at the end of compilation to write out some data derived from
# globals and to close the output.
def close_output(*args):
    global output_file
    examine_globals()
    output_file.close()

# The pass which derives some exception-checking information.  We take
# a two-step approach: first we get a call graph from the compiler.
# This is emitted by the plugin as Python code.  Then, we run a second
# program that reads all the generated Python and uses it to get a
# global view of exception routes in gdb.
class GdbExceptionChecker(gcc.GimplePass):
    def __init__(self, output_file):
        gcc.GimplePass.__init__(self, 'gdb_exception_checker')
        self.output_file = output_file

    def log(self, obj):
        self.output_file.write("# %s\n" % str(obj))

    # Return true if FN is a call to a method on a Python object.
    # We know these cannot throw in the gdb sense.
    def fn_is_python_ignorable(self, fn):
        if not isinstance(fn, gcc.SsaName):
            return False
        stmt = fn.def_stmt
        if not isinstance(stmt, gcc.GimpleAssign):
            return False
        if stmt.exprcode is not gcc.ComponentRef:
            return False
        rhs = stmt.rhs[0]
        if not isinstance(rhs, gcc.ComponentRef):
            return False
        if not isinstance(rhs.field, gcc.FieldDecl):
            return False
        return rhs.field.name == 'tp_dealloc' or rhs.field.name == 'tp_free'

    # Decode a function call and write something to the output.
    # THIS_FUN is the enclosing function that we are processing.
    # FNDECL is the call to process; it might not actually be a DECL
    # node.
    # LOC is the location of the call.
    def handle_one_fndecl(self, this_fun, fndecl, loc):
        callee_name = ''
        if isinstance(fndecl, gcc.AddrExpr):
            fndecl = fndecl.operand
        if isinstance(fndecl, gcc.FunctionDecl):
            # Ordinary call to a named function.
            callee_name = str(fndecl.name)
            self.output_file.write("function_call(%s, %s, %s)\n"
                                   % (repr(callee_name),
                                      repr(this_fun.decl.name),
                                      repr(str(loc))))
        elif self.fn_is_python_ignorable(fndecl):
            # Call to tp_dealloc.
            pass
        elif (isinstance(fndecl, gcc.SsaName)
              and isinstance(fndecl.var, gcc.ParmDecl)):
            # We can ignore an indirect call via a parameter to the
            # current function, because this is handled via the rule
            # for passthrough functions.
            pass
        else:
            # Any other indirect call.
            self.output_file.write("has_indirect_call(%s, %s)\n"
                                   % (repr(this_fun.decl.name),
                                      repr(str(loc))))
        return callee_name

    # This does most of the work for examine_one_bb.
    # THIS_FUN is the enclosing function.
    # BB is the basic block to process.
    # Returns True if this block is the header of a TRY_CATCH, False
    # otherwise.
    def examine_one_bb_inner(self, this_fun, bb):
        if not bb.gimple:
            return False
        try_catch = False
        for stmt in bb.gimple:
            loc = stmt.loc
            if not loc:
                loc = this_fun.decl.location
            if not isinstance(stmt, gcc.GimpleCall):
                continue
            callee_name = self.handle_one_fndecl(this_fun, stmt.fn, loc)

            if callee_name == 'exceptions_state_mc_action_iter':
                try_catch = True

            global non_passthrough_functions
            if callee_name in non_passthrough_functions:
                continue

            # We have to specially handle calls where an argument to
            # the call is itself a function, e.g., qsort.  In general
            # we model these as "passthrough" -- we assume that in
            # addition to the call the qsort there is also a call to
            # the argument function.
            for arg in stmt.args:
                # We are only interested in arguments which are functions.
                t = arg.type
                if isinstance(t, gcc.PointerType):
                    t = t.dereference
                if not isinstance(t, gcc.FunctionType):
                    continue

                if isinstance(arg, gcc.AddrExpr):
                    arg = arg.operand

                global cleanup_functions
                if callee_name in cleanup_functions:
                    if not isinstance(arg, gcc.FunctionDecl):
                        gcc.inform(loc, 'cleanup argument not a DECL: %s' % repr(arg))
                    else:
                        # Cleanups must be nothrow.
                        self.output_file.write("declare_cleanup(%s)\n"
                                               % repr(str(arg.name)))
                else:
                    # Assume we have a passthrough function, like
                    # qsort or an iterator.  We model this by
                    # pretending there is an ordinary call at this
                    # point.
                    self.handle_one_fndecl(this_fun, arg, loc)
        return try_catch

    # Examine all the calls in a basic block and generate output for
    # them.
    # THIS_FUN is the enclosing function.
    # BB is the basic block to examine.
    # BB_WORKLIST is a list of basic blocks to work on; we add the
    # appropriate successor blocks to this.
    # SEEN_BBS is a map whose keys are basic blocks we have already
    # processed.  We use this to ensure that we only visit a given
    # block once.
    def examine_one_bb(self, this_fun, bb, bb_worklist, seen_bbs):
        try_catch = self.examine_one_bb_inner(this_fun, bb)
        for edge in bb.succs:
            if edge.dest in seen_bbs:
                continue
            seen_bbs[edge.dest] = 1
            if try_catch:
                # This is bogus, but we magically know the right
                # answer.
                if edge.false_value:
                    bb_worklist.append(edge.dest)
            else:
                bb_worklist.append(edge.dest)

    # Iterate over all basic blocks in THIS_FUN.
    def iterate_bbs(self, this_fun):
        # Iteration must be in control-flow order, because if we see a
        # TRY_CATCH construct we need to drop all the contained blocks.
        bb_worklist = [this_fun.cfg.entry]
        seen_bbs = {}
        seen_bbs[this_fun.cfg.entry] = 1
        for bb in bb_worklist:
            self.examine_one_bb(this_fun, bb, bb_worklist, seen_bbs)

    def execute(self, fun):
        if fun and fun.cfg and fun.decl:
            self.output_file.write("################\n")
            self.output_file.write("# Analysis for %s\n" % fun.decl.name)
            self.output_file.write("define_function(%s, %s)\n"
                                   % (repr(fun.decl.name),
                                      repr(str(fun.decl.location))))

            global ignore_functions
            if fun.decl.name not in ignore_functions:
                self.iterate_bbs(fun)

def main(**kwargs):
    global output_file
    output_file = open(gcc.get_dump_base_name() + '.gdb_exc.py', 'w')
    # We used to use attributes here, but there didn't seem to be a
    # big benefit over hard-coding.
    output_file.write('declare_throw("throw_exception")\n')
    output_file.write('declare_throw("throw_verror")\n')
    output_file.write('declare_throw("throw_vfatal")\n')
    output_file.write('declare_throw("throw_error")\n')
    gcc.register_callback(gcc.PLUGIN_FINISH_UNIT, close_output)
    ps = GdbExceptionChecker(output_file)
    ps.register_after('ssa')

main()
