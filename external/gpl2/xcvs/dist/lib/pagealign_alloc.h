/* Memory allocation aligned to system page boundaries.

   Copyright (C) 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.  */

#ifndef _PAGEALIGN_ALLOC_H
# define _PAGEALIGN_ALLOC_H

# include <stddef.h>

/* Allocate a block of memory of SIZE bytes, aligned on a system page
   boundary.
   If SIZE is not a multiple of the system page size, it will be rounded up
   to the next multiple.
   Return a pointer to the start of the memory block. Upon allocation failure,
   return NULL and set errno.  */
extern void *pagealign_alloc (size_t size);

/* Like pagealign_alloc, except it exits the program if the allocation
   fails.  */
extern void *pagealign_xalloc (size_t size);

/* Free a memory block.
   PTR must be a non-NULL pointer returned by pagealign_alloc or
   pagealign_xalloc.  */
extern void pagealign_free (void *ptr);

#endif /* _PAGEALIGN_ALLOC_H */
