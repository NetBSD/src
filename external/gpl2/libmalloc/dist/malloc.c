/*	$NetBSD: malloc.c,v 1.1.1.1 2016/01/13 21:42:18 christos Exp $	*/

/* Memory allocator `malloc'.
   Copyright 1990, 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.
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

/* How to really get more memory.  */
__ptr_t (*__morecore) __P ((ptrdiff_t __size)) = __default_morecore;

/* Debugging hook for `malloc'.  */
__ptr_t (*__malloc_hook) __P ((__malloc_size_t __size));

/* Pointer to the base of the first block.  */
char *_heapbase;

/* Block information table.  Allocated with align/__free (not malloc/free).  */
malloc_info *_heapinfo;

/* Number of info entries.  */
static __malloc_size_t heapsize;

/* Search index in the info table.  */
__malloc_size_t _heapindex;

/* Limit of valid info table indices.  */
__malloc_size_t _heaplimit;

/* Free lists for each fragment size.  */
struct list _fraghead[BLOCKLOG];

/* Instrumentation.  */
__malloc_size_t _chunks_used;
__malloc_size_t _bytes_used;
__malloc_size_t _chunks_free;
__malloc_size_t _bytes_free;

/* Are you experienced?  */
int __malloc_initialized;

void (*__malloc_initialize_hook) __P ((void));
void (*__after_morecore_hook) __P ((void));

/* Aligned allocation.  */
static __ptr_t align __P ((__malloc_size_t));
static __ptr_t
align (size)
     __malloc_size_t size;
{
  __ptr_t result;
  unsigned long int adj;

  result = (*__morecore) (size);
  adj = (unsigned long int) ((unsigned long int) ((char *) result -
						  (char *) NULL)) % BLOCKSIZE;
  if (adj != 0)
    {
      adj = BLOCKSIZE - adj;
      (void) (*__morecore) (adj);
      result = (char *) result + adj;
    }

  if (__after_morecore_hook)
    (*__after_morecore_hook) ();

  return result;
}

/* Set everything up and remember that we have.  */
static int initialize __P ((void));
static int
initialize ()
{
  if (__malloc_initialize_hook)
    (*__malloc_initialize_hook) ();

  heapsize = HEAP / BLOCKSIZE;
  _heapinfo = (malloc_info *) align (heapsize * sizeof (malloc_info));
  if (_heapinfo == NULL)
    return 0;
  memset (_heapinfo, 0, heapsize * sizeof (malloc_info));
  _heapinfo[0].free.size = 0;
  _heapinfo[0].free.next = _heapinfo[0].free.prev = 0;
  _heapindex = 0;
  _heapbase = (char *) _heapinfo;

  /* Account for the _heapinfo block itself in the statistics.  */
  _bytes_used = heapsize * sizeof (malloc_info);
  _chunks_used = 1;

  __malloc_initialized = 1;
  return 1;
}

/* Get neatly aligned memory, initializing or
   growing the heap info table as necessary. */
static __ptr_t morecore __P ((__malloc_size_t));
static __ptr_t
morecore (size)
     __malloc_size_t size;
{
  __ptr_t result;
  malloc_info *newinfo, *oldinfo;
  __malloc_size_t newsize;

  result = align (size);
  if (result == NULL)
    return NULL;

  /* Check if we need to grow the info table.  */
  if ((__malloc_size_t) BLOCK ((char *) result + size) > heapsize)
    {
      newsize = heapsize;
      while ((__malloc_size_t) BLOCK ((char *) result + size) > newsize)
	newsize *= 2;
      newinfo = (malloc_info *) align (newsize * sizeof (malloc_info));
      if (newinfo == NULL)
	{
	  (*__morecore) (-size);
	  return NULL;
	}
      memcpy (newinfo, _heapinfo, heapsize * sizeof (malloc_info));
      memset (&newinfo[heapsize], 0,
	      (newsize - heapsize) * sizeof (malloc_info));
      oldinfo = _heapinfo;
      newinfo[BLOCK (oldinfo)].busy.type = 0;
      newinfo[BLOCK (oldinfo)].busy.info.size
	= BLOCKIFY (heapsize * sizeof (malloc_info));
      _heapinfo = newinfo;
      /* Account for the _heapinfo block itself in the statistics.  */
      _bytes_used += newsize * sizeof (malloc_info);
      ++_chunks_used;
      _free_internal (oldinfo);
      heapsize = newsize;
    }

  _heaplimit = BLOCK ((char *) result + size);
  return result;
}

