# Copyright (C) 2010-2025 Free Software Foundation, Inc.

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

import os
import signal
import sys
import threading
import traceback
from contextlib import contextmanager
from importlib import reload

# The star import imports _gdb names.  When the names are used locally, they
# trigger F405 warnings unless added to the explicit import list.
# Note that two indicators are needed here to silence flake8.
from _gdb import *  # noqa: F401,F403
from _gdb import (
    STDERR,
    STDOUT,
    Command,
    execute,
    flush,
    parameter,
    selected_inferior,
    write,
)

# isort: split

# Historically, gdb.events was always available, so ensure it's
# still available without an explicit import.
import _gdbevents as events

sys.modules["gdb.events"] = events


class _GdbFile(object):
    # These two are needed in Python 3
    encoding = "UTF-8"
    errors = "strict"

    def __init__(self, stream):
        self.stream = stream

    def close(self):
        # Do nothing.
        return None

    def isatty(self):
        return False

    def writelines(self, iterable):
        for line in iterable:
            self.write(line)

    def flush(self):
        flush(stream=self.stream)

    def write(self, s):
        write(s, stream=self.stream)


sys.stdout = _GdbFile(STDOUT)
sys.stderr = _GdbFile(STDERR)

# Default prompt hook does nothing.
prompt_hook = None

# Ensure that sys.argv is set to something.
# We do not use PySys_SetArgvEx because it did not appear until 2.6.6.
sys.argv = [""]

# Initial pretty printers.
pretty_printers = []

# Initial type printers.
type_printers = []
# Initial xmethod matchers.
xmethods = []
# Initial frame filters.
frame_filters = {}
# Initial frame unwinders.
frame_unwinders = []
# The missing file handlers.  Each item is a tuple with the form
# (TYPE, HANDLER) where TYPE is a string either 'debug' or 'objfile'.
missing_file_handlers = []


def _execute_unwinders(pending_frame):
    """Internal function called from GDB to execute all unwinders.

    Runs each currently enabled unwinder until it finds the one that
    can unwind given frame.

    Arguments:
        pending_frame: gdb.PendingFrame instance.

    Returns:
        Tuple with:

          [0] gdb.UnwindInfo instance
          [1] Name of unwinder that claimed the frame (type `str`)

        or None, if no unwinder has claimed the frame.
    """
    for objfile in objfiles():
        for unwinder in objfile.frame_unwinders:
            if unwinder.enabled:
                unwind_info = unwinder(pending_frame)
                if unwind_info is not None:
                    return (unwind_info, unwinder.name)

    for unwinder in current_progspace().frame_unwinders:
        if unwinder.enabled:
            unwind_info = unwinder(pending_frame)
            if unwind_info is not None:
                return (unwind_info, unwinder.name)

    for unwinder in frame_unwinders:
        if unwinder.enabled:
            unwind_info = unwinder(pending_frame)
            if unwind_info is not None:
                return (unwind_info, unwinder.name)

    return None


# Convenience variable to GDB's python directory
PYTHONDIR = os.path.dirname(os.path.dirname(__file__))

# Auto-load all functions/commands.

# Packages to auto-load.

packages = ["function", "command", "printer"]

# pkgutil.iter_modules is not available prior to Python 2.6.  Instead,
# manually iterate the list, collating the Python files in each module
# path.  Construct the module name, and import.


def _auto_load_packages():
    for package in packages:
        location = os.path.join(os.path.dirname(__file__), package)
        if os.path.exists(location):
            py_files = filter(
                lambda x: x.endswith(".py") and x != "__init__.py", os.listdir(location)
            )

            for py_file in py_files:
                # Construct from foo.py, gdb.module.foo
                modname = "%s.%s.%s" % (__name__, package, py_file[:-3])
                try:
                    if modname in sys.modules:
                        # reload modules with duplicate names
                        reload(__import__(modname))
                    else:
                        __import__(modname)
                except Exception:
                    sys.stderr.write(traceback.format_exc() + "\n")


_auto_load_packages()


def GdbSetPythonDirectory(dir):
    """Update sys.path, reload gdb and auto-load packages."""
    global PYTHONDIR

    try:
        sys.path.remove(PYTHONDIR)
    except ValueError:
        pass
    sys.path.insert(0, dir)

    PYTHONDIR = dir

    # note that reload overwrites the gdb module without deleting existing
    # attributes
    reload(__import__(__name__))
    _auto_load_packages()


def current_progspace():
    "Return the current Progspace."
    return selected_inferior().progspace


def objfiles():
    "Return a sequence of the current program space's objfiles."
    return current_progspace().objfiles()


def solib_name(addr):
    """solib_name (Long) -> String.\n\
Return the name of the shared library holding a given address, or None."""
    return current_progspace().solib_name(addr)


def block_for_pc(pc):
    "Return the block containing the given pc value, or None."
    return current_progspace().block_for_pc(pc)


def find_pc_line(pc):
    """find_pc_line (pc) -> Symtab_and_line.
    Return the gdb.Symtab_and_line object corresponding to the pc value."""
    return current_progspace().find_pc_line(pc)


