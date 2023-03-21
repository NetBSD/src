/* CTF archive files.
   Copyright (C) 2019-2020 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <ctf-impl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
#include "ctf-endian.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

static off_t arc_write_one_ctf (ctf_file_t * f, int fd, size_t threshold);
static ctf_file_t *ctf_arc_open_by_offset (const struct ctf_archive *arc,
					   const ctf_sect_t *symsect,
					   const ctf_sect_t *strsect,
					   size_t offset, int *errp);
static int sort_modent_by_name (const void *one, const void *two, void *n);
static void *arc_mmap_header (int fd, size_t headersz);
static void *arc_mmap_file (int fd, size_t size);
static int arc_mmap_writeout (int fd, void *header, size_t headersz,
			      const char **errmsg);
static int arc_mmap_unmap (void *header, size_t headersz, const char **errmsg);

/* Write out a CTF archive to the start of the file referenced by the passed-in
   fd.  The entries in CTF_FILES are referenced by name: the names are passed in
   the names array, which must have CTF_FILES entries.

   Returns 0 on success, or an errno, or an ECTF_* value.  */
int
ctf_arc_write_fd (int fd, ctf_file_t **ctf_files, size_t ctf_file_cnt,
		  const char **names, size_t threshold)
{
  const char *errmsg;
  struct ctf_archive *archdr;
  size_t i;
  char dummy = 0;
  size_t headersz;
  ssize_t namesz;
  size_t ctf_startoffs;		/* Start of the section we are working over.  */
  char *nametbl = NULL;		/* The name table.  */
  char *np;
  off_t nameoffs;
  struct ctf_archive_modent *modent;

  ctf_dprintf ("Writing CTF archive with %lu files\n",
	       (unsigned long) ctf_file_cnt);

  /* Figure out the size of the mmap()ed header, including the
     ctf_archive_modent array.  We assume that all of this needs no
     padding: a likely assumption, given that it's all made up of
     uint64_t's.  */
  headersz = sizeof (struct ctf_archive)
    + (ctf_file_cnt * sizeof (uint64_t) * 2);
  ctf_dprintf ("headersz is %lu\n", (unsigned long) headersz);

  /* From now on we work in two pieces: an mmap()ed region from zero up to the
     headersz, and a region updated via write() starting after that, containing
     all the tables.  Platforms that do not support mmap() just use write().  */
  ctf_startoffs = headersz;
  if (lseek (fd, ctf_startoffs - 1, SEEK_SET) < 0)
    {
      errmsg = N_("ctf_arc_write(): cannot extend file while writing");
      goto err;
    }

  if (write (fd, &dummy, 1) < 0)
    {
      errmsg = N_("ctf_arc_write(): cannot extend file while writing");
      goto err;
    }

  if ((archdr = arc_mmap_header (fd, headersz)) == NULL)
    {
      errmsg = N_("ctf_arc_write(): cannot mmap");
      goto err;
    }

  /* Fill in everything we can, which is everything other than the name
     table offset.  */
  archdr->ctfa_magic = htole64 (CTFA_MAGIC);
  archdr->ctfa_nfiles = htole64 (ctf_file_cnt);
  archdr->ctfa_ctfs = htole64 (ctf_startoffs);

  /* We could validate that all CTF files have the same data model, but
     since any reasonable construction process will be building things of
     only one bitness anyway, this is pretty pointless, so just use the
     model of the first CTF file for all of them.  (It *is* valid to
     create an empty archive: the value of ctfa_model is irrelevant in
     this case, but we must be sure not to dereference uninitialized
     memory.)  */

  if (ctf_file_cnt > 0)
    archdr->ctfa_model = htole64 (ctf_getmodel (ctf_files[0]));

  /* Now write out the CTFs: ctf_archive_modent array via the mapping,
     ctfs via write().  The names themselves have not been written yet: we
     track them in a local strtab until the time is right, and sort the
     modents array after construction.

    The name table is not sorted.  */

  for (i = 0, namesz = 0; i < le64toh (archdr->ctfa_nfiles); i++)
    namesz += strlen (names[i]) + 1;

  nametbl = malloc (namesz);
  if (nametbl == NULL)
    {
      errmsg = N_("ctf_arc_write(): error writing named CTF to archive");
      goto err_unmap;
    }

  for (i = 0, namesz = 0,
       modent = (ctf_archive_modent_t *) ((char *) archdr
					  + sizeof (struct ctf_archive));
       i < le64toh (archdr->ctfa_nfiles); i++)
    {
      off_t off;

      strcpy (&nametbl[namesz], names[i]);

      off = arc_write_one_ctf (ctf_files[i], fd, threshold);
      if ((off < 0) && (off > -ECTF_BASE))
	{
	  errmsg = N_("ctf_arc_write(): cannot determine file "
		      "position while writing to archive");
	  goto err_free;
	}
      if (off < 0)
	{
	  errmsg = N_("ctf_arc_write(): cannot write CTF file to archive");
	  errno = off * -1;
	  goto err_free;
	}

      modent->name_offset = htole64 (namesz);
      modent->ctf_offset = htole64 (off - ctf_startoffs);
      namesz += strlen (names[i]) + 1;
      modent++;
    }

  ctf_qsort_r ((ctf_archive_modent_t *) ((char *) archdr
					 + sizeof (struct ctf_archive)),
	       le64toh (archdr->ctfa_nfiles),
	       sizeof (struct ctf_archive_modent), sort_modent_by_name,
	       nametbl);

   /* Now the name table.  */

  if ((nameoffs = lseek (fd, 0, SEEK_CUR)) < 0)
    {
      errmsg = N_("ctf_arc_write(): cannot get current file position "
		  "in archive");
      goto err_free;
    }
  archdr->ctfa_names = htole64 (nameoffs);
  np = nametbl;
  while (namesz > 0)
    {
      ssize_t len;
      if ((len = write (fd, np, namesz)) < 0)
	{
	  errmsg = N_("ctf_arc_write(): cannot write name table to archive");
	  goto err_free;
	}
      namesz -= len;
      np += len;
    }
  free (nametbl);

  if (arc_mmap_writeout (fd, archdr, headersz, &errmsg) < 0)
    goto err_unmap;
  if (arc_mmap_unmap (archdr, headersz, &errmsg) < 0)
    goto err;
  return 0;

err_free:
  free (nametbl);
err_unmap:
  arc_mmap_unmap (archdr, headersz, NULL);
err:
  /* We report errors into the first file in the archive, if any: if this is a
     zero-file archive, put it in the open-errors stream for lack of anywhere
     else for it to go.  */
  ctf_err_warn (ctf_file_cnt > 0 ? ctf_files[0] : NULL, 0, errno, "%s",
		gettext (errmsg));
  return errno;
}

