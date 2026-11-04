/* Functions to deal with the inferior being executed on GDB or
   GDBserver.

   Copyright (C) 1986-2025 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef GDBSUPPORT_COMMON_INFERIOR_H
#define GDBSUPPORT_COMMON_INFERIOR_H

#include "gdbsupport/array-view.h"

/* Return the exec wrapper to be used when starting the inferior, or NULL
   otherwise.  */
extern const char *get_exec_wrapper ();

/* Return the inferior's current working directory.

   If it is not set, the string is empty.  */
extern const std::string &get_inferior_cwd ();

/* Whether to start up the debuggee under a shell.

   If startup-with-shell is set, GDB's "run" will attempt to start up
   the debuggee under a shell.  This also happens when using GDBserver
   under extended remote mode.

   This is in order for argument-expansion to occur.  E.g.,

   (gdb) run *

   The "*" gets expanded by the shell into a list of files.

   While this is a nice feature, it may be handy to bypass the shell
   in some cases.  To disable this feature, do "set startup-with-shell
   false".

   The catch-exec traps expected during start-up will be one more if
   the target is started up with a shell.  */
extern bool startup_with_shell;

/* Combine elements of ARGV into a single string, placing a single
   whitespace character between each element.  When ESCAPE_SHELL_CHAR is
   true then any special shell characters in elements of ARGV will be
   escaped.  When ESCAPE_SHELL_CHAR is false only the characters that GDB
   sees as special (quotes and whitespace) are escaped.  */
extern std::string
construct_inferior_arguments (gdb::array_view<char * const> argv,
			      bool escape_shell_char);

#endif /* GDBSUPPORT_COMMON_INFERIOR_H */