def set_parameter(name, value):
    """Set the GDB parameter NAME to VALUE."""
    # Handle the specific cases of None and booleans here, because
    # gdb.parameter can return them, but they can't be passed to 'set'
    # this way.
    if value is None:
        value = "unlimited"
    elif isinstance(value, bool):
        if value:
            value = "on"
        else:
            value = "off"
    execute("set " + name + " " + str(value), to_string=True)


@contextmanager
def with_parameter(name, value):
    """Temporarily set the GDB parameter NAME to VALUE.
    Note that this is a context manager."""
    old_value = parameter(name)
    set_parameter(name, value)
    try:
        # Nothing that useful to return.
        yield None
    finally:
        set_parameter(name, old_value)


@contextmanager
def blocked_signals():
    """A helper function that blocks and unblocks signals."""
    if not hasattr(signal, "pthread_sigmask"):
        yield
        return

    to_block = {signal.SIGCHLD, signal.SIGINT, signal.SIGALRM, signal.SIGWINCH}
    old_mask = signal.pthread_sigmask(signal.SIG_BLOCK, to_block)
    try:
        yield None
    finally:
        signal.pthread_sigmask(signal.SIG_SETMASK, old_mask)


class Thread(threading.Thread):
    """A GDB-specific wrapper around threading.Thread

    This wrapper ensures that the new thread blocks any signals that
    must be delivered on GDB's main thread."""

    def start(self):
        # GDB requires that these be delivered to the main thread.  We
        # do this here to avoid any possible race with the creation of
        # the new thread.  The thread mask is inherited by new
        # threads.
        with blocked_signals():
            super().start()


def _filter_missing_file_handlers(handlers, handler_type):
    """Each list of missing file handlers is a list of tuples, the first
    item in the tuple is a string either 'debug' or 'objfile' to
    indicate what type of handler it is.  The second item in the tuple
    is the actual handler object.

    This function takes HANDLER_TYPE which is a string, either 'debug'
    or 'objfile' and HANDLERS, a list of tuples.  The function returns
    an iterable over all of the handler objects (extracted from the
    tuples) which match HANDLER_TYPE.
    """

    return map(lambda t: t[1], filter(lambda t: t[0] == handler_type, handlers))


def _handle_missing_files(pspace, handler_type, cb):
    """Helper for _handle_missing_debuginfo and _handle_missing_objfile.

    Arguments:
        pspace: The gdb.Progspace in which we're operating.  Used to
            lookup program space specific handlers.
        handler_type: A string, either 'debug' or 'objfile', this is the
            type of handler we're looking for.
        cb: A callback which takes a handler and returns the result of
            calling the handler.

    Returns:
        None: No suitable file could be found.
        False: A handler has decided that the requested file cannot be
                found, and no further searching should be done.
        True: The file has been found and installed in a location
                where GDB would normally look for it.  GDB should
                repeat its lookup process, the file should now be in
                place.
        A string: This is the filename of where the missing file can
                be found.
    """

    for handler in _filter_missing_file_handlers(
        pspace.missing_file_handlers, handler_type
    ):
        if handler.enabled:
            result = cb(handler)
            if result is not None:
                return result

    for handler in _filter_missing_file_handlers(missing_file_handlers, handler_type):
        if handler.enabled:
            result = cb(handler)
            if result is not None:
                return result

    return None


def _handle_missing_debuginfo(objfile):
    """Internal function called from GDB to execute missing debug
    handlers.

    Run each of the currently registered, and enabled missing debug
    handler objects for the current program space and then from the
    global list.  Stop after the first handler that returns a result
    other than None.

    Arguments:
        objfile: A gdb.Objfile for which GDB could not find any debug
                 information.

    Returns:
        None: No debug information could be found for objfile.
        False: A handler has done all it can with objfile, but no
               debug information could be found.
        True: Debug information might have been installed by a
              handler, GDB should check again.
        A string: This is the filename of a file containing the
                  required debug information.
    """

    pspace = objfile.progspace

    return _handle_missing_files(pspace, "debug", lambda h: h(objfile))


def _handle_missing_objfile(pspace, buildid, filename):
    """Internal function called from GDB to execute missing objfile
    handlers.

    Run each of the currently registered, and enabled missing objfile
    handler objects for the gdb.Progspace passed in as an argument,
    and then from the global list.  Stop after the first handler that
    returns a result other than None.

    Arguments:
        pspace: A gdb.Progspace for which the missing objfile handlers
                should be run.  This is the program space in which an
                objfile was found to be missing.
        buildid: A string containing the build-id we're looking for.
        filename: The filename of the file GDB tried to find but
                  couldn't.  This is not where the file should be
                  placed if found, in fact, this file might already
                  exist on disk but have the wrong build-id.  This is
                  mostly provided in order to be used in messages to
                  the user.

    Returns:
        None: No objfile could be found for this build-id.
        False: A handler has done all it can with for this build-id,
               but no objfile could be found.
        True: An objfile might have been installed by a handler, GDB
              should check again.  The only place GDB checks is within
              the .build-id sub-directory within the
              debug-file-directory.  If the required file was not
              installed there then GDB will not find it.
        A string: This is the filename of a file containing the
                  missing objfile.
    """

    return _handle_missing_files(
        pspace, "objfile", lambda h: h(pspace, buildid, filename)
    )