/* Write out a CTF archive.  The entries in CTF_FILES are referenced by name:
   the names are passed in the names array, which must have CTF_FILES entries.

   If the filename is NULL, create a temporary file and return a pointer to it.

   Returns 0 on success, or an errno, or an ECTF_* value.  */
int
ctf_arc_write (const char *file, ctf_file_t **ctf_files, size_t ctf_file_cnt,
	       const char **names, size_t threshold)
{
  int err;
  int fd;

  if ((fd = open (file, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0666)) < 0)
    {
      ctf_err_warn (ctf_file_cnt > 0 ? ctf_files[0] : NULL, 0, errno,
		    _("ctf_arc_write(): cannot create %s"), file);
      return errno;
    }

  err = ctf_arc_write_fd (fd, ctf_files, ctf_file_cnt, names, threshold);
  if (err)
    goto err_close;

  if ((err = close (fd)) < 0)
    ctf_err_warn (ctf_file_cnt > 0 ? ctf_files[0] : NULL, 0, errno,
		  _("ctf_arc_write(): cannot close after writing to archive"));
  goto err;

 err_close:
  (void) close (fd);
 err:
  if (err < 0)
    unlink (file);

  return err;
}

/* Write one CTF file out.  Return the file position of the written file (or
   rather, of the file-size uint64_t that precedes it): negative return is a
   negative errno or ctf_errno value.  On error, the file position may no longer
   be at the end of the file.  */
