/* CLI Definitions for GDB, the GNU debugger.

   Copyright (C) 2016 Free Software Foundation, Inc.

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

#ifndef CLI_INTERP_H
#define CLI_INTERP_H 1

struct interp;

extern int cli_interpreter_supports_command_editing (struct interp *interp);

extern void cli_interpreter_pre_command_loop (struct interp *self);

/* Returns true if the current stop should be printed to
   CONSOLE_INTERP.  */
extern int should_print_stop_to_console (struct interp *interp,
					 struct thread_info *tp);

#endif
