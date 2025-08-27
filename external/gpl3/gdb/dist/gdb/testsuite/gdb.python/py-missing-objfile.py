# Copyright (C) 2024 Free Software Foundation, Inc.

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
import shutil
from enum import Enum

import gdb
from gdb.missing_objfile import MissingObjfileHandler

# A global log that is filled in by instances of the LOG_HANDLER class
# when they are called.
handler_call_log = []

# A global holding a string, the build-id of the last missing objfile
# which triggered the 'handler' class below.  This is set in the
# __call__ method of the 'handler' class and then checked from the
# expect script.
handler_last_buildid = None


# A global holding a string, the filename of the last missing objfile
# which triggered the 'handler' class below.  This is set in the
# __call__ method of the 'handler' class and then checked from the
# expect script.
handler_last_filename = None


# A helper function that makes some assertions about the arguments
# passed to a MissingObjfileHandler.__call__() method.
def check_args(pspace, buildid, filename):
    assert type(filename) == str
    assert filename != ""
    assert type(pspace) == gdb.Progspace
    assert type(buildid) == str
    assert buildid != ""


# Enum used to configure the 'handler' class from the test script.
class Mode(Enum):
    RETURN_NONE = 0
    RETURN_TRUE = 1
    RETURN_FALSE = 2
    RETURN_STRING = 3


# A missing objfile handler which can be configured to return each of
# the different possible return types.
class handler(MissingObjfileHandler):
    def __init__(self):
        super().__init__("handler")
        self._call_count = 0
        self._mode = Mode.RETURN_NONE

    def __call__(self, pspace, buildid, filename):
        global handler_call_log, handler_last_buildid, handler_last_filename
        check_args(pspace, buildid, filename)
        handler_call_log.append(self.name)
        handler_last_buildid = buildid
        handler_last_filename = filename
        self._call_count += 1
        if self._mode == Mode.RETURN_NONE:
            return None

        if self._mode == Mode.RETURN_TRUE:
            shutil.copy(self._src, self._dest)

            # If we're using the fission-dwp board then there will
            # also be a .dwp file that needs to be copied.
            dwp_src = self._src + ".dwp"
            if os.path.exists(dwp_src):
                dwp_dest = self._dest + ".dwp"
                shutil.copy(dwp_src, dwp_dest)

            return True

        if self._mode == Mode.RETURN_FALSE:
            return False

        if self._mode == Mode.RETURN_STRING:
            return self._dest

        assert False

    @property
    def call_count(self):
        """Return a count, the number of calls to __call__ since the last
        call to set_mode.
        """
        return self._call_count

    def set_mode(self, mode, *args):
        self._call_count = 0
        self._mode = mode

        if mode == Mode.RETURN_NONE:
            assert len(args) == 0
            return

        if mode == Mode.RETURN_TRUE:
            assert len(args) == 2
            self._src = args[0]
            self._dest = args[1]
            return

        if mode == Mode.RETURN_FALSE:
            assert len(args) == 0
            return

        if mode == Mode.RETURN_STRING:
            assert len(args) == 1
            self._dest = args[0]
            return

        assert False


# A missing objfile handler which raises an exception.  The type of
# exception to be raised is configured from the test script.
class exception_handler(MissingObjfileHandler):
    def __init__(self):
        super().__init__("exception_handler")
        self.exception_type = None

    def __call__(self, pspace, buildid, filename):
        global handler_call_log
        check_args(pspace, buildid, filename)
        handler_call_log.append(self.name)
        assert self.exception_type is not None
        raise self.exception_type("message")


# A very simple logging missing objfile handler.  Always returns None
# so that GDB will try any other registered handlers, but first logs
# the name of this handler into the global HANDLER_CALL_LOG, which can
# then be checked from the test script.
class log_handler(MissingObjfileHandler):
    def __call__(self, pspace, buildid, filename):
        global handler_call_log
        check_args(pspace, buildid, filename)
        handler_call_log.append(self.name)
        return None


# A basic helper function, this keeps lines shorter in the TCL script.
def register(name, locus=None):
    gdb.missing_objfile.register_handler(locus, log_handler(name))


# Create instances of the handlers, but don't install any.  We install
# these as needed from the TCL script.
rhandler = exception_handler()
handler_obj = handler()

print("Success")
