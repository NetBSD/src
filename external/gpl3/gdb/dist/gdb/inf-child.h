/* Low level Unix child interface, for GDB when running under Unix.

   Copyright (C) 2004-2014 Free Software Foundation, Inc.

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

#endif
