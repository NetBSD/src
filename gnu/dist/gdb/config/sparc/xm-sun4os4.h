/* Macro definitions for running gdb on a Sun 4 running sunos 4.
   Copyright (C) 1989, 1993, 1994, 1995, 1996
   Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "sparc/xm-sparc.h"

#define FPU

/* If you expect to use the mmalloc package to obtain mapped symbol files,
   for now you have to specify some parameters that determine how gdb places
   the mappings in it's address space.  See the comments in map_to_address()
   for details.  This is expected to only be a short term solution.  Yes it
   is a kludge.
   FIXME:  Make this more automatic. */

#define MMAP_BASE_ADDRESS	0xE0000000	/* First mapping here */
#define MMAP_INCREMENT		0x01000000	/* Increment to next mapping */

/* /usr/include/malloc.h is included by vx-share/xdr_ld, and might
   declare these using char * not void *.  The following should work with
   acc, gcc, or /bin/cc.  */

#define MALLOC_INCOMPATIBLE
#include <malloc.h>

/* SunOS 4.x uses nonstandard "char *" as type of third argument to ptrace() */

#define PTRACE_ARG3_TYPE char*
