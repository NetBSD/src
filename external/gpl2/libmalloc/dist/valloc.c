/*	$NetBSD: valloc.c,v 1.2 2016/01/13 21:56:38 christos Exp $	*/

/* Allocate memory on a page boundary.
   Copyright (C) 1991, 1992, 1993, 1994 Free Software Foundation, Inc.

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

#if defined (__GNU_LIBRARY__) || defined (_LIBC)
#include <stddef.h>
#include <unistd.h>
#include <sys/cdefs.h>
extern size_t __getpagesize __P ((void));
#else
#include "getpagesize.h"
#define	 __getpagesize()	getpagesize()
#endif

#ifndef	_MALLOC_INTERNAL
#define	_MALLOC_INTERNAL
#include <malloc.h>
#endif

static __malloc_size_t pagesize;

__ptr_t
valloc (size)
     __malloc_size_t size;
{
  if (pagesize == 0)
    pagesize = __getpagesize ();

  return memalign (pagesize, size);
}
