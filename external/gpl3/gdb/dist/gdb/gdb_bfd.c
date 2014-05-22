/* Definitions for BFD wrappers used by GDB.

   Copyright (C) 2011-2013 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "gdb_bfd.h"
#include "gdb_assert.h"
#include "gdb_string.h"
#include "ui-out.h"
#include "gdbcmd.h"
#include "hashtab.h"
#ifdef HAVE_ZLIB_H
#include <zlib.h>
#endif
#ifdef HAVE_MMAP
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif
#endif

/* An object of this type is stored in the section's user data when
   mapping a section.  */

struct gdb_bfd_section_data
{
  /* Size of the data.  */
  bfd_size_type size;
  /* If the data was mmapped, this is the length of the map.  */
  bfd_size_type map_len;
  /* The data.  If NULL, the section data has not been read.  */
  void *data;
  /* If the data was mmapped, this is the map address.  */
  void *map_addr;
};

/* A hash table holding every BFD that gdb knows about.  This is not
   to be confused with 'gdb_bfd_cache', which is used for sharing
   BFDs; in contrast, this hash is used just to implement
   "maint info bfd".  */

static htab_t all_bfds;

/* See gdb_bfd.h.  */

void
gdb_bfd_stash_filename (struct bfd *abfd)
{
  char *name = bfd_get_filename (abfd);
  char *data;

  data = bfd_alloc (abfd, strlen (name) + 1);
  strcpy (data, name);

  /* Unwarranted chumminess with BFD.  */
  abfd->filename = data;
}

/* An object of this type is stored in each BFD's user data.  */

struct gdb_bfd_data
{
  /* The reference count.  */
  int refc;

  /* The mtime of the BFD at the point the cache entry was made.  */
  time_t mtime;

  /* If the BFD comes from an archive, this points to the archive's
     BFD.  Otherwise, this is NULL.  */
  bfd *archive_bfd;

  /* The registry.  */
  REGISTRY_FIELDS;
};

#define GDB_BFD_DATA_ACCESSOR(ABFD) \
  ((struct gdb_bfd_data *) bfd_usrdata (ABFD))

DEFINE_REGISTRY (bfd, GDB_BFD_DATA_ACCESSOR)

/* A hash table storing all the BFDs maintained in the cache.  */

static htab_t gdb_bfd_cache;

/* The type of an object being looked up in gdb_bfd_cache.  We use
   htab's capability of storing one kind of object (BFD in this case)
   and using a different sort of object for searching.  */

struct gdb_bfd_cache_search
{
  /* The filename.  */
  const char *filename;
  /* The mtime.  */
  time_t mtime;
};

/* A hash function for BFDs.  */

static hashval_t
hash_bfd (const void *b)
{
  const bfd *abfd = b;

  /* It is simplest to just hash the filename.  */
  return htab_hash_string (bfd_get_filename (abfd));
}

/* An equality function for BFDs.  Note that this expects the caller
   to search using struct gdb_bfd_cache_search only, not BFDs.  */

static int
eq_bfd (const void *a, const void *b)
{
  const bfd *abfd = a;
  const struct gdb_bfd_cache_search *s = b;
  struct gdb_bfd_data *gdata = bfd_usrdata (abfd);

  return (gdata->mtime == s->mtime
	  && strcmp (bfd_get_filename (abfd), s->filename) == 0);
}

/* See gdb_bfd.h.  */

