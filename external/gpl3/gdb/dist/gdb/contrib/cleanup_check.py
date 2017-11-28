#   Copyright 2013-2017 Free Software Foundation, Inc.
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

import gcc
import gccutils
import sys

want_raii_info = False

logging = False
show_cfg = False

def log(msg, indent=0):
    global logging
    if logging:
        sys.stderr.write('%s%s\n' % ('  ' * indent, msg))
        sys.stderr.flush()

def is_cleanup_type(return_type):
    if not isinstance(return_type, gcc.PointerType):
        return False
    if not isinstance(return_type.dereference, gcc.RecordType):
        return False
    if str(return_type.dereference.name) == 'cleanup':
        return True
    return False

def is_constructor(decl):
    "Return True if the function DECL is a cleanup constructor; False otherwise"
    return is_cleanup_type(decl.type.type) and (not decl.name or str(decl.name) != 'make_final_cleanup')

destructor_names = set(['do_cleanups', 'discard_cleanups'])

def is_destructor(decl):
    return decl.name in destructor_names

# This list is just much too long... we should probably have an
# attribute instead.
special_names = set(['do_final_cleanups', 'discard_final_cleanups',
                     'save_cleanups', 'save_final_cleanups',
                     'restore_cleanups', 'restore_final_cleanups',
                     'exceptions_state_mc_init',
                     'make_my_cleanup2', 'make_final_cleanup', 'all_cleanups',
                     'save_my_cleanups', 'quit_target'])

def needs_special_treatment(decl):
    return decl.name in special_names

# Sometimes we need a new placeholder object that isn't the same as
# anything else.
class Dummy(object):
    def __init__(self, location):
        self.location = location

# A wrapper for a cleanup which has been assigned to a variable.
# This holds the variable and the location.
class Cleanup(object):
    def __init__(self, var, location):
        self.var = var
        self.location = location

# A class representing a master cleanup.  This holds a stack of
# cleanup objects and supports a merging operation.
class MasterCleanup(object):
    # Create a new MasterCleanup object.  OTHER, if given, is a
    # MasterCleanup object to copy.
    def __init__(self, other = None):
        # 'cleanups' is a list of cleanups.  Each element is either a
        # Dummy, for an anonymous cleanup, or a Cleanup, for a cleanup
        # which was assigned to a variable.
        if other is None:
            self.cleanups = []
            self.aliases = {}
        else:
            self.cleanups = other.cleanups[:]
            self.aliases = dict(other.aliases)

    def compare_vars(self, definition, argument):
        if definition == argument:
            return True
        if argument in self.aliases:
            argument = self.aliases[argument]
        if definition in self.aliases:
            definition = self.aliases[definition]
        return definition == argument

    def note_assignment(self, lhs, rhs):
        log('noting assignment %s = %s' % (lhs, rhs), 4)
        self.aliases[lhs] = rhs

    # Merge with another MasterCleanup.
    # Returns True if this resulted in a change to our state.
    def merge(self, other):
        # We do explicit iteration like this so we can easily
        # update the list after the loop.
        counter = -1
        found_named = False
        for counter in range(len(self.cleanups) - 1, -1, -1):
            var = self.cleanups[counter]
            log('merge checking %s' % var, 4)
            # Only interested in named cleanups.
            if isinstance(var, Dummy):
                log('=> merge dummy', 5)
                continue
            # Now see if VAR is found in OTHER.
            if other._find_var(var.var) >= 0:
                log ('=> merge found', 5)
                break
            log('=>merge not found', 5)
            found_named = True
        if found_named and counter < len(self.cleanups) - 1:
            log ('merging to %d' % counter, 4)
            if counter < 0:
                self.cleanups = []
            else:
                self.cleanups = self.cleanups[0:counter]
            return True
        # If SELF is empty but OTHER has some cleanups, then consider
        # that a change as well.
        if len(self.cleanups) == 0 and len(other.cleanups) > 0:
            log('merging non-empty other', 4)
            self.cleanups = other.cleanups[:]
            return True
        return False

    # Push a new constructor onto our stack.  LHS is the
    # left-hand-side of the GimpleCall statement.  It may be None,
    # meaning that this constructor's value wasn't used.
    def push(self, location, lhs):
        if lhs is None:
            obj = Dummy(location)
        else:
            obj = Cleanup(lhs, location)
        log('pushing %s' % lhs, 4)
        idx = self._find_var(lhs)
        if idx >= 0:
            gcc.permerror(location, 'reassigning to known cleanup')
            gcc.inform(self.cleanups[idx].location,
                       'previous assignment is here')
        self.cleanups.append(obj)

    # A helper for merge and pop that finds BACK_TO in self.cleanups,
    # and returns the index, or -1 if not found.
    def _find_var(self, back_to):
        for i in range(len(self.cleanups) - 1, -1, -1):
            if isinstance(self.cleanups[i], Dummy):
                continue
            if self.compare_vars(self.cleanups[i].var, back_to):
                return i
        return -1

    # Pop constructors until we find one matching BACK_TO.
    # This is invoked when we see a do_cleanups call.
    def pop(self, location, back_to):
        log('pop:', 4)
        i = self._find_var(back_to)
        if i >= 0:
            self.cleanups = self.cleanups[0:i]
        else:
            gcc.permerror(location, 'destructor call with unknown argument')

    # Check whether ARG is the current master cleanup.  Return True if
    # all is well.
    def verify(self, location, arg):
        log('verify %s' % arg, 4)
        return (len(self.cleanups) > 0
                and not isinstance(self.cleanups[0], Dummy)
                and self.compare_vars(self.cleanups[0].var, arg))

    # Check whether SELF is empty.
    def isempty(self):
        log('isempty: len = %d' % len(self.cleanups), 4)
        return len(self.cleanups) == 0

    # Emit informational warnings about the cleanup stack.
    def inform(self):
        for item in reversed(self.cleanups):
            gcc.inform(item.location, 'leaked cleanup')