static off_t
arc_write_one_ctf (ctf_file_t * f, int fd, size_t threshold)
{
  off_t off, end_off;
  uint64_t ctfsz = 0;
  char *ctfszp;
  size_t ctfsz_len;
  int (*writefn) (ctf_file_t * fp, int fd);

  if (ctf_serialize (f) < 0)
    return f->ctf_errno * -1;

  if ((off = lseek (fd, 0, SEEK_CUR)) < 0)
    return errno * -1;

  if (f->ctf_size > threshold)
    writefn = ctf_compress_write;
  else
    writefn = ctf_write;

  /* This zero-write turns into the size in a moment. */
  ctfsz_len = sizeof (ctfsz);
  ctfszp = (char *) &ctfsz;
  while (ctfsz_len > 0)
    {
      ssize_t writelen = write (fd, ctfszp, ctfsz_len);
      if (writelen < 0)
	return errno * -1;
      ctfsz_len -= writelen;
      ctfszp += writelen;
    }

  if (writefn (f, fd) != 0)
    return f->ctf_errno * -1;

  if ((end_off = lseek (fd, 0, SEEK_CUR)) < 0)
    return errno * -1;
  ctfsz = htole64 (end_off - off);

  if ((lseek (fd, off, SEEK_SET)) < 0)
    return errno * -1;

  /* ... here.  */
  ctfsz_len = sizeof (ctfsz);
  ctfszp = (char *) &ctfsz;
  while (ctfsz_len > 0)
    {
      ssize_t writelen = write (fd, ctfszp, ctfsz_len);
      if (writelen < 0)
	return errno * -1;
      ctfsz_len -= writelen;
      ctfszp += writelen;
    }

  end_off = LCTF_ALIGN_OFFS (end_off, 8);
  if ((lseek (fd, end_off, SEEK_SET)) < 0)
    return errno * -1;

  return off;
}

/* qsort() function to sort the array of struct ctf_archive_modents into
   ascending name order.  */
static int
sort_modent_by_name (const void *one, const void *two, void *n)
{
  const struct ctf_archive_modent *a = one;
  const struct ctf_archive_modent *b = two;
  char *nametbl = n;

  return strcmp (&nametbl[le64toh (a->name_offset)],
		 &nametbl[le64toh (b->name_offset)]);
}

/* bsearch_r() function to search for a given name in the sorted array of struct
   ctf_archive_modents.  */
static int
search_modent_by_name (const void *key, const void *ent, void *arg)
{
  const char *k = key;
  const struct ctf_archive_modent *v = ent;
  const char *search_nametbl = arg;

  return strcmp (k, &search_nametbl[le64toh (v->name_offset)]);
}

/* Make a new struct ctf_archive_internal wrapper for a ctf_archive or a
   ctf_file.  Closes ARC and/or FP on error.  Arrange to free the SYMSECT or
   STRSECT, as needed, on close.  Possibly do not unmap on close.  */

