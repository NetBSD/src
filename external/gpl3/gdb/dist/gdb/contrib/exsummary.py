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

import sys
import glob

# Compute the summary information from the files created by
# excheck.py.  Run in the build directory where you used the
# excheck.py plugin.

class Function:
    def __init__(self, name):
        self.name = name
        self.location = None
        self.callers = []
        self.can_throw = False
        self.marked_nothrow = False
        self.reason = None

    def log(self, message):
        print "%s: note: %s" % (self.location, message)

    def set_location(self, location):
        self.location = location

    # CALLER is an Edge.
    def add_caller(self, caller):
        # self.log("adding call from %s" % caller.from_fn.name)
        self.callers.append(caller)
        # self.log("len = %d" % len(self.callers))

    def consistency_check(self):
        if self.marked_nothrow and self.can_throw:
            print ("%s: error: %s marked as both 'throw' and 'nothrow'"
                   % (self.location, self.name))

    def declare_nothrow(self):
        self.marked_nothrow = True
        self.consistency_check()

    def declare_throw(self):
        result = not self.can_throw # Return True the first time
        self.can_throw = True
        self.consistency_check()
        return result

    def print_stack(self, is_indirect):
        if is_indirect:
            print ("%s: error: function %s is marked nothrow but is assumed to throw due to indirect call"
                   % (self.location, self.name))
        else:
            print ("%s: error: function %s is marked nothrow but can throw"
                   % (self.location, self.name))

        edge = self.reason
        while edge is not None:
            print ("%s: info: via call to %s"
                   % (edge.location, edge.to_fn.name))
            edge = edge.to_fn.reason

    def mark_throw(self, edge, work_list, is_indirect):
        if not self.can_throw:
            # self.log("can throw")
            self.can_throw = True
            self.reason = edge
            if self.marked_nothrow:
                self.print_stack(is_indirect)
            else:
                # Do this in the 'else' to avoid extra error
                # propagation.
                work_list.append(self)

class Edge:
    def __init__(self, from_fn, to_fn, location):
        self.from_fn = from_fn
        self.to_fn = to_fn
        self.location = location

# Work list of known-throwing functions.
work_list = []
# Map from function name to Function object.
function_map = {}
# Work list of indirect calls.
indirect_functions = []
# Whether we should process cleanup functions as well.
process_cleanups = False
# Whether we should process indirect function calls.
process_indirect = False

def declare(fn_name):
    global function_map
    if fn_name not in function_map:
        function_map[fn_name] = Function(fn_name)
    return function_map[fn_name]

def define_function(fn_name, location):
    fn = declare(fn_name)
    fn.set_location(location)

def declare_throw(fn_name):
    global work_list
    fn = declare(fn_name)
    if fn.declare_throw():
        work_list.append(fn)

def declare_nothrow(fn_name):
    fn = declare(fn_name)
    fn.declare_nothrow()

def declare_cleanup(fn_name):
    global process_cleanups
    fn = declare(fn_name)
    if process_cleanups:
        fn.declare_nothrow()

def function_call(to, frm, location):
    to_fn = declare(to)
    frm_fn = declare(frm)
    to_fn.add_caller(Edge(frm_fn, to_fn, location))

def has_indirect_call(fn_name, location):
    global indirect_functions
    fn = declare(fn_name)
    phony = Function("<indirect call>")
    phony.add_caller(Edge(fn, phony, location))
    indirect_functions.append(phony)

def mark_functions(worklist, is_indirect):
    for callee in worklist:
        for edge in callee.callers:
            edge.from_fn.mark_throw(edge, worklist, is_indirect)

def help_and_exit():
    print "Usage: exsummary [OPTION]..."
    print ""
    print "Read the .py files from the exception checker plugin and"
    print "generate an error summary."
    print ""
    print "  --cleanups     Include invalid behavior in cleanups"
    print "  --indirect     Include assumed errors due to indirect function calls"
    sys.exit(0)

def main():
    global work_list
    global indirect_functions
    global process_cleanups
    global process_indirect

    for arg in sys.argv:
        if arg == '--cleanups':
            process_cleanups = True
        elif arg == '--indirect':
            process_indirect = True
        elif arg == '--help':
            help_and_exit()

    for fname in sorted(glob.glob('*.c.gdb_exc.py')):
        execfile(fname)
    print "================"
    print "= Ordinary marking"
    print "================"
    mark_functions(work_list, False)
    if process_indirect:
        print "================"
        print "= Indirect marking"
        print "================"
        mark_functions(indirect_functions, True)
    return 0

if __name__ == '__main__':
    status = main()
    sys.exit(status)
