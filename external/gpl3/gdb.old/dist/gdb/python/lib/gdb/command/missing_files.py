# Missing debug and objfile related commands.
#
# Copyright 2023-2024 Free Software Foundation, Inc.
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

import re

import gdb


def validate_regexp(exp, idstring):
    """Compile exp into a compiled regular expression object.

    Arguments:
        exp: The string to compile into a re.Pattern object.
        idstring: A string, what exp is a regexp for.

    Returns:
        A re.Pattern object representing exp.

    Raises:
        SyntaxError: If exp is an invalid regexp.
    """

    try:
        return re.compile(exp)
    except SyntaxError:
        raise SyntaxError("Invalid %s regexp: %s." % (idstring, exp))


def parse_missing_file_command_args(arg):
    """Internal utility to parse missing file handler command argv.

    Arguments:
        arg: The arguments to the command. The format is:
             [locus-regexp [name-regexp]]

    Returns:
        A 2-tuple of compiled regular expressions.

    Raises:
        SyntaxError: an error processing ARG
    """

    argv = gdb.string_to_argv(arg)
    argc = len(argv)
    if argc > 2:
        raise SyntaxError("Too many arguments.")
    locus_regexp = ""
    name_regexp = ""
    if argc >= 1:
        locus_regexp = argv[0]
        if argc >= 2:
            name_regexp = argv[1]
    return (
        validate_regexp(locus_regexp, "locus"),
        validate_regexp(name_regexp, "handler"),
    )


class InfoMissingFileHandlers(gdb.Command):
    """GDB command to list missing HTYPE handlers.

    Usage: info missing-HTYPE-handlers [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression matching the location of the
    handler.  If it is omitted, all registered handlers from all
    loci are listed.  A locus can be 'global', 'progspace' to list
    the handlers from the current progspace, or a regular expression
    matching filenames of progspaces.

    NAME-REGEXP is a regular expression to filter missing HTYPE
    handler names.  If this omitted for a specified locus, then all
    registered handlers in the locus are listed.
    """

    def __init__(self, handler_type):
        # Update the doc string before calling the parent constructor,
        # replacing the string 'HTYPE' with the value of HANDLER_TYPE.
        # The parent constructor will grab a copy of this string to
        # use as the commands help text.
        self.__doc__ = self.__doc__.replace("HTYPE", handler_type)
        super().__init__(
            "info missing-" + handler_type + "-handlers", gdb.COMMAND_FILES
        )
        self.handler_type = handler_type

    def list_handlers(self, title, handlers, name_re):
        """Lists the missing file handlers whose name matches regexp.

        Arguments:
            title: The line to print before the list.
            handlers: The list of the missing file handlers.
            name_re: handler name filter.
        """

        if not handlers:
            return
        print(title)
        for handler in gdb._filter_missing_file_handlers(handlers, self.handler_type):
            if name_re.match(handler.name):
                print(
                    "  %s%s" % (handler.name, "" if handler.enabled else " [disabled]")
                )

    def invoke(self, arg, from_tty):
        locus_re, name_re = parse_missing_file_command_args(arg)

        if locus_re.match("progspace") and locus_re.pattern != "":
            cp = gdb.current_progspace()
            self.list_handlers(
                "Progspace %s:" % cp.filename, cp.missing_file_handlers, name_re
            )

        for progspace in gdb.progspaces():
            filename = progspace.filename or ""
            if locus_re.match(filename):
                if filename == "":
                    if progspace == gdb.current_progspace():
                        msg = "Current Progspace:"
                    else:
                        msg = "Progspace <no-file>:"
                else:
                    msg = "Progspace %s:" % filename
                self.list_handlers(
                    msg,
                    progspace.missing_file_handlers,
                    name_re,
                )

        # Print global handlers last, as these are invoked last.
        if locus_re.match("global"):
            self.list_handlers("Global:", gdb.missing_file_handlers, name_re)