struct ctf_archive_internal *
ctf_new_archive_internal (int is_archive, int unmap_on_close,
			  struct ctf_archive *arc,
			  ctf_file_t *fp, const ctf_sect_t *symsect,
			  const ctf_sect_t *strsect,
			  int *errp)
{
  struct ctf_archive_internal *arci;

  if ((arci = calloc (1, sizeof (struct ctf_archive_internal))) == NULL)
    {
      if (is_archive)
	{
	  if (unmap_on_close)
	    ctf_arc_close_internal (arc);
	}
      else
	ctf_file_close (fp);
      return (ctf_set_open_errno (errp, errno));
    }
  arci->ctfi_is_archive = is_archive;
  if (is_archive)
    arci->ctfi_archive = arc;
  else
    arci->ctfi_file = fp;
  if (symsect)
     memcpy (&arci->ctfi_symsect, symsect, sizeof (struct ctf_sect));
  if (strsect)
     memcpy (&arci->ctfi_strsect, strsect, sizeof (struct ctf_sect));
  arci->ctfi_free_symsect = 0;
  arci->ctfi_free_strsect = 0;
  arci->ctfi_unmap_on_close = unmap_on_close;

  return arci;
}

/* Open a CTF archive or dictionary from data in a buffer (which the caller must
   preserve until ctf_arc_close() time).  Returns the archive, or NULL and an
   error in *err (if not NULL).  */
ctf_archive_t *
ctf_arc_bufopen (const ctf_sect_t *ctfsect, const ctf_sect_t *symsect,
		 const ctf_sect_t *strsect, int *errp)
{
  struct ctf_archive *arc = NULL;
  int is_archive;
  ctf_file_t *fp = NULL;

  if (ctfsect->cts_size > sizeof (uint64_t) &&
      (le64toh ((*(uint64_t *) ctfsect->cts_data)) == CTFA_MAGIC))
    {
      /* The archive is mmappable, so this operation is trivial.

	 This buffer is nonmodifiable, so the trick involving mmapping only part
	 of it and storing the length in the magic number is not applicable: so
	 record this fact in the archive-wrapper header.  (We cannot record it
	 in the archive, because the archive may very well be a read-only
	 mapping.)  */

      is_archive = 1;
      arc = (struct ctf_archive *) ctfsect->cts_data;
    }
  else
    {
      is_archive = 0;
      if ((fp = ctf_bufopen (ctfsect, symsect, strsect, errp)) == NULL)
	{
	  ctf_err_warn (NULL, 0, *errp, _("ctf_arc_bufopen(): cannot open CTF"));
	  return NULL;
	}
    }
  return ctf_new_archive_internal (is_archive, 0, arc, fp, symsect, strsect,
				   errp);
}

/* Open a CTF archive.  Returns the archive, or NULL and an error in *err (if
   not NULL).  */
struct ctf_archive *
ctf_arc_open_internal (const char *filename, int *errp)
{
  const char *errmsg;
  int fd;
  struct stat s;
  struct ctf_archive *arc;		/* (Actually the whole file.)  */

  libctf_init_debug();
  if ((fd = open (filename, O_RDONLY)) < 0)
    {
      errmsg = N_("ctf_arc_open(): cannot open %s");
      goto err;
    }
  if (fstat (fd, &s) < 0)
    {
      errmsg = N_("ctf_arc_open(): cannot stat %s");
      goto err_close;
    }

  if ((arc = arc_mmap_file (fd, s.st_size)) == NULL)
    {
      errmsg = N_("ctf_arc_open(): cannot read in %s");
      goto err_close;
    }

  if (le64toh (arc->ctfa_magic) != CTFA_MAGIC)
    {
      errmsg = N_("ctf_arc_open(): %s: invalid magic number");
      errno = ECTF_FMT;
      goto err_unmap;
    }

  /* This horrible hack lets us know how much to unmap when the file is
     closed.  (We no longer need the magic number, and the mapping
     is private.)  */
  arc->ctfa_magic = s.st_size;
  close (fd);
  return arc;

err_unmap:
  arc_mmap_unmap (arc, s.st_size, NULL);
err_close:
  close (fd);
err:
  if (errp)
    *errp = errno;
  ctf_err_warn (NULL, 0, errno, gettext (errmsg), filename);
  return NULL;
}