struct bfd *
gdb_bfd_open (const char *name, const char *target, int fd)
{
  hashval_t hash;
  void **slot;
  bfd *abfd;
  struct gdb_bfd_cache_search search;
  struct stat st;

  if (gdb_bfd_cache == NULL)
    gdb_bfd_cache = htab_create_alloc (1, hash_bfd, eq_bfd, NULL,
				       xcalloc, xfree);

  if (fd == -1)
    {
      fd = open (name, O_RDONLY | O_BINARY);
      if (fd == -1)
	{
	  bfd_set_error (bfd_error_system_call);
	  return NULL;
	}
    }

  search.filename = name;
  if (fstat (fd, &st) < 0)
    {
      /* Weird situation here.  */
      search.mtime = 0;
    }
  else
    search.mtime = st.st_mtime;

  /* Note that this must compute the same result as hash_bfd.  */
  hash = htab_hash_string (name);
  /* Note that we cannot use htab_find_slot_with_hash here, because
     opening the BFD may fail; and this would violate hashtab
     invariants.  */
  abfd = htab_find_with_hash (gdb_bfd_cache, &search, hash);
  if (abfd != NULL)
    {
      close (fd);
      gdb_bfd_ref (abfd);
      return abfd;
    }

  abfd = bfd_fopen (name, target, FOPEN_RB, fd);
  if (abfd == NULL)
    return NULL;

  slot = htab_find_slot_with_hash (gdb_bfd_cache, &search, hash, INSERT);
  gdb_assert (!*slot);
  *slot = abfd;

  gdb_bfd_stash_filename (abfd);
  gdb_bfd_ref (abfd);
  return abfd;
}

/* A helper function that releases any section data attached to the
   BFD.  */

static void
free_one_bfd_section (bfd *abfd, asection *sectp, void *ignore)
{
  struct gdb_bfd_section_data *sect = bfd_get_section_userdata (abfd, sectp);

  if (sect != NULL && sect->data != NULL)
    {
#ifdef HAVE_MMAP
      if (sect->map_addr != NULL)
	{
	  int res;

	  res = munmap (sect->map_addr, sect->map_len);
	  gdb_assert (res == 0);
	}
      else
#endif
	xfree (sect->data);
    }
}

/* Close ABFD, and warn if that fails.  */

static int
gdb_bfd_close_or_warn (struct bfd *abfd)
{
  int ret;
  char *name = bfd_get_filename (abfd);

  bfd_map_over_sections (abfd, free_one_bfd_section, NULL);

  ret = bfd_close (abfd);

  if (!ret)
    warning (_("cannot close \"%s\": %s"),
	     name, bfd_errmsg (bfd_get_error ()));

  return ret;
}

/* See gdb_bfd.h.  */

void
gdb_bfd_ref (struct bfd *abfd)
{
  struct gdb_bfd_data *gdata;
  void **slot;

  if (abfd == NULL)
    return;

  gdata = bfd_usrdata (abfd);

  if (gdata != NULL)
    {
      gdata->refc += 1;
      return;
    }

  /* Ask BFD to decompress sections in bfd_get_full_section_contents.  */
  abfd->flags |= BFD_DECOMPRESS;

  gdata = bfd_zalloc (abfd, sizeof (struct gdb_bfd_data));
  gdata->refc = 1;
  gdata->mtime = bfd_get_mtime (abfd);
  gdata->archive_bfd = NULL;
  bfd_usrdata (abfd) = gdata;

  bfd_alloc_data (abfd);

  /* This is the first we've seen it, so add it to the hash table.  */
  slot = htab_find_slot (all_bfds, abfd, INSERT);
  gdb_assert (slot && !*slot);
  *slot = abfd;
}

/* See gdb_bfd.h.  */

void
gdb_bfd_unref (struct bfd *abfd)
{
  struct gdb_bfd_data *gdata;
  struct gdb_bfd_cache_search search;
  bfd *archive_bfd;

  if (abfd == NULL)
    return;

  gdata = bfd_usrdata (abfd);
  gdb_assert (gdata->refc >= 1);

  gdata->refc -= 1;
  if (gdata->refc > 0)
    return;

  archive_bfd = gdata->archive_bfd;
  search.filename = bfd_get_filename (abfd);

  if (gdb_bfd_cache && search.filename)
    {
      hashval_t hash = htab_hash_string (search.filename);
      void **slot;

      search.mtime = gdata->mtime;
      slot = htab_find_slot_with_hash (gdb_bfd_cache, &search, hash,
				       NO_INSERT);

      if (slot && *slot)
	htab_clear_slot (gdb_bfd_cache, slot);
    }

  bfd_free_data (abfd);
  bfd_usrdata (abfd) = NULL;  /* Paranoia.  */

  htab_remove_elt (all_bfds, abfd);

  gdb_bfd_close_or_warn (abfd);

  gdb_bfd_unref (archive_bfd);
}

