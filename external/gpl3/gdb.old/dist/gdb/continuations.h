/* Continuations for GDB, the GNU debugger.

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

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

#ifndef CONTINUATIONS_H
#define CONTINUATIONS_H

struct inferior;

/* To continue the execution commands when running gdb asynchronously.
   A continuation structure contains a pointer to a function to be called
   to finish the command, once the target has stopped.  Such mechanism is
   used by the finish and until commands, and in the remote protocol
   when opening an extended-remote connection.  */

/* Prototype of the continuation callback functions.  ARG is the
   continuation argument registered in the corresponding
   add_*_continuation call.  ERR is true when the command should be
   cancelled instead of finished normally.  In that case, the
   continuation should clean up whatever state had been set up for the
   command in question (e.g., remove momentary breakpoints).  This
   happens e.g., when an error was thrown while handling a target
   event, or when the inferior thread the command was being executed
   on exits.  */
typedef void (continuation_ftype) (void *arg, int err);

/* Prototype of the function responsible for releasing the argument
   passed to the continuation callback functions, either when the
   continuation is called, or discarded.  */
typedef void (continuation_free_arg_ftype) (void *);

/* Inferior specific (any thread) continuations.  */

extern void add_inferior_continuation (continuation_ftype *,
				       void *,
				       continuation_free_arg_ftype *);
extern void do_all_inferior_continuations (int err);
extern void discard_all_inferior_continuations (struct inferior *inf);

#endif