/* Close an archive.  */
void
ctf_arc_close_internal (struct ctf_archive *arc)
{
  if (arc == NULL)
    return;

  /* See the comment in ctf_arc_open().  */
  arc_mmap_unmap (arc, arc->ctfa_magic, NULL);
}

/* Public entry point: close an archive, or CTF file.  */
void
ctf_arc_close (ctf_archive_t *arc)
{
  if (arc == NULL)
    return;

  if (arc->ctfi_is_archive)
    {
      if (arc->ctfi_unmap_on_close)
	ctf_arc_close_internal (arc->ctfi_archive);
    }
  else
    ctf_file_close (arc->ctfi_file);
  if (arc->ctfi_free_symsect)
    free ((void *) arc->ctfi_symsect.cts_data);
  if (arc->ctfi_free_strsect)
    free ((void *) arc->ctfi_strsect.cts_data);
  free (arc->ctfi_data);
  if (arc->ctfi_bfd_close)
    arc->ctfi_bfd_close (arc);
  free (arc);
}

/* Return the ctf_file_t with the given name, or NULL if none, setting 'err' if
   non-NULL.  A name of NULL means to open the default file.  */
static ctf_file_t *
ctf_arc_open_by_name_internal (const struct ctf_archive *arc,
			       const ctf_sect_t *symsect,
			       const ctf_sect_t *strsect,
			       const char *name, int *errp)
{
  struct ctf_archive_modent *modent;
  const char *search_nametbl;

  if (name == NULL)
    name = _CTF_SECTION;		 /* The default name.  */

  ctf_dprintf ("ctf_arc_open_by_name(%s): opening\n", name);

  modent = (ctf_archive_modent_t *) ((char *) arc
				     + sizeof (struct ctf_archive));

  search_nametbl = (const char *) arc + le64toh (arc->ctfa_names);
  modent = bsearch_r (name, modent, le64toh (arc->ctfa_nfiles),
		      sizeof (struct ctf_archive_modent),
		      search_modent_by_name, (void *) search_nametbl);

  /* This is actually a common case and normal operation: no error
     debug output.  */
  if (modent == NULL)
    {
      if (errp)
	*errp = ECTF_ARNNAME;
      return NULL;
    }

  return ctf_arc_open_by_offset (arc, symsect, strsect,
				 le64toh (modent->ctf_offset), errp);
}

/* Return the ctf_file_t with the given name, or NULL if none, setting 'err' if
   non-NULL.  A name of NULL means to open the default file.

   Use the specified string and symbol table sections.

   Public entry point.  */
ctf_file_t *
ctf_arc_open_by_name_sections (const ctf_archive_t *arc,
			       const ctf_sect_t *symsect,
			       const ctf_sect_t *strsect,
			       const char *name,
			       int *errp)
{
  if (arc->ctfi_is_archive)
    {
      ctf_file_t *ret;
      ret = ctf_arc_open_by_name_internal (arc->ctfi_archive, symsect, strsect,
					   name, errp);
      if (ret)
	ret->ctf_archive = (ctf_archive_t *) arc;
      return ret;
    }

  if ((name != NULL) && (strcmp (name, _CTF_SECTION) != 0))
    {
      if (errp)
	*errp = ECTF_ARNNAME;
      return NULL;
    }
  arc->ctfi_file->ctf_archive = (ctf_archive_t *) arc;

  /* Bump the refcount so that the user can ctf_file_close() it.  */
  arc->ctfi_file->ctf_refcnt++;
  return arc->ctfi_file;
}

/* Return the ctf_file_t with the given name, or NULL if none, setting 'err' if
   non-NULL.  A name of NULL means to open the default file.

   Public entry point.  */
ctf_file_t *
ctf_arc_open_by_name (const ctf_archive_t *arc, const char *name, int *errp)
{
  const ctf_sect_t *symsect = &arc->ctfi_symsect;
  const ctf_sect_t *strsect = &arc->ctfi_strsect;

  if (symsect->cts_name == NULL)
    symsect = NULL;
  if (strsect->cts_name == NULL)
    strsect = NULL;

  return ctf_arc_open_by_name_sections (arc, symsect, strsect, name, errp);
}