class CleanupChecker:
    def __init__(self, fun):
        self.fun = fun
        self.seen_edges = set()
        self.bad_returns = set()

        # This maps BB indices to a list of master cleanups for the
        # BB.
        self.master_cleanups = {}

    # Pick a reasonable location for the basic block BB.
    def guess_bb_location(self, bb):
        if isinstance(bb.gimple, list):
            for stmt in bb.gimple:
                if stmt.loc:
                    return stmt.loc
        return self.fun.end

    # Compute the master cleanup list for BB.
    # Modifies MASTER_CLEANUP in place.
    def compute_master(self, bb, bb_from, master_cleanup):
        if not isinstance(bb.gimple, list):
            return
        curloc = self.fun.end
        for stmt in bb.gimple:
            if stmt.loc:
                curloc = stmt.loc
            if isinstance(stmt, gcc.GimpleCall) and stmt.fndecl:
                if is_constructor(stmt.fndecl):
                    log('saw constructor %s in bb=%d' % (str(stmt.fndecl), bb.index), 2)
                    self.cleanup_aware = True
                    master_cleanup.push(curloc, stmt.lhs)
                elif is_destructor(stmt.fndecl):
                    if str(stmt.fndecl.name) != 'do_cleanups':
                        self.only_do_cleanups_seen = False
                    log('saw destructor %s in bb=%d, bb_from=%d, argument=%s'
                        % (str(stmt.fndecl.name), bb.index, bb_from, str(stmt.args[0])),
                        2)
                    master_cleanup.pop(curloc, stmt.args[0])
                elif needs_special_treatment(stmt.fndecl):
                    pass
                    # gcc.permerror(curloc, 'function needs special treatment')
            elif isinstance(stmt, gcc.GimpleAssign):
                if isinstance(stmt.lhs, gcc.VarDecl) and isinstance(stmt.rhs[0], gcc.VarDecl):
                    master_cleanup.note_assignment(stmt.lhs, stmt.rhs[0])
            elif isinstance(stmt, gcc.GimpleReturn):
                if self.is_constructor:
                    if not master_cleanup.verify(curloc, stmt.retval):
                        gcc.permerror(curloc,
                                      'constructor does not return master cleanup')
                elif not self.is_special_constructor:
                    if not master_cleanup.isempty():
                        if curloc not in self.bad_returns:
                            gcc.permerror(curloc, 'cleanup stack is not empty at return')
                            self.bad_returns.add(curloc)
                            master_cleanup.inform()

    # Traverse a basic block, updating the master cleanup information
    # and propagating to other blocks.
    def traverse_bbs(self, edge, bb, bb_from, entry_master):
        log('traverse_bbs %d from %d' % (bb.index, bb_from), 1)

        # Propagate the entry MasterCleanup though this block.
        master_cleanup = MasterCleanup(entry_master)
        self.compute_master(bb, bb_from, master_cleanup)

        modified = False
        if bb.index in self.master_cleanups:
            # Merge the newly-computed MasterCleanup into the one we
            # have already computed.  If this resulted in a
            # significant change, then we need to re-propagate.
            modified = self.master_cleanups[bb.index].merge(master_cleanup)
        else:
            self.master_cleanups[bb.index] = master_cleanup
            modified = True

        # EDGE is None for the entry BB.
        if edge is not None:
            # If merging cleanups caused a change, check to see if we
            # have a bad loop.
            if edge in self.seen_edges:
                # This error doesn't really help.
                # if modified:
                #     gcc.permerror(self.guess_bb_location(bb),
                #                   'invalid cleanup use in loop')
                return
            self.seen_edges.add(edge)

        if not modified:
            return

        # Now propagate to successor nodes.
        for edge in bb.succs:
            self.traverse_bbs(edge, edge.dest, bb.index, master_cleanup)

    def check_cleanups(self):
        if not self.fun.cfg or not self.fun.decl:
            return 'ignored'
        if is_destructor(self.fun.decl):
            return 'destructor'
        if needs_special_treatment(self.fun.decl):
            return 'special'

        self.is_constructor = is_constructor(self.fun.decl)
        self.is_special_constructor = not self.is_constructor and str(self.fun.decl.name).find('with_cleanup') > -1
        # Yuck.
        if str(self.fun.decl.name) == 'gdb_xml_create_parser_and_cleanup_1':
            self.is_special_constructor = True

        if self.is_special_constructor:
            gcc.inform(self.fun.start, 'function %s is a special constructor' % (self.fun.decl.name))

        # If we only see do_cleanups calls, and this function is not
        # itself a constructor, then we can convert it easily to RAII.
        self.only_do_cleanups_seen = not self.is_constructor
        # If we ever call a constructor, then we are "cleanup-aware".
        self.cleanup_aware = False

        entry_bb = self.fun.cfg.entry
        master_cleanup = MasterCleanup()
        self.traverse_bbs(None, entry_bb, -1, master_cleanup)
        if want_raii_info and self.only_do_cleanups_seen and self.cleanup_aware:
            gcc.inform(self.fun.decl.location,
                       'function %s could be converted to RAII' % (self.fun.decl.name))
        if self.is_constructor:
            return 'constructor'
        return 'OK'

class CheckerPass(gcc.GimplePass):
    def execute(self, fun):
        if fun.decl:
            log("Starting " + fun.decl.name)
            if show_cfg:
                dot = gccutils.cfg_to_dot(fun.cfg, fun.decl.name)
                gccutils.invoke_dot(dot, name=fun.decl.name)
        checker = CleanupChecker(fun)
        what = checker.check_cleanups()
        if fun.decl:
            log(fun.decl.name + ': ' + what, 2)

ps = CheckerPass(name = 'check-cleanups')
# We need the cfg, but we want a relatively high-level Gimple.
ps.register_after('cfg')
