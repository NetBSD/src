/* Base/prototype target for default child (native) targets.

   Copyright (C) 2004-2016 Free Software Foundation, Inc.

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

#ifndef INF_CHILD_H
#define INF_CHILD_H

/* Create a prototype child target.  The client can override it with
   local methods.  */

extern struct target_ops *inf_child_target (void);

/* Functions for helping to write a native target.  */

/* This is for native targets which use a unix/POSIX-style waitstatus.  */
extern void store_waitstatus (struct target_waitstatus *, int);

/* This is to be called by the native target's open routine to push
   the target, in case it need to override to_open.  */

extern void inf_child_open_target (struct target_ops *target,
				   const char *arg, int from_tty);

/* To be called by the native target's to_mourn_inferior routine.  */

extern void inf_child_mourn_inferior (struct target_ops *ops);

/* Unpush the target if it wasn't explicitly open with "target native"
   and there are no live inferiors left.  Note: if calling this as a
   result of a mourn or detach, the current inferior shall already
   have its PID cleared, so it isn't counted as live.  That's usually
   done by calling either generic_mourn_inferior or
   detach_inferior.  */

extern void inf_child_maybe_unpush_target (struct target_ops *ops);

#endif