/* Return the ctf_file_t at the given ctfa_ctfs-relative offset, or NULL if
   none, setting 'err' if non-NULL.  */
static ctf_file_t *
ctf_arc_open_by_offset (const struct ctf_archive *arc,
			const ctf_sect_t *symsect,
			const ctf_sect_t *strsect, size_t offset,
			int *errp)
{
  ctf_sect_t ctfsect;
  ctf_file_t *fp;

  ctf_dprintf ("ctf_arc_open_by_offset(%lu): opening\n", (unsigned long) offset);

  memset (&ctfsect, 0, sizeof (ctf_sect_t));

  offset += le64toh (arc->ctfa_ctfs);

  ctfsect.cts_name = _CTF_SECTION;
  ctfsect.cts_size = le64toh (*((uint64_t *) ((char *) arc + offset)));
  ctfsect.cts_entsize = 1;
  ctfsect.cts_data = (void *) ((char *) arc + offset + sizeof (uint64_t));
  fp = ctf_bufopen (&ctfsect, symsect, strsect, errp);
  if (fp)
    ctf_setmodel (fp, le64toh (arc->ctfa_model));
  return fp;
}

/* Return the number of members in an archive.  */
size_t
ctf_archive_count (const ctf_archive_t *wrapper)
{
  if (!wrapper->ctfi_is_archive)
    return 1;

  return wrapper->ctfi_archive->ctfa_nfiles;
}

/* Raw iteration over all CTF files in an archive.  We pass the raw data for all
   CTF files in turn to the specified callback function.  */
static int
ctf_archive_raw_iter_internal (const struct ctf_archive *arc,
			       ctf_archive_raw_member_f *func, void *data)
{
  int rc;
  size_t i;
  struct ctf_archive_modent *modent;
  const char *nametbl;

  modent = (ctf_archive_modent_t *) ((char *) arc
				     + sizeof (struct ctf_archive));
  nametbl = (((const char *) arc) + le64toh (arc->ctfa_names));

  for (i = 0; i < le64toh (arc->ctfa_nfiles); i++)
    {
      const char *name;
      char *fp;

      name = &nametbl[le64toh (modent[i].name_offset)];
      fp = ((char *) arc + le64toh (arc->ctfa_ctfs)
	    + le64toh (modent[i].ctf_offset));

      if ((rc = func (name, (void *) (fp + sizeof (uint64_t)),
		      le64toh (*((uint64_t *) fp)), data)) != 0)
	return rc;
    }
  return 0;
}

/* Raw iteration over all CTF files in an archive: public entry point.

   Returns -EINVAL if not supported for this sort of archive.  */
int
ctf_archive_raw_iter (const ctf_archive_t *arc,
		      ctf_archive_raw_member_f * func, void *data)
{
  if (arc->ctfi_is_archive)
    return ctf_archive_raw_iter_internal (arc->ctfi_archive, func, data);

  return -EINVAL;			 /* Not supported. */
}

/* Iterate over all CTF files in an archive.  We pass all CTF files in turn to
   the specified callback function.  */
static int
ctf_archive_iter_internal (const ctf_archive_t *wrapper,
			   const struct ctf_archive *arc,
			   const ctf_sect_t *symsect,
			   const ctf_sect_t *strsect,
			   ctf_archive_member_f *func, void *data)
{
  int rc;
  size_t i;
  ctf_file_t *f;
  struct ctf_archive_modent *modent;
  const char *nametbl;

  modent = (ctf_archive_modent_t *) ((char *) arc
				     + sizeof (struct ctf_archive));
  nametbl = (((const char *) arc) + le64toh (arc->ctfa_names));

  for (i = 0; i < le64toh (arc->ctfa_nfiles); i++)
    {
      const char *name;

      name = &nametbl[le64toh (modent[i].name_offset)];
      if ((f = ctf_arc_open_by_name_internal (arc, symsect, strsect,
					      name, &rc)) == NULL)
	return rc;

      f->ctf_archive = (ctf_archive_t *) wrapper;
      if ((rc = func (f, name, data)) != 0)
	{
	  ctf_file_close (f);
	  return rc;
	}

      ctf_file_close (f);
    }
  return 0;
}