def do_enable_handler1(handlers, name_re, flag, handler_type):
    """Enable/disable missing file handlers whose names match given regex.

    Arguments:
        handlers: The list of missing file handlers.
        name_re: Handler name filter.
        flag: A boolean indicating if we should enable or disable.
        handler_type: A string, either 'debug' or 'objfile', use to control
            which handlers are modified.

    Returns:
        The number of handlers affected.
    """

    total = 0
    for handler in gdb._filter_missing_file_handlers(handlers, handler_type):
        if name_re.match(handler.name) and handler.enabled != flag:
            handler.enabled = flag
            total += 1
    return total


def do_enable_handler(arg, flag, handler_type):
    """Enable or disable missing file handlers."""

    (locus_re, name_re) = parse_missing_file_command_args(arg)
    total = 0
    if locus_re.match("global"):
        total += do_enable_handler1(
            gdb.missing_file_handlers, name_re, flag, handler_type
        )
    if locus_re.match("progspace") and locus_re.pattern != "":
        total += do_enable_handler1(
            gdb.current_progspace().missing_file_handlers, name_re, flag, handler_type
        )
    for progspace in gdb.progspaces():
        filename = progspace.filename or ""
        if locus_re.match(filename):
            total += do_enable_handler1(
                progspace.missing_file_handlers, name_re, flag, handler_type
            )
    print(
        "%d missing %s handler%s %s"
        % (
            total,
            handler_type,
            "" if total == 1 else "s",
            "enabled" if flag else "disabled",
        )
    )


class EnableMissingFileHandler(gdb.Command):
    """GDB command to enable missing HTYPE handlers.

    Usage: enable missing-HTYPE-handler [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression specifying the handlers to
    enable.  It can be 'global', 'progspace' for the current
    progspace, or the filename for a file associated with a progspace.

    NAME_REGEXP is a regular expression to filter handler names.  If
    this omitted for a specified locus, then all registered handlers
    in the locus are affected.
    """

    def __init__(self, handler_type):
        # Update the doc string before calling the parent constructor,
        # replacing the string 'HTYPE' with the value of HANDLER_TYPE.
        # The parent constructor will grab a copy of this string to
        # use as the commands help text.
        self.__doc__ = self.__doc__.replace("HTYPE", handler_type)
        super().__init__(
            "enable missing-" + handler_type + "-handler", gdb.COMMAND_FILES
        )
        self.handler_type = handler_type

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        do_enable_handler(arg, True, self.handler_type)


class DisableMissingFileHandler(gdb.Command):
    """GDB command to disable missing HTYPE handlers.

    Usage: disable missing-HTYPE-handler [LOCUS-REGEXP [NAME-REGEXP]]

    LOCUS-REGEXP is a regular expression specifying the handlers to
    enable.  It can be 'global', 'progspace' for the current
    progspace, or the filename for a file associated with a progspace.

    NAME_REGEXP is a regular expression to filter handler names.  If
    this omitted for a specified locus, then all registered handlers
    in the locus are affected.
    """

    def __init__(self, handler_type):
        # Update the doc string before calling the parent constructor,
        # replacing the string 'HTYPE' with the value of HANDLER_TYPE.
        # The parent constructor will grab a copy of this string to
        # use as the commands help text.
        self.__doc__ = self.__doc__.replace("HTYPE", handler_type)
        super().__init__(
            "disable missing-" + handler_type + "-handler", gdb.COMMAND_FILES
        )
        self.handler_type = handler_type

    def invoke(self, arg, from_tty):
        """GDB calls this to perform the command."""
        do_enable_handler(arg, False, self.handler_type)


def register_missing_file_handler_commands():
    """Installs the missing file handler commands."""
    for handler_type in ["debug", "objfile"]:
        InfoMissingFileHandlers(handler_type)
        EnableMissingFileHandler(handler_type)
        DisableMissingFileHandler(handler_type)


register_missing_file_handler_commands()