/* Allocate memory from the heap.  */
__ptr_t
malloc (size)
     __malloc_size_t size;
{
  __ptr_t result;
  __malloc_size_t block, blocks, lastblocks, start;
  register __malloc_size_t i;
  struct list *next;

  /* ANSI C allows `malloc (0)' to either return NULL, or to return a
     valid address you can realloc and free (though not dereference).

     It turns out that some extant code (sunrpc, at least Ultrix's version)
     expects `malloc (0)' to return non-NULL and breaks otherwise.
     Be compatible.  */

#if	0
  if (size == 0)
    return NULL;
#endif

  if (__malloc_hook != NULL)
    return (*__malloc_hook) (size);

  if (!__malloc_initialized)
    if (!initialize ())
      return NULL;

  if (size < sizeof (struct list))
    size = sizeof (struct list);

#ifdef SUNOS_LOCALTIME_BUG
  if (size < 16)
    size = 16;
#endif

  /* Determine the allocation policy based on the request size.  */
  if (size <= BLOCKSIZE / 2)
    {
      /* Small allocation to receive a fragment of a block.
	 Determine the logarithm to base two of the fragment size. */
      register __malloc_size_t log = 1;
      --size;
      while ((size /= 2) != 0)
	++log;

      /* Look in the fragment lists for a
	 free fragment of the desired size. */
      next = _fraghead[log].next;
      if (next != NULL)
	{
	  /* There are free fragments of this size.
	     Pop a fragment out of the fragment list and return it.
	     Update the block's nfree and first counters. */
	  result = (__ptr_t) next;
	  next->prev->next = next->next;
	  if (next->next != NULL)
	    next->next->prev = next->prev;
	  block = BLOCK (result);
	  if (--_heapinfo[block].busy.info.frag.nfree != 0)
	    _heapinfo[block].busy.info.frag.first = (unsigned long int)
	      ((unsigned long int) ((char *) next->next - (char *) NULL)
	       % BLOCKSIZE) >> log;

	  /* Update the statistics.  */
	  ++_chunks_used;
	  _bytes_used += 1 << log;
	  --_chunks_free;
	  _bytes_free -= 1 << log;
	}
      else
	{
	  /* No free fragments of the desired size, so get a new block
	     and break it into fragments, returning the first.  */
	  result = malloc (BLOCKSIZE);
	  if (result == NULL)
	    return NULL;

	  /* Link all fragments but the first into the free list.  */
	  for (i = 1; i < (__malloc_size_t) (BLOCKSIZE >> log); ++i)
	    {
	      next = (struct list *) ((char *) result + (i << log));
	      next->next = _fraghead[log].next;
	      next->prev = &_fraghead[log];
	      next->prev->next = next;
	      if (next->next != NULL)
		next->next->prev = next;
	    }

	  /* Initialize the nfree and first counters for this block.  */
	  block = BLOCK (result);
	  _heapinfo[block].busy.type = log;
	  _heapinfo[block].busy.info.frag.nfree = i - 1;
	  _heapinfo[block].busy.info.frag.first = i - 1;

	  _chunks_free += (BLOCKSIZE >> log) - 1;
	  _bytes_free += BLOCKSIZE - (1 << log);
	  _bytes_used -= BLOCKSIZE - (1 << log);
	}
    }
  else
    {
      /* Large allocation to receive one or more blocks.
	 Search the free list in a circle starting at the last place visited.
	 If we loop completely around without finding a large enough
	 space we will have to get more memory from the system.  */
      blocks = BLOCKIFY (size);
      start = block = _heapindex;
      while (_heapinfo[block].free.size < blocks)
	{
	  block = _heapinfo[block].free.next;
	  if (block == start)
	    {
	      /* Need to get more from the system.  Check to see if
		 the new core will be contiguous with the final free
		 block; if so we don't need to get as much.  */
	      block = _heapinfo[0].free.prev;
	      lastblocks = _heapinfo[block].free.size;
	      if (_heaplimit != 0 && block + lastblocks == _heaplimit &&
		  (*__morecore) (0) == ADDRESS (block + lastblocks) &&
		  (morecore ((blocks - lastblocks) * BLOCKSIZE)) != NULL)
		{
 		  /* Which block we are extending (the `final free
 		     block' referred to above) might have changed, if
 		     it got combined with a freed info table.  */
 		  block = _heapinfo[0].free.prev;
  		  _heapinfo[block].free.size += (blocks - lastblocks);
		  _bytes_free += (blocks - lastblocks) * BLOCKSIZE;
		  continue;
		}
	      result = morecore (blocks * BLOCKSIZE);
	      if (result == NULL)
		return NULL;
	      block = BLOCK (result);
	      _heapinfo[block].busy.type = 0;
	      _heapinfo[block].busy.info.size = blocks;
	      ++_chunks_used;
	      _bytes_used += blocks * BLOCKSIZE;
	      return result;
	    }
	}

      /* At this point we have found a suitable free list entry.
	 Figure out how to remove what we need from the list. */
      result = ADDRESS (block);
      if (_heapinfo[block].free.size > blocks)
	{
	  /* The block we found has a bit left over,
	     so relink the tail end back into the free list. */
	  _heapinfo[block + blocks].free.size
	    = _heapinfo[block].free.size - blocks;
	  _heapinfo[block + blocks].free.next
	    = _heapinfo[block].free.next;
	  _heapinfo[block + blocks].free.prev
	    = _heapinfo[block].free.prev;
	  _heapinfo[_heapinfo[block].free.prev].free.next
	    = _heapinfo[_heapinfo[block].free.next].free.prev
	    = _heapindex = block + blocks;
	}
      else
	{
	  /* The block exactly matches our requirements,
	     so just remove it from the list. */
	  _heapinfo[_heapinfo[block].free.next].free.prev
	    = _heapinfo[block].free.prev;
	  _heapinfo[_heapinfo[block].free.prev].free.next
	    = _heapindex = _heapinfo[block].free.next;
	  --_chunks_free;
	}

      _heapinfo[block].busy.type = 0;
      _heapinfo[block].busy.info.size = blocks;
      ++_chunks_used;
      _bytes_used += blocks * BLOCKSIZE;
      _bytes_free -= blocks * BLOCKSIZE;

      /* Mark all the blocks of the object just allocated except for the
	 first with a negative number so you can find the first block by
	 adding that adjustment.  */
      while (--blocks > 0)
	_heapinfo[block + blocks].busy.info.size = -blocks;
    }

  return result;
}

#ifndef _LIBC

/* On some ANSI C systems, some libc functions call _malloc, _free
   and _realloc.  Make them use the GNU functions.  */

__ptr_t
_malloc (size)
     __malloc_size_t size;
{
  return malloc (size);
}

void
_free (ptr)
     __ptr_t ptr;
{
  free (ptr);
}

__ptr_t
_realloc (ptr, size)
     __ptr_t ptr;
     __malloc_size_t size;
{
  return realloc (ptr, size);
}

#endif