/* A helper function that returns the section data descriptor
   associated with SECTION.  If no such descriptor exists, a new one
   is allocated and cleared.  */

static struct gdb_bfd_section_data *
get_section_descriptor (asection *section)
{
  struct gdb_bfd_section_data *result;

  result = bfd_get_section_userdata (section->owner, section);

  if (result == NULL)
    {
      result = bfd_zalloc (section->owner, sizeof (*result));
      bfd_set_section_userdata (section->owner, section, result);
    }

  return result;
}

/* See gdb_bfd.h.  */

const gdb_byte *
gdb_bfd_map_section (asection *sectp, bfd_size_type *size)
{
  bfd *abfd;
  struct gdb_bfd_section_data *descriptor;
  bfd_byte *data;

  gdb_assert ((sectp->flags & SEC_RELOC) == 0);
  gdb_assert (size != NULL);

  abfd = sectp->owner;

  descriptor = get_section_descriptor (sectp);

  /* If the data was already read for this BFD, just reuse it.  */
  if (descriptor->data != NULL)
    goto done;

#ifdef HAVE_MMAP
  if (!bfd_is_section_compressed (abfd, sectp))
    {
      /* The page size, used when mmapping.  */
      static int pagesize;

      if (pagesize == 0)
	pagesize = getpagesize ();

      /* Only try to mmap sections which are large enough: we don't want
	 to waste space due to fragmentation.  */

      if (bfd_get_section_size (sectp) > 4 * pagesize)
	{
	  descriptor->size = bfd_get_section_size (sectp);
	  descriptor->data = bfd_mmap (abfd, 0, descriptor->size, PROT_READ,
				       MAP_PRIVATE, sectp->filepos,
				       &descriptor->map_addr,
				       &descriptor->map_len);

	  if ((caddr_t)descriptor->data != MAP_FAILED)
	    {
#if HAVE_POSIX_MADVISE
	      posix_madvise (descriptor->map_addr, descriptor->map_len,
			     POSIX_MADV_WILLNEED);
#endif
	      goto done;
	    }

	  /* On failure, clear out the section data and try again.  */
	  memset (descriptor, 0, sizeof (*descriptor));
	}
    }
#endif /* HAVE_MMAP */

  /* Handle compressed sections, or ordinary uncompressed sections in
     the no-mmap case.  */

  descriptor->size = bfd_get_section_size (sectp);
  descriptor->data = NULL;

  data = NULL;
  if (!bfd_get_full_section_contents (abfd, sectp, &data))
    error (_("Can't read data for section '%s' in file '%s'"),
	   bfd_get_section_name (abfd, sectp),
	   bfd_get_filename (abfd));
  descriptor->data = data;

 done:
  gdb_assert (descriptor->data != NULL);
  *size = descriptor->size;
  return descriptor->data;
}



/* See gdb_bfd.h.  */

bfd *
gdb_bfd_fopen (const char *filename, const char *target, const char *mode,
	       int fd)
{
  bfd *result = bfd_fopen (filename, target, mode, fd);

  if (result)
    {
      gdb_bfd_stash_filename (result);
      gdb_bfd_ref (result);
    }

  return result;
}

/* See gdb_bfd.h.  */

bfd *
gdb_bfd_openr (const char *filename, const char *target)
{
  bfd *result = bfd_openr (filename, target);

  if (result)
    {
      gdb_bfd_stash_filename (result);
      gdb_bfd_ref (result);
    }

  return result;
}

/* See gdb_bfd.h.  */

