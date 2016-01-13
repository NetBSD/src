/*	$NetBSD: realloc.c,v 1.1.1.1 2016/01/13 21:42:18 christos Exp $	*/

/* Change the size of a block allocated by `malloc'.
   Copyright 1990, 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
		     Written May 1989 by Mike Haertel.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.

   The author may be reached (Email) at the address mike@ai.mit.edu,
   or (US mail) as Mike Haertel c/o Free Software Foundation.  */

#ifndef	_MALLOC_INTERNAL
#define _MALLOC_INTERNAL
#include <malloc.h>
#endif

#if  (defined (MEMMOVE_MISSING) || \
      !defined(_LIBC) && !defined(STDC_HEADERS) && !defined(USG))

/* Snarfed directly from Emacs src/dispnew.c:
   XXX Should use system bcopy if it handles overlap.  */
#ifndef emacs

/* Like bcopy except never gets confused by overlap.  */

static void
safe_bcopy (from, to, size)
     char *from, *to;
     int size;
{
  if (size <= 0 || from == to)
    return;

  /* If the source and destination don't overlap, then bcopy can
     handle it.  If they do overlap, but the destination is lower in
     memory than the source, we'll assume bcopy can handle that.  */
  if (to < from || from + size <= to)
    bcopy (from, to, size);

  /* Otherwise, we'll copy from the end.  */
  else
    {
      register char *endf = from + size;
      register char *endt = to + size;

      /* If TO - FROM is large, then we should break the copy into
	 nonoverlapping chunks of TO - FROM bytes each.  However, if
	 TO - FROM is small, then the bcopy function call overhead
	 makes this not worth it.  The crossover point could be about
	 anywhere.  Since I don't think the obvious copy loop is too
	 bad, I'm trying to err in its favor.  */
      if (to - from < 64)
	{
	  do
	    *--endt = *--endf;
	  while (endf != from);
	}
      else
	{
	  for (;;)
	    {
	      endt -= (to - from);
	      endf -= (to - from);

	      if (endt < to)
		break;

	      bcopy (endf, endt, to - from);
	    }

	  /* If SIZE wasn't a multiple of TO - FROM, there will be a
	     little left over.  The amount left over is
	     (endt + (to - from)) - to, which is endt - from.  */
	  bcopy (from, to, endt - from);
	}
    }
}     
#endif	/* Not emacs.  */

#define memmove(to, from, size) safe_bcopy ((from), (to), (size))

#endif


#define min(A, B) ((A) < (B) ? (A) : (B))

/* Debugging hook for realloc.  */
__ptr_t (*__realloc_hook) __P ((__ptr_t __ptr, __malloc_size_t __size));

/* Resize the given region to the new size, returning a pointer
   to the (possibly moved) region.  This is optimized for speed;
   some benchmarks seem to indicate that greater compactness is
   achieved by unconditionally allocating and copying to a
   new region.  This module has incestuous knowledge of the
   internals of both free and malloc. */
__ptr_t
realloc (ptr, size)
     __ptr_t ptr;
     __malloc_size_t size;
{
  __ptr_t result;
  int type;
  __malloc_size_t block, blocks, oldlimit;

  if (size == 0)
    {
      free (ptr);
      return malloc (0);
    }
  else if (ptr == NULL)
    return malloc (size);

  if (__realloc_hook != NULL)
    return (*__realloc_hook) (ptr, size);

  block = BLOCK (ptr);

  type = _heapinfo[block].busy.type;
  switch (type)
    {
    case 0:
      /* Maybe reallocate a large block to a small fragment.  */
      if (size <= BLOCKSIZE / 2)
	{
	  result = malloc (size);
	  if (result != NULL)
	    {
	      memcpy (result, ptr, size);
	      _free_internal (ptr);
	      return result;
	    }
	}

      /* The new size is a large allocation as well;
	 see if we can hold it in place. */
      blocks = BLOCKIFY (size);
      if (blocks < _heapinfo[block].busy.info.size)
	{
	  /* The new size is smaller; return
	     excess memory to the free list. */
	  _heapinfo[block + blocks].busy.type = 0;
	  _heapinfo[block + blocks].busy.info.size
	    = _heapinfo[block].busy.info.size - blocks;
	  _heapinfo[block].busy.info.size = blocks;
	  /* We have just created a new chunk by splitting a chunk in two.
	     Now we will free this chunk; increment the statistics counter
	     so it doesn't become wrong when _free_internal decrements it.  */
	  ++_chunks_used;
	  _free_internal (ADDRESS (block + blocks));
	  result = ptr;
	}
      else if (blocks == _heapinfo[block].busy.info.size)
	/* No size change necessary.  */
	result = ptr;
      else
	{
	  /* Won't fit, so allocate a new region that will.
	     Free the old region first in case there is sufficient
	     adjacent free space to grow without moving. */
	  blocks = _heapinfo[block].busy.info.size;
	  /* Prevent free from actually returning memory to the system.  */
	  oldlimit = _heaplimit;
	  _heaplimit = 0;
	  _free_internal (ptr);
	  _heaplimit = oldlimit;
	  result = malloc (size);
	  if (result == NULL)
	    {
	      /* Now we're really in trouble.  We have to unfree
		 the thing we just freed.  Unfortunately it might
		 have been coalesced with its neighbors.  */
	      if (_heapindex == block)
	        (void) malloc (blocks * BLOCKSIZE);
	      else
		{
		  __ptr_t previous = malloc ((block - _heapindex) * BLOCKSIZE);
		  (void) malloc (blocks * BLOCKSIZE);
		  _free_internal (previous);
		}
	      return NULL;
	    }
	  if (ptr != result)
	    memmove (result, ptr, blocks * BLOCKSIZE);
	}
      break;

    default:
      /* Old size is a fragment; type is logarithm
	 to base two of the fragment size.  */
      if (size > (__malloc_size_t) (1 << (type - 1)) &&
	  size <= (__malloc_size_t) (1 << type))
	/* The new size is the same kind of fragment.  */
	result = ptr;
      else
	{
	  /* The new size is different; allocate a new space,
	     and copy the lesser of the new size and the old. */
	  result = malloc (size);
	  if (result == NULL)
	    return NULL;
	  memcpy (result, ptr, min (size, (__malloc_size_t) 1 << type));
	  free (ptr);
	}
      break;
    }

  return result;
}
