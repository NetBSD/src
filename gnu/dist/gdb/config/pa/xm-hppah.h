/* Parameters for hosting on an HPPA-RISC machine running HPUX, for GDB.
   Copyright 1991, 1992 Free Software Foundation, Inc. 

   Contributed by the Center for Software Science at the
   University of Utah (pa-gdb-bugs@cs.utah.edu).

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

/* Host is big-endian. */
#define	HOST_BYTE_ORDER	BIG_ENDIAN

#include "pa/xm-pa.h"

/* Avoid "INT_MIN redefined" warnings -- by defining it here, exactly
   the same as in the system <machine/machtypes.h> file.  */
#undef  INT_MIN
#define INT_MIN         0x80000000

#define USG

#ifndef __STDC__
/* This define is discussed in decode_line_1 in symtab.c  */
#define HPPA_COMPILER_BUG
#endif

#define HAVE_TERMIOS

/* HP defines malloc and realloc as returning void *, even for non-ANSI
   compilations (such as with the native compiler). */

#define MALLOC_INCOMPATIBLE

/* If you expect to use the mmalloc package to obtain mapped symbol files,
   for now you have to specify some parameters that determine how gdb places
   the mappings in it's address space.  See the comments in map_to_address()
   for details.  This is expected to only be a short term solution.  Yes it
   is a kludge.
   FIXME:  Make this more automatic. */

#define MMAP_BASE_ADDRESS	0xA0000000	/* First mapping here */
#define MMAP_INCREMENT		0x01000000	/* Increment to next mapping */

/* On hpux, autoconf 2.4 (and possibly others) does not properly detect that
   mmap is available.  Until this is fixed, we have to explicitly force 
   HAVE_MMAP.  -fnf */

#define HAVE_MMAP 1

extern void *
malloc PARAMS ((size_t));

extern void *
realloc PARAMS ((void *, size_t));

extern void
free PARAMS ((void *));