bfd *
gdb_bfd_openw (const char *filename, const char *target)
{
  bfd *result = bfd_openw (filename, target);

  if (result)
    {
      gdb_bfd_stash_filename (result);
      gdb_bfd_ref (result);
    }

  return result;
}

/* See gdb_bfd.h.  */

bfd *
gdb_bfd_openr_iovec (const char *filename, const char *target,
		     void *(*open_func) (struct bfd *nbfd,
					 void *open_closure),
		     void *open_closure,
		     file_ptr (*pread_func) (struct bfd *nbfd,
					     void *stream,
					     void *buf,
					     file_ptr nbytes,
					     file_ptr offset),
		     int (*close_func) (struct bfd *nbfd,
					void *stream),
		     int (*stat_func) (struct bfd *abfd,
				       void *stream,
				       struct stat *sb))
{
  bfd *result = bfd_openr_iovec (filename, target,
				 open_func, open_closure,
				 pread_func, close_func, stat_func);

  if (result)
    {
      gdb_bfd_ref (result);
      gdb_bfd_stash_filename (result);
    }

  return result;
}

/* See gdb_bfd.h.  */

void
gdb_bfd_mark_parent (bfd *child, bfd *parent)
{
  struct gdb_bfd_data *gdata;

  gdb_bfd_ref (child);
  /* No need to stash the filename here, because we also keep a
     reference on the parent archive.  */

  gdata = bfd_usrdata (child);
  if (gdata->archive_bfd == NULL)
    {
      gdata->archive_bfd = parent;
      gdb_bfd_ref (parent);
    }
  else
    gdb_assert (gdata->archive_bfd == parent);
}

/* See gdb_bfd.h.  */

bfd *
gdb_bfd_openr_next_archived_file (bfd *archive, bfd *previous)
{
  bfd *result = bfd_openr_next_archived_file (archive, previous);

  if (result)
    gdb_bfd_mark_parent (result, archive);

  return result;
}

/* See gdb_bfd.h.  */

bfd *
gdb_bfd_fdopenr (const char *filename, const char *target, int fd)
{
  bfd *result = bfd_fdopenr (filename, target, fd);

  if (result)
    {
      gdb_bfd_ref (result);
      gdb_bfd_stash_filename (result);
    }

  return result;
}



/* A callback for htab_traverse that prints a single BFD.  */

static int
print_one_bfd (void **slot, void *data)
{
  bfd *abfd = *slot;
  struct gdb_bfd_data *gdata = bfd_usrdata (abfd);
  struct ui_out *uiout = data;
  struct cleanup *inner;

  inner = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
  ui_out_field_int (uiout, "refcount", gdata->refc);
  ui_out_field_string (uiout, "addr", host_address_to_string (abfd));
  ui_out_field_string (uiout, "filename", bfd_get_filename (abfd));
  ui_out_text (uiout, "\n");
  do_cleanups (inner);

  return 1;
}

/* Implement the 'maint info bfd' command.  */

static void
maintenance_info_bfds (char *arg, int from_tty)
{
  struct cleanup *cleanup;
  struct ui_out *uiout = current_uiout;

  cleanup = make_cleanup_ui_out_table_begin_end (uiout, 3, -1, "bfds");
  ui_out_table_header (uiout, 10, ui_left, "refcount", "Refcount");
  ui_out_table_header (uiout, 18, ui_left, "addr", "Address");
  ui_out_table_header (uiout, 40, ui_left, "filename", "Filename");

  ui_out_table_body (uiout);
  htab_traverse (all_bfds, print_one_bfd, uiout);

  do_cleanups (cleanup);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_gdb_bfd;

void
_initialize_gdb_bfd (void)
{
  all_bfds = htab_create_alloc (10, htab_hash_pointer, htab_eq_pointer,
				NULL, xcalloc, xfree);

  add_cmd ("bfds", class_maintenance, maintenance_info_bfds, _("\
List the BFDs that are currently open."),
	   &maintenanceinfolist);
}