/* Iterate over all CTF files in an archive: public entry point.  We pass all
   CTF files in turn to the specified callback function.  */
int
ctf_archive_iter (const ctf_archive_t *arc, ctf_archive_member_f *func,
		  void *data)
{
  const ctf_sect_t *symsect = &arc->ctfi_symsect;
  const ctf_sect_t *strsect = &arc->ctfi_strsect;

  if (symsect->cts_name == NULL)
    symsect = NULL;
  if (strsect->cts_name == NULL)
    strsect = NULL;

  if (arc->ctfi_is_archive)
    return ctf_archive_iter_internal (arc, arc->ctfi_archive, symsect, strsect,
				      func, data);

  return func (arc->ctfi_file, _CTF_SECTION, data);
}

/* Iterate over all CTF files in an archive, returning each dict in turn as a
   ctf_file_t, and NULL on error or end of iteration.  It is the caller's
   responsibility to close it.  Parent dicts may be skipped.  Regardless of
   whether they are skipped or not, the caller must ctf_import the parent if
   need be.

   We identify parents by name rather than by flag value: for now, with the
   linker only emitting parents named _CTF_SECTION, this works well enough.  */

ctf_file_t *
ctf_archive_next (const ctf_archive_t *wrapper, ctf_next_t **it, const char **name,
		  int skip_parent, int *errp)
{
  ctf_file_t *f;
  ctf_next_t *i = *it;
  struct ctf_archive *arc;
  struct ctf_archive_modent *modent;
  const char *nametbl;
  const char *name_;

  if (!i)
    {
      if ((i = ctf_next_create()) == NULL)
	{
	  if (errp)
	    *errp = ENOMEM;
	  return NULL;
	}
      i->cu.ctn_arc = wrapper;
      i->ctn_iter_fun = (void (*) (void)) ctf_archive_next;
      *it = i;
    }

  if ((void (*) (void)) ctf_archive_next != i->ctn_iter_fun)
    {
      if (errp)
	*errp = ECTF_NEXT_WRONGFUN;
      return NULL;
    }

  if (wrapper != i->cu.ctn_arc)
    {
      if (errp)
	*errp = ECTF_NEXT_WRONGFP;
      return NULL;
    }

  /* Iteration is made a bit more complex by the need to handle ctf_file_t's
     transparently wrapped in a single-member archive.  These are parents: if
     skip_parent is on, they are skipped and the iterator terminates
     immediately.  */

  if (!wrapper->ctfi_is_archive && i->ctn_n == 0)
    {
      i->ctn_n++;
      if (!skip_parent)
	{
	  wrapper->ctfi_file->ctf_refcnt++;
	  return wrapper->ctfi_file;
	}
    }

  arc = wrapper->ctfi_archive;

  /* The loop keeps going when skip_parent is on as long as the member we find
     is the parent (i.e. at most two iterations, but possibly an early return if
     *all* we have is a parent).  */

  const ctf_sect_t *symsect;
  const ctf_sect_t *strsect;

  do
    {
      if ((!wrapper->ctfi_is_archive) || (i->ctn_n >= le64toh (arc->ctfa_nfiles)))
	{
	  ctf_next_destroy (i);
	  *it = NULL;
	  if (errp)
	    *errp = ECTF_NEXT_END;
	  return NULL;
	}

      symsect = &wrapper->ctfi_symsect;
      strsect = &wrapper->ctfi_strsect;

      if (symsect->cts_name == NULL)
	symsect = NULL;
      if (strsect->cts_name == NULL)
	strsect = NULL;

      modent = (ctf_archive_modent_t *) ((char *) arc
					 + sizeof (struct ctf_archive));
      nametbl = (((const char *) arc) + le64toh (arc->ctfa_names));

      name_ = &nametbl[le64toh (modent[i->ctn_n].name_offset)];
      i->ctn_n++;
    } while (skip_parent && strcmp (name_, _CTF_SECTION) == 0);

  if (name)
    *name = name_;

  f = ctf_arc_open_by_name_internal (arc, symsect, strsect,
				     name_, errp);
  f->ctf_archive = (ctf_archive_t *) wrapper;
  return f;
}

