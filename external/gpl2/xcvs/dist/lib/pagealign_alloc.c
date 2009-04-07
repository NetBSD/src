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

/* Written by Derek R. Price <derek@ximbiot.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "pagealign_alloc.h"

#include <errno.h>
#include <stdlib.h>

#include <fcntl.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_MMAP
# include <sys/mman.h>
#endif

#include "error.h"
#include "exit.h"
#include "getpagesize.h"
#include "xalloc.h"
#include "gettext.h"

#define _(str) gettext (str)

#if HAVE_MMAP
/* Define MAP_FILE when it isn't otherwise.  */
# ifndef MAP_FILE
#  define MAP_FILE 0
# endif
/* Define MAP_FAILED for old systems which neglect to.  */
# ifndef MAP_FAILED
#  define MAP_FAILED ((void *)-1)
# endif
#endif


#if HAVE_MMAP || ! HAVE_POSIX_MEMALIGN

# if HAVE_MMAP
/* For each memory region, we store its size.  */
typedef size_t info_t;
# else
/* For each memory region, we store the original pointer returned by
   malloc().  */
typedef void * info_t;
# endif

/* A simple linked list of allocated memory regions.  It is probably not the
   most efficient way to store these, but anyway...  */
typedef struct memnode_s memnode_t;
struct memnode_s
{
  void *aligned_ptr;
  info_t info;
  memnode_t *next;
};

/* The list of currently allocated memory regions.  */
static memnode_t *memnode_table = NULL;


static void
new_memnode (void *aligned_ptr, info_t info)
{
  memnode_t *new_node = (memnode_t *) xmalloc (sizeof (memnode_t));
  new_node->aligned_ptr = aligned_ptr;
  new_node->info = info;
  new_node->next = memnode_table;
  memnode_table = new_node;
}


/* Dispose of the memnode containing a map for the ALIGNED_PTR in question
   and return the content of the node's INFO field.  */
static info_t
get_memnode (void *aligned_ptr)
{
  info_t ret;
  memnode_t *c;
  memnode_t **p_next = &memnode_table;

  for (c = *p_next; c != NULL; p_next = &c->next, c = c->next)
    if (c->aligned_ptr == aligned_ptr)
      break;

  if (c == NULL)
    /* An attempt to free untracked memory.  A wrong pointer was passed
       to pagealign_free().  */
    abort ();

  /* Remove this entry from the list, save the return value, and free it.  */
  *p_next = c->next;
  ret = c->info;
  free (c);

  return ret;
}

#endif /* HAVE_MMAP || !HAVE_POSIX_MEMALIGN */


void *
pagealign_alloc (size_t size)
{
  void *ret;
#if HAVE_MMAP
# ifdef HAVE_MAP_ANONYMOUS
  const int fd = -1;
  const int flags = MAP_ANONYMOUS | MAP_PRIVATE;
# else /* !HAVE_MAP_ANONYMOUS */
  static int fd = -1;  /* Only open /dev/zero once in order to avoid limiting
			  the amount of memory we may allocate based on the
			  number of open file descriptors.  */
  const int flags = MAP_FILE | MAP_PRIVATE;
  if (fd == -1)
    {
      fd = open ("/dev/zero", O_RDONLY, 0666);
      if (fd < 0)
	error (EXIT_FAILURE, errno, _("Failed to open /dev/zero for read"));
    }
# endif /* HAVE_MAP_ANONYMOUS */
  ret = mmap (NULL, size, PROT_READ | PROT_WRITE, flags, fd, 0);
  if (ret == MAP_FAILED)
    return NULL;
  new_memnode (ret, size);
#elif HAVE_POSIX_MEMALIGN
  int status = posix_memalign (&ret, getpagesize (), size);
  if (status)
    {
      errno = status;
      return NULL;
    }
#else /* !HAVE_MMAP && !HAVE_POSIX_MEMALIGN */
  size_t pagesize = getpagesize ();
  void *unaligned_ptr = malloc (size + pagesize - 1);
  if (unaligned_ptr == NULL)
    {
      /* Set errno.  We don't know whether malloc already set errno: some
	 implementations of malloc do, some don't.  */
      errno = ENOMEM;
      return NULL;
    }
  ret = (char *) unaligned_ptr
        + ((- (unsigned long) unaligned_ptr) & (pagesize - 1));
  new_memnode (ret, unaligned_ptr);
#endif /* HAVE_MMAP && HAVE_POSIX_MEMALIGN */
  return ret;
}


void *
pagealign_xalloc (size_t size)
{
  void *ret;

  ret = pagealign_alloc (size);
  if (ret == NULL)
    xalloc_die ();
  return ret;
}


void
pagealign_free (void *aligned_ptr)
{
#if HAVE_MMAP
  if (munmap (aligned_ptr, get_memnode (aligned_ptr)) < 0)
    error (EXIT_FAILURE, errno, "Failed to unmap memory");
#elif HAVE_POSIX_MEMALIGN
  free (aligned_ptr);
#else
  free (get_memnode (aligned_ptr));
#endif
}
