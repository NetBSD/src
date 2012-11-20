/* BFD library -- caching of file descriptors.

   Copyright 1990, 1991, 1992, 1993, 1994, 1996, 2000, 2001, 2002,
   2003, 2004 Free Software Foundation, Inc.

   Hacked by Steve Chamberlain of Cygnus Support (steve@cygnus.com).

This file is part of BFD, the Binary File Descriptor library.

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

/*
SECTION
	File caching

	The file caching mechanism is embedded within BFD and allows
	the application to open as many BFDs as it wants without
	regard to the underlying operating system's file descriptor
	limit (often as low as 20 open files).  The module in
	<<cache.c>> maintains a least recently used list of
	<<BFD_CACHE_MAX_OPEN>> files, and exports the name
	<<bfd_cache_lookup>>, which runs around and makes sure that
	the required BFD is open. If not, then it chooses a file to
	close, closes it and opens the one wanted, returning its file
	handle.

*/

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"
#include "libiberty.h"

static bfd_boolean bfd_cache_delete (bfd *);


static file_ptr
cache_btell (struct bfd *abfd)
{
  return real_ftell (bfd_cache_lookup (abfd));
}

static int
cache_bseek (struct bfd *abfd, file_ptr offset, int whence)
{
  return real_fseek (bfd_cache_lookup (abfd), offset, whence);
}

/* Note that archive entries don't have streams; they share their parent's.
   This allows someone to play with the iostream behind BFD's back.

   Also, note that the origin pointer points to the beginning of a file's
   contents (0 for non-archive elements).  For archive entries this is the
   first octet in the file, NOT the beginning of the archive header.  */

static file_ptr
cache_bread (struct bfd *abfd, void *buf, file_ptr nbytes)
{
  file_ptr nread;
  /* FIXME - this looks like an optimization, but it's really to cover
     up for a feature of some OSs (not solaris - sigh) that
     ld/pe-dll.c takes advantage of (apparently) when it creates BFDs
     internally and tries to link against them.  BFD seems to be smart
     enough to realize there are no symbol records in the "file" that
     doesn't exist but attempts to read them anyway.  On Solaris,
     attempting to read zero bytes from a NULL file results in a core
     dump, but on other platforms it just returns zero bytes read.
     This makes it to something reasonable. - DJ */
  if (nbytes == 0)
    return 0;

#if defined (__VAX) && defined (VMS)
  /* Apparently fread on Vax VMS does not keep the record length
     information.  */
  nread = read (fileno (bfd_cache_lookup (abfd)), buf, nbytes);
  /* Set bfd_error if we did not read as much data as we expected.  If
     the read failed due to an error set the bfd_error_system_call,
     else set bfd_error_file_truncated.  */
  if (nread == (file_ptr)-1)
    {
      bfd_set_error (bfd_error_system_call);
      return -1;
    }
#else
  nread = fread (buf, 1, nbytes, bfd_cache_lookup (abfd));
  /* Set bfd_error if we did not read as much data as we expected.  If
     the read failed due to an error set the bfd_error_system_call,
     else set bfd_error_file_truncated.  */
  if (nread < nbytes && ferror (bfd_cache_lookup (abfd)))
    {
      bfd_set_error (bfd_error_system_call);
      return -1;
    }
#endif
  return nread;
}

static file_ptr
cache_bwrite (struct bfd *abfd, const void *where, file_ptr nbytes)
{
  file_ptr nwrite = fwrite (where, 1, nbytes, bfd_cache_lookup (abfd));
  if (nwrite < nbytes && ferror (bfd_cache_lookup (abfd)))
    {
      bfd_set_error (bfd_error_system_call);
      return -1;
    }
  return nwrite;
}

static int
cache_bclose (struct bfd *abfd)
{
  return bfd_cache_close (abfd);
}

static int
cache_bflush (struct bfd *abfd)
{
  int sts = fflush (bfd_cache_lookup (abfd));
  if (sts < 0)
    bfd_set_error (bfd_error_system_call);
  return sts;
}

static int
cache_bstat (struct bfd *abfd, struct stat *sb)
{
  int sts = fstat (fileno (bfd_cache_lookup (abfd)), sb);
  if (sts < 0)
    bfd_set_error (bfd_error_system_call);
  return sts;
}

static const struct bfd_iovec cache_iovec = {
  &cache_bread, &cache_bwrite, &cache_btell, &cache_bseek,
  &cache_bclose, &cache_bflush, &cache_bstat
};

