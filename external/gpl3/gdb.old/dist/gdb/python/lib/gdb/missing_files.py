# Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

"""
MissingFileHandler base class, and support functions used by the
missing_debug.py and missing_objfile.py modules.
"""

import sys

import gdb

if sys.version_info >= (3, 7):
    # Functions str.isascii() and str.isalnum are available starting Python
    # 3.7.
    def isascii(ch):
        return ch.isascii()

    def isalnum(ch):
        return ch.isalnum()

else:
    # Older version of Python doesn't have str.isascii() and
    # str.isalnum() so provide our own.
    #
    # We could import isalnum() and isascii() from the curses library,
    # but that adds an extra dependency.  Given these functions are
    # both small and trivial lets implement them here.
    #
    # These definitions are based on those in the curses library, but
    # simplified as we know C will always be a single character 'str'.

    def isdigit(c):
        return 48 <= ord(c) <= 57

    def islower(c):
        return 97 <= ord(c) <= 122

    def isupper(c):
        return 65 <= ord(c) <= 90

    def isalpha(c):
        return isupper(c) or islower(c)

    def isalnum(c):
        return isalpha(c) or isdigit(c)

    def isascii(c):
        return 0 <= ord(c) <= 127


def _validate_name(name):
    """Validate a missing file handler name string.

    If name is valid as a missing file handler name, then this
    function does nothing.  If name is not valid then an exception is
    raised.

    Arguments:
        name: A string, the name of a missing file handler.

    Returns:
        Nothing.

    Raises:
        ValueError: If name is invalid as a missing file handler
                    name.
    """

    for ch in name:
        if not isascii(ch) or not (isalnum(ch) or ch in "_-"):
            raise ValueError("invalid character '%s' in handler name: %s" % (ch, name))


class MissingFileHandler(object):
    """Base class for missing file handlers written in Python.

    A missing file handler has a single method __call__ along with the
    read/write attribute enabled, and a read-only attribute name.  The
    attributes are provided by this class while the __call__ method is
    provided by a sub-class.  Each sub-classes __call__ method will
    have a different signature.

    Attributes:
        name: Read-only attribute, the name of this handler.
        enabled: When true this handler is enabled.
    """

    def __init__(self, name):
        """Constructor.

        Args:
            name: An identifying name for this handler.

        Raises:
            TypeError: name is not a string.
            ValueError: name contains invalid characters.
        """

        if not isinstance(name, str):
            raise TypeError("incorrect type for name: %s" % type(name))

        _validate_name(name)

        self._name = name
        self._enabled = True

    @property
    def name(self):
        return self._name

    @property
    def enabled(self):
        return self._enabled

    @enabled.setter
    def enabled(self, value):
        if not isinstance(value, bool):
            raise TypeError("incorrect type for enabled attribute: %s" % type(value))
        self._enabled = value


def register_handler(handler_type, locus, handler, replace=False):
    """Register handler in given locus.

    The handler is prepended to the locus's missing file handlers
    list. The name of handler should be unique (or replace must be
    True), and the name must pass the _validate_name check.

    Arguments:
        handler_type: A string, either 'debug' or 'objfile' indicating the
            type of handler to be registered.
        locus: Either a progspace, or None (in which case the unwinder
               is registered globally).
        handler: An object used as a missing file handler.  Usually a
            sub-class of MissingFileHandler.
        replace: If True, replaces existing handler with the same name
                 within locus.  Otherwise, raises RuntimeException if
                 unwinder with the same name already exists.

    Returns:
        Nothing.

    Raises:
        RuntimeError: The name of handler is not unique.
        TypeError: Bad locus type.
        AttributeError: Required attributes of handler are missing.
        ValueError: If the name of the handler is invalid, or if
            handler_type is neither 'debug' or 'objfile'.
    """

    if handler_type != "debug" and handler_type != "objfile":
        raise ValueError("handler_type must be 'debug' or 'objfile'")

    if locus is None:
        if gdb.parameter("verbose"):
            gdb.write("Registering global %s handler ...\n" % handler.name)
        locus = gdb
    elif isinstance(locus, gdb.Progspace):
        if gdb.parameter("verbose"):
            gdb.write(
                "Registering %s handler for %s ...\n" % (handler.name, locus.filename)
            )
    else:
        raise TypeError("locus should be gdb.Progspace or None")

    # Some sanity checks on HANDLER.  Calling getattr will raise an
    # exception if the attribute doesn't exist, which is what we want.
    # These checks are not exhaustive; we don't check the attributes
    # have the correct types, or the method has the correct signature,
    # but this should catch some basic mistakes.
    name = getattr(handler, "name")
    _validate_name(name)

    getattr(handler, "enabled")

    call_method = getattr(handler, "__call__")
    if not callable(call_method):
        raise AttributeError(
            "'%s' object's '__call__' attribute is not callable"
            % type(handler).__name__
        )

    i = 0
    for needle in locus.missing_file_handlers:
        if needle[0] == handler_type and needle[1].name == handler.name:
            if replace:
                del locus.missing_file_handlers[i]
            else:
                raise RuntimeError("Handler %s already exists." % handler.name)
        i += 1
    locus.missing_file_handlers.insert(0, (handler_type, handler))
