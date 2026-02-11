/* gmon_io.c - Input and output from/to gmon.out files.

   Copyright (C) 1999-2026 Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#include "config.h"
#include "util.h"
#include "bfd.h"

#include "fopen-bin.h" /* maybe better sysdep.h ? */

#include "gp-gmon.h"

#include "binary-io.h"
#include "symtab.h"
#include "cg_arcs.h"
#include "basic_blocks.h"
#include "call_graph.h"
#include "gmon_io.h"
#include "gmon_out.h"
#include "gmon.h"		/* Fetch header for old format.  */
#include "hist.h"

enum gmon_ptr_size {
  ptr_32bit,
  ptr_64bit,
  ptr_unknown
};

enum gmon_ptr_signedness {
  ptr_signed,
  ptr_unsigned
};

static enum gmon_ptr_signedness gmon_get_ptr_signedness (void);

static int gmon_io_read_64 (FILE *, uint64_t *);

int gmon_input = 0;
int gmon_file_version = 0;	/* 0 == old (non-versioned) file format.  */

static enum gmon_ptr_size
gmon_get_ptr_size (const char *whoami)
{
  int size;

  /* Pick best size for pointers.  Start with the ELF size, and if not
     elf go with the architecture's address size.  */
  size = bfd_get_arch_size (core_bfd);
  if (size == -1)
    size = bfd_arch_bits_per_address (core_bfd);

  switch (size)
    {
    case 32:
      return ptr_32bit;

    case 64:
      return ptr_64bit;

    default:
      fprintf (stderr, "%s: address size has unexpected value of %u\n",
	       whoami, size);
      done (1);
    }
  return ptr_unknown;
}

static enum gmon_ptr_signedness
gmon_get_ptr_signedness (void)
{
  int sext;

  /* Figure out whether to sign extend.  If BFD doesn't know, assume no.  */
  sext = bfd_get_sign_extend_vma (core_bfd);
  if (sext == -1)
    return ptr_unsigned;
  return (sext ? ptr_signed : ptr_unsigned);
}

int
gmon_io_read_32 (FILE *ifp, unsigned int *valp)
{
  char buf[4];

  if (fread (buf, 1, 4, ifp) != 4)
    return 1;
  *valp = bfd_get_32 (core_bfd, buf);
  return 0;
}

static int
gmon_io_read_64 (FILE *ifp, uint64_t *valp)
{
  char buf[8];

  if (fread (buf, 1, 8, ifp) != 8)
    return 1;
  *valp = bfd_get_64 (core_bfd, buf);
  return 0;
}

int
gmon_io_read_vma (FILE *ifp, bfd_vma *valp, const char *whoami)
{
  unsigned int val32;
  uint64_t val64;

  switch (gmon_get_ptr_size (whoami))
    {
    case ptr_32bit:
      if (gmon_io_read_32 (ifp, &val32))
	return 1;
      if (gmon_get_ptr_signedness () == ptr_signed)
	*valp = (int) val32;
      else
	*valp = val32;
      break;

    case ptr_64bit:
      if (gmon_io_read_64 (ifp, &val64))
	return 1;
      if (gmon_get_ptr_signedness () == ptr_signed)
	*valp = (int64_t) val64;
      else
	*valp = val64;
      break;
    }
  return 0;
}

int
gmon_io_read (FILE *ifp, char *buf, size_t n)
{
  if (fread (buf, 1, n, ifp) != n)
    return 1;
  return 0;
}

int
gmon_out_read (const char *filename,
	       File_Format file_format, const char *whoami)
{
  FILE *ifp;
  struct gmon_hdr ghdr;
  unsigned char tag;
  int nhist = 0, narcs = 0, nbbs = 0;

  /* Open gmon.out file.  */
  if (strcmp (filename, "-") == 0)
    {
      ifp = stdin;
      SET_BINARY (fileno (stdin));
    }
  else
    {
      ifp = fopen (filename, FOPEN_RB);

      if (!ifp)
	{
	  perror (filename);
	  return -1;
	}
    }

  if (fread (&ghdr, sizeof (struct gmon_hdr), 1, ifp) != 1)
    {
      fprintf (stderr, "%s: file too short to be a gmon file\n",
	       filename);
      return -1;
    }

  if ((file_format == FF_MAGIC)
      || (file_format == FF_AUTO && !strncmp (&ghdr.cookie[0], GMON_MAGIC, 4)))
    {
      if (file_format == FF_MAGIC && strncmp (&ghdr.cookie[0], GMON_MAGIC, 4))
	{
	  fprintf (stderr, "%s: file `%s' has bad magic cookie\n",
		   whoami, filename);
	  return -1;
	}

      /* Right magic, so it's probably really a new gmon.out file.  */
      gmon_file_version = bfd_get_32 (core_bfd, (bfd_byte *) ghdr.version);

      if (gmon_file_version != GMON_VERSION && gmon_file_version != 0)
	{
	  fprintf (stderr,
		   "%s: file `%s' has unsupported version %d\n",
		   whoami, filename, gmon_file_version);
	  return -1;
	}

      /* Read in all the records.  */
      while (fread (&tag, sizeof (tag), 1, ifp) == 1)
	{
	  switch (tag)
	    {
	    case GMON_TAG_TIME_HIST:
	      ++nhist;
	      gmon_input |= INPUT_HISTOGRAM;
	      hist_read_rec (ifp, filename, whoami);
	      break;

	    case GMON_TAG_CG_ARC:
	      ++narcs;
	      gmon_input |= INPUT_CALL_GRAPH;
	      cg_read_rec (ifp, filename, whoami);
	      break;

	    case GMON_TAG_BB_COUNT:
	      ++nbbs;
	      gmon_input |= INPUT_BB_COUNTS;
	      bb_read_rec (ifp, filename, false, whoami);
	      break;

	    default:
	      fprintf (stderr,
		       "%s: %s: found bad tag %d (file corrupted?)\n",
		       whoami, filename, tag);
	      return -1;
	    }
	}
    }
  else
    {
      fprintf (stderr, "%s: don't know how to deal with file format %d\n",
	       whoami, file_format);
      return -1;
    }

  if (ifp != stdin)
    fclose (ifp);
  return 0;
}
