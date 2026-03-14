/* Native-dependent code for GNU/Linux i386.

   Copyright (C) 2024-2025 Free Software Foundation, Inc.

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

#ifndef GDB_NAT_I386_LINUX_H
#define GDB_NAT_I386_LINUX_H

/* Does the current host support the GETFPXREGS request?  The system header
   file may or may not define it, but even if it is defined, the kernel
   will return EIO if it's running on a pre-SSE processor.

   Initially this will be TRIBOOL_UNKNOWN and should be changed to
   TRIBOOL_FALSE if the ptrace call is attempted and fails or changed to
   TRIBOOL_TRUE if the ptrace call is attempted and succeeds.

   My instinct is to attach this to some architecture- or target-specific
   data structure, but really, a particular GDB process can only run on top
   of one kernel at a time.  So it's okay - for this to be a global
   variable.  */
extern tribool have_ptrace_getfpxregs;

#endif /* GDB_NAT_I386_LINUX_H */
