/*	$NetBSD: mcheck.c,v 1.1.1.1 2016/01/13 21:42:18 christos Exp $	*/

/* Standard debugging hooks for `malloc'.
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
#define	_MALLOC_INTERNAL
#include <malloc.h>
#include <stdio.h>
#endif

/* Old hook values.  */
static void (*old_free_hook) __P ((__ptr_t ptr));
static __ptr_t (*old_malloc_hook) __P ((__malloc_size_t size));
static __ptr_t (*old_realloc_hook) __P ((__ptr_t ptr, __malloc_size_t size));

/* Function to call when something awful happens.  */
static void (*abortfunc) __P ((enum mcheck_status));

/* Arbitrary magical numbers.  */
#define MAGICWORD	0xfedabeeb
#define MAGICFREE	0xd8675309
#define MAGICBYTE	((char) 0xd7)
#define MALLOCFLOOD	((char) 0x93)
#define FREEFLOOD	((char) 0x95)

struct hdr
  {
    __malloc_size_t size;		/* Exact size requested by user.  */
    unsigned long int magic;	/* Magic number to check header integrity.  */
  };

#if	defined(_LIBC) || defined(STDC_HEADERS) || defined(USG)
#define flood memset
#else
static void flood __P ((__ptr_t, int, __malloc_size_t));
static void
flood (ptr, val, size)
     __ptr_t ptr;
     int val;
     __malloc_size_t size;
{
  char *cp = ptr;
  while (size--)
    *cp++ = val;
}
#endif

static enum mcheck_status checkhdr __P ((const struct hdr *));
static enum mcheck_status
checkhdr (hdr)
     const struct hdr *hdr;
{
  enum mcheck_status status;
  switch (hdr->magic)
    {
    default:
      status = MCHECK_HEAD;
      break;
    case MAGICFREE:
      status = MCHECK_FREE;
      break;
    case MAGICWORD:
      if (((char *) &hdr[1])[hdr->size] != MAGICBYTE)
	status = MCHECK_TAIL;
      else
	status = MCHECK_OK;
      break;
    }
  if (status != MCHECK_OK)
    (*abortfunc) (status);
  return status;
}

static void freehook __P ((__ptr_t));
static void
freehook (ptr)
     __ptr_t ptr;
{
  struct hdr *hdr = ((struct hdr *) ptr) - 1;
  checkhdr (hdr);
  hdr->magic = MAGICFREE;
  flood (ptr, FREEFLOOD, hdr->size);
  __free_hook = old_free_hook;
  free (hdr);
  __free_hook = freehook;
}

static __ptr_t mallochook __P ((__malloc_size_t));
static __ptr_t
mallochook (size)
     __malloc_size_t size;
{
  struct hdr *hdr;

  __malloc_hook = old_malloc_hook;
  hdr = (struct hdr *) malloc (sizeof (struct hdr) + size + 1);
  __malloc_hook = mallochook;
  if (hdr == NULL)
    return NULL;

  hdr->size = size;
  hdr->magic = MAGICWORD;
  ((char *) &hdr[1])[size] = MAGICBYTE;
  flood ((__ptr_t) (hdr + 1), MALLOCFLOOD, size);
  return (__ptr_t) (hdr + 1);
}

static __ptr_t reallochook __P ((__ptr_t, __malloc_size_t));
static __ptr_t
reallochook (ptr, size)
     __ptr_t ptr;
     __malloc_size_t size;
{
  struct hdr *hdr = ((struct hdr *) ptr) - 1;
  __malloc_size_t osize = hdr->size;

  checkhdr (hdr);
  if (size < osize)
    flood ((char *) ptr + size, FREEFLOOD, osize - size);
  __free_hook = old_free_hook;
  __malloc_hook = old_malloc_hook;
  __realloc_hook = old_realloc_hook;
  hdr = (struct hdr *) realloc ((__ptr_t) hdr, sizeof (struct hdr) + size + 1);
  __free_hook = freehook;
  __malloc_hook = mallochook;
  __realloc_hook = reallochook;
  if (hdr == NULL)
    return NULL;

  hdr->size = size;
  hdr->magic = MAGICWORD;
  ((char *) &hdr[1])[size] = MAGICBYTE;
  if (size > osize)
    flood ((char *) (hdr + 1) + osize, MALLOCFLOOD, size - osize);
  return (__ptr_t) (hdr + 1);
}

static void
mabort (status)
     enum mcheck_status status;
{
  const char *msg;
  switch (status)
    {
    case MCHECK_OK:
      msg = "memory is consistent, library is buggy";
      break;
    case MCHECK_HEAD:
      msg = "memory clobbered before allocated block";
      break;
    case MCHECK_TAIL:
      msg = "memory clobbered past end of allocated block";
      break;
    case MCHECK_FREE:
      msg = "block freed twice";
      break;
    default:
      msg = "bogus mcheck_status, library is buggy";
      break;
    }
#ifdef __GNU_LIBRARY__
  __libc_fatal (msg);
#else
  fprintf (stderr, "mcheck: %s\n", msg);
  fflush (stderr);
  abort ();
#endif
}

static int mcheck_used = 0;

int
mcheck (func)
     void (*func) __P ((enum mcheck_status));
{
  abortfunc = (func != NULL) ? func : &mabort;

  /* These hooks may not be safely inserted if malloc is already in use.  */
  if (!__malloc_initialized && !mcheck_used)
    {
      old_free_hook = __free_hook;
      __free_hook = freehook;
      old_malloc_hook = __malloc_hook;
      __malloc_hook = mallochook;
      old_realloc_hook = __realloc_hook;
      __realloc_hook = reallochook;
      mcheck_used = 1;
    }

  return mcheck_used ? 0 : -1;
}

enum mcheck_status
mprobe (__ptr_t ptr)
{
  return mcheck_used ? checkhdr (ptr) : MCHECK_DISABLED;
}