/*
INTERNAL_FUNCTION
	BFD_CACHE_MAX_OPEN macro

DESCRIPTION
	The maximum number of files which the cache will keep open at
	one time.

.#define BFD_CACHE_MAX_OPEN 10

*/

/* The number of BFD files we have open.  */

static int open_files;

/*
INTERNAL_FUNCTION
	bfd_last_cache

SYNOPSIS
	extern bfd *bfd_last_cache;

DESCRIPTION
	Zero, or a pointer to the topmost BFD on the chain.  This is
	used by the <<bfd_cache_lookup>> macro in @file{libbfd.h} to
	determine when it can avoid a function call.
*/

bfd *bfd_last_cache;

/*
  INTERNAL_FUNCTION
  	bfd_cache_lookup

  DESCRIPTION
 	Check to see if the required BFD is the same as the last one
 	looked up. If so, then it can use the stream in the BFD with
 	impunity, since it can't have changed since the last lookup;
 	otherwise, it has to perform the complicated lookup function.

  .#define bfd_cache_lookup(x) \
  .    ((x) == bfd_last_cache ? \
  .      (FILE *) (bfd_last_cache->iostream): \
  .       bfd_cache_lookup_worker (x))

 */

/* Insert a BFD into the cache.  */

static void
insert (bfd *abfd)
{
  if (bfd_last_cache == NULL)
    {
      abfd->lru_next = abfd;
      abfd->lru_prev = abfd;
    }
  else
    {
      abfd->lru_next = bfd_last_cache;
      abfd->lru_prev = bfd_last_cache->lru_prev;
      abfd->lru_prev->lru_next = abfd;
      abfd->lru_next->lru_prev = abfd;
    }
  bfd_last_cache = abfd;
}

/* Remove a BFD from the cache.  */

static void
snip (bfd *abfd)
{
  abfd->lru_prev->lru_next = abfd->lru_next;
  abfd->lru_next->lru_prev = abfd->lru_prev;
  if (abfd == bfd_last_cache)
    {
      bfd_last_cache = abfd->lru_next;
      if (abfd == bfd_last_cache)
	bfd_last_cache = NULL;
    }
}

/* We need to open a new file, and the cache is full.  Find the least
   recently used cacheable BFD and close it.  */

static bfd_boolean
close_one (void)
{
  register bfd *kill;

  if (bfd_last_cache == NULL)
    kill = NULL;
  else
    {
      for (kill = bfd_last_cache->lru_prev;
	   ! kill->cacheable;
	   kill = kill->lru_prev)
	{
	  if (kill == bfd_last_cache)
	    {
	      kill = NULL;
	      break;
	    }
	}
    }

  if (kill == NULL)
    {
      /* There are no open cacheable BFD's.  */
      return TRUE;
    }

  kill->where = real_ftell ((FILE *) kill->iostream);

  return bfd_cache_delete (kill);
}

/* Close a BFD and remove it from the cache.  */

static bfd_boolean
bfd_cache_delete (bfd *abfd)
{
  bfd_boolean ret;

  if (fclose ((FILE *) abfd->iostream) == 0)
    ret = TRUE;
  else
    {
      ret = FALSE;
      bfd_set_error (bfd_error_system_call);
    }

  snip (abfd);

  abfd->iostream = NULL;
  --open_files;

  return ret;
}

/*
INTERNAL_FUNCTION
	bfd_cache_init

SYNOPSIS
	bfd_boolean bfd_cache_init (bfd *abfd);

DESCRIPTION
	Add a newly opened BFD to the cache.
*/

bfd_boolean
bfd_cache_init (bfd *abfd)
{
  BFD_ASSERT (abfd->iostream != NULL);
  if (open_files >= BFD_CACHE_MAX_OPEN)
    {
      if (! close_one ())
	return FALSE;
    }
  abfd->iovec = &cache_iovec;
  insert (abfd);
  ++open_files;
  return TRUE;
}

/*
INTERNAL_FUNCTION
	bfd_cache_close

SYNOPSIS
	bfd_boolean bfd_cache_close (bfd *abfd);

DESCRIPTION
	Remove the BFD @var{abfd} from the cache. If the attached file is open,
	then close it too.

RETURNS
	<<FALSE>> is returned if closing the file fails, <<TRUE>> is
	returned if all is well.
*/

bfd_boolean
bfd_cache_close (bfd *abfd)
{
  if (abfd->iovec != &cache_iovec)
    return TRUE;

  if (abfd->iostream == NULL)
    /* Previously closed.  */
    return TRUE;

  return bfd_cache_delete (abfd);
}