class ParameterPrefix:
    # A wrapper around gdb.Command for creating set/show prefixes.
    #
    # When creating a gdb.Parameter sub-classes, it is sometimes necessary
    # to first create a gdb.Command object in order to create the needed
    # command prefix.  However, for parameters, we actually need two
    # prefixes, a 'set' prefix, and a 'show' prefix.  With this helper
    # class, a single instance of this class will create both prefixes at
    # once.
    #
    # It is important that this class-level documentation not be a __doc__
    # string.  Users are expected to sub-class this ParameterPrefix class
    # and add their own documentation.  If they don't, then GDB will
    # generate a suitable doc string.  But, if this (parent) class has a
    # __doc__ string of its own, then sub-classes will inherit that __doc__
    # string, and GDB will not understand that it needs to generate one.

    class _PrefixCommand(Command):
        """A gdb.Command used to implement both the set and show prefixes.

        This documentation string is not used as the prefix command
        documentation as it is overridden in the __init__ method below."""

        # This private method is connected to the 'invoke' attribute within
        # this _PrefixCommand object if the containing ParameterPrefix
        # object has an invoke_set or invoke_show method.
        #
        # This method records within self.__delegate which _PrefixCommand
        # object is currently active, and then calls the correct invoke
        # method on the delegat object (the ParameterPrefix sub-class
        # object).
        #
        # Recording the currently active _PrefixCommand object is important;
        # if from the invoke method the user calls dont_repeat, then this is
        # forwarded to the currently active _PrefixCommand object.
        def __invoke(self, args, from_tty):

            # A helper class for use as part of a Python 'with' block.
            # Records which gdb.Command object is currently running its
            # invoke method.
            class MarkActiveCallback:
                # The CMD is a _PrefixCommand object, and the DELEGATE is
                # the ParameterPrefix class, or sub-class object.  At this
                # point we simple record both of these within the
                # MarkActiveCallback object.
                def __init__(self, cmd, delegate):
                    self.__cmd = cmd
                    self.__delegate = delegate

                # Record the currently active _PrefixCommand object within
                # the outer ParameterPrefix sub-class object.
                def __enter__(self):
                    self.__delegate.active_prefix = self.__cmd

                # Once the invoke method has completed, then clear the
                # _PrefixCommand object that was stored into the outer
                # ParameterPrefix sub-class object.
                def __exit__(self, exception_type, exception_value, traceback):
                    self.__delegate.active_prefix = None

            # The self.__cb attribute is set when the _PrefixCommand object
            # is created, and is either invoke_set or invoke_show within the
            # ParameterPrefix sub-class object.
            assert callable(self.__cb)

            # Record the currently active _PrefixCommand object within the
            # ParameterPrefix sub-class object, then call the relevant
            # invoke method within the ParameterPrefix sub-class object.
            with MarkActiveCallback(self, self.__delegate):
                self.__cb(args, from_tty)

        @staticmethod
        def __find_callback(delegate, mode):
            """The MODE is either 'set' or 'show'.  Look for an invoke_MODE method
            on DELEGATE, if a suitable method is found, then return it, otherwise,
            return None.
            """
            cb = getattr(delegate, "invoke_" + mode, None)
            if callable(cb):
                return cb
            return None

        def __init__(self, mode, name, cmd_class, delegate, doc=None):
            """Setup this gdb.Command.  Mode is a string, either 'set' or 'show'.
            NAME is the name for this prefix command, that is, the
            words that appear after both 'set' and 'show' in the
            command name.  CMD_CLASS is the usual enum.  And DELEGATE
            is the gdb.ParameterPrefix object this prefix is part of.
            """
            assert mode == "set" or mode == "show"
            if doc is None:
                self.__doc__ = delegate.__doc__
            else:
                self.__doc__ = doc
            self.__cb = self.__find_callback(delegate, mode)
            self.__delegate = delegate
            if self.__cb is not None:
                self.invoke = self.__invoke
            super().__init__(mode + " " + name, cmd_class, prefix=True)

    def __init__(self, name, cmd_class, doc=None):
        """Create a _PrefixCommand for both the set and show prefix commands.
        NAME is the command name without either the leading 'set ' or
        'show ' strings, and CMD_CLASS is the usual enum value.
        """
        self.active_prefix = None
        self._set_prefix_cmd = self._PrefixCommand("set", name, cmd_class, self, doc)
        self._show_prefix_cmd = self._PrefixCommand("show", name, cmd_class, self, doc)

    # When called from within an invoke method the self.active_prefix
    # attribute should be set to a gdb.Command sub-class (a _PrefixCommand
    # object, see above).  Forward the dont_repeat call to this object to
    # register the actual command as none repeating.
    def dont_repeat(self):
        if self.active_prefix is not None:
            self.active_prefix.dont_repeat()