#ifdef HAVE_MMAP
/* Map the header in.  Only used on new, empty files.  */
static void *arc_mmap_header (int fd, size_t headersz)
{
  void *hdr;
  if ((hdr = mmap (NULL, headersz, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
		   0)) == MAP_FAILED)
    return NULL;
  return hdr;
}

/* mmap() the whole file, for reading only.  (Map it writably, but privately: we
   need to modify the region, but don't need anyone else to see the
   modifications.)  */
static void *arc_mmap_file (int fd, size_t size)
{
  void *arc;
  if ((arc = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
		   fd, 0)) == MAP_FAILED)
    return NULL;
  return arc;
}

/* Persist the header to disk.  */
static int arc_mmap_writeout (int fd _libctf_unused_, void *header,
			      size_t headersz, const char **errmsg)
{
    if (msync (header, headersz, MS_ASYNC) < 0)
    {
      if (errmsg)
	*errmsg = N_("arc_mmap_writeout(): cannot sync after writing "
		     "to %s: %s");
      return -1;
    }
    return 0;
}

/* Unmap the region.  */
static int arc_mmap_unmap (void *header, size_t headersz, const char **errmsg)
{
  if (munmap (header, headersz) < 0)
    {
      if (errmsg)
	*errmsg = N_("arc_mmap_munmap(): cannot unmap after writing "
		     "to %s: %s");
      return -1;
    }
    return 0;
}
#else
/* Map the header in.  Only used on new, empty files.  */
static void *arc_mmap_header (int fd _libctf_unused_, size_t headersz)
{
  void *hdr;
  if ((hdr = malloc (headersz)) == NULL)
    return NULL;
  return hdr;
}

/* Pull in the whole file, for reading only.  We assume the current file
   position is at the start of the file.  */
static void *arc_mmap_file (int fd, size_t size)
{
  char *data;

  if ((data = malloc (size)) == NULL)
    return NULL;

  if (ctf_pread (fd, data, size, 0) < 0)
    {
      free (data);
      return NULL;
    }
  return data;
}

/* Persist the header to disk.  */
static int arc_mmap_writeout (int fd, void *header, size_t headersz,
			      const char **errmsg)
{
  ssize_t len;
  size_t acc = 0;
  char *data = (char *) header;
  ssize_t count = headersz;

  if ((lseek (fd, 0, SEEK_SET)) < 0)
    {
      if (errmsg)
	*errmsg = N_("arc_mmap_writeout(): cannot seek while writing header to "
		     "%s: %s");
      return -1;
    }

  while (headersz > 0)
    {
      if ((len = write (fd, data, count)) < 0)
	{
	  if (errmsg)
	    *errmsg = N_("arc_mmap_writeout(): cannot write header to %s: %s");
	  return len;
	}
      if (len == EINTR)
	continue;

      acc += len;
      if (len == 0)				/* EOF.  */
	break;

      count -= len;
      data += len;
    }
  return 0;
}

/* Unmap the region.  */
static int arc_mmap_unmap (void *header, size_t headersz _libctf_unused_,
			   const char **errmsg _libctf_unused_)
{
  free (header);
  return 0;
}
#endif
