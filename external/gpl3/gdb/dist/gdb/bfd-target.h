/* Very simple "bfd" target, for GDB, the GNU debugger.

   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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

#ifndef BFD_TARGET_H
#define BFD_TARGET_H

struct bfd;
struct target_ops;

/* Given an existing BFD, re-open it as a "struct target_ops".  This
   acquires a new reference to the BFD.  This reference will be
   released when the target is closed.  */
struct target_ops *target_bfd_reopen (struct bfd *bfd);

#endif
