/*	$NetBSD: malloc-find.c,v 1.1.1.1 2016/01/13 21:42:18 christos Exp $	*/

/* Find the starting address of a malloc'd block, from anywhere inside it.
   Copyright (C) 1995 Free Software Foundation, Inc.

This file is part of the GNU C Library.  Its master source is NOT part of
the C library, however.  The master source lives in /gd/gnu/lib.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

#ifndef	_MALLOC_INTERNAL
#define _MALLOC_INTERNAL
#include <malloc.h>
#endif

/* Given an address in the middle of a malloc'd object,
   return the address of the beginning of the object.  */

__ptr_t
malloc_find_object_address (ptr)
     __ptr_t ptr;
{
  __malloc_size_t block = BLOCK (ptr);
  int type = _heapinfo[block].busy.type;

  if (type == 0)
    {
      /* The object is one or more entire blocks.  */
      __malloc_ptrdiff_t sizevalue = _heapinfo[block].busy.info.size;

      if (sizevalue < 0)
	/* This is one of the blocks after the first.  SIZEVALUE
	   says how many blocks to go back to find the first.  */
	block += sizevalue;

      /* BLOCK is now the first block of the object.
	 Its start is the start of the object.  */
      return ADDRESS (block);
    }
  else
    {
      /* Get the size of fragments in this block.  */
      __malloc_size_t size = 1 << type;

      /* Turn off the low bits to find the start address of the fragment.  */
      return _heapbase + (((char *) ptr - _heapbase) & ~(size - 1));
    }
}