/*
FUNCTION
	bfd_cache_close_all

SYNOPSIS
	bfd_boolean bfd_cache_close_all (void);

DESCRIPTION
	Remove all BFDs from the cache. If the attached file is open,
	then close it too.

RETURNS
	<<FALSE>> is returned if closing one of the file fails, <<TRUE>> is
	returned if all is well.
*/

bfd_boolean
bfd_cache_close_all ()
{
  bfd_boolean ret = TRUE;

  while (bfd_last_cache != NULL)
    ret &= bfd_cache_close (bfd_last_cache);

  return ret;
}

/*
INTERNAL_FUNCTION
	bfd_open_file

SYNOPSIS
	FILE* bfd_open_file (bfd *abfd);

DESCRIPTION
	Call the OS to open a file for @var{abfd}.  Return the <<FILE *>>
	(possibly <<NULL>>) that results from this operation.  Set up the
	BFD so that future accesses know the file is open. If the <<FILE *>>
	returned is <<NULL>>, then it won't have been put in the
	cache, so it won't have to be removed from it.
*/

FILE *
bfd_open_file (bfd *abfd)
{
  abfd->cacheable = TRUE;	/* Allow it to be closed later.  */

  if (open_files >= BFD_CACHE_MAX_OPEN)
    {
      if (! close_one ())
	return NULL;
    }

  switch (abfd->direction)
    {
    case read_direction:
    case no_direction:
      abfd->iostream = (PTR) fopen (abfd->filename, FOPEN_RB);
      break;
    case both_direction:
    case write_direction:
      if (abfd->opened_once)
	{
	  abfd->iostream = (PTR) fopen (abfd->filename, FOPEN_RUB);
	  if (abfd->iostream == NULL)
	    abfd->iostream = (PTR) fopen (abfd->filename, FOPEN_WUB);
	}
      else
	{
	  /* Create the file.

	     Some operating systems won't let us overwrite a running
	     binary.  For them, we want to unlink the file first.

	     However, gcc 2.95 will create temporary files using
	     O_EXCL and tight permissions to prevent other users from
	     substituting other .o files during the compilation.  gcc
	     will then tell the assembler to use the newly created
	     file as an output file.  If we unlink the file here, we
	     open a brief window when another user could still
	     substitute a file.

	     So we unlink the output file if and only if it has
	     non-zero size.  */
#ifndef __MSDOS__
	  /* Don't do this for MSDOS: it doesn't care about overwriting
	     a running binary, but if this file is already open by
	     another BFD, we will be in deep trouble if we delete an
	     open file.  In fact, objdump does just that if invoked with
	     the --info option.  */
	  struct stat s;

	  if (stat (abfd->filename, &s) == 0 && s.st_size != 0)
	    unlink_if_ordinary (abfd->filename);
#endif
	  abfd->iostream = (PTR) fopen (abfd->filename, FOPEN_WUB);
	  abfd->opened_once = TRUE;
	}
      break;
    }

  if (abfd->iostream != NULL)
    {
      if (! bfd_cache_init (abfd))
	return NULL;
    }

  return (FILE *) abfd->iostream;
}

/*
INTERNAL_FUNCTION
	bfd_cache_lookup_worker

SYNOPSIS
	FILE *bfd_cache_lookup_worker (bfd *abfd);

DESCRIPTION
	Called when the macro <<bfd_cache_lookup>> fails to find a
	quick answer.  Find a file descriptor for @var{abfd}.  If
	necessary, it open it.  If there are already more than
	<<BFD_CACHE_MAX_OPEN>> files open, it tries to close one first, to
	avoid running out of file descriptors.  It will abort rather than
	returning NULL if it is unable to (re)open the @var{abfd}.
*/

FILE *
bfd_cache_lookup_worker (bfd *abfd)
{
  if ((abfd->flags & BFD_IN_MEMORY) != 0)
    abort ();

  if (abfd->my_archive)
    abfd = abfd->my_archive;

  if (abfd->iostream != NULL)
    {
      /* Move the file to the start of the cache.  */
      if (abfd != bfd_last_cache)
	{
	  snip (abfd);
	  insert (abfd);
	}
    }
  else
    {
      if (bfd_open_file (abfd) == NULL
	  || abfd->where != (unsigned long) abfd->where
	  || real_fseek ((FILE *) abfd->iostream, abfd->where, SEEK_SET) != 0)
	abort ();
    }

  return (FILE *) abfd->iostream;
}
