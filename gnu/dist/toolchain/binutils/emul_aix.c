/* Binutils emulation layer.
   Copyright (C) 2002 Free Software Foundation, Inc.
   Written by Tom Rix, Redhat.

   This file is part of GNU Binutils.

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

#include "binemul.h"
#include "bfdlink.h"
#include "coff/internal.h"
#include "coff/xcoff.h"
#include "libcoff.h"
#include "libxcoff.h"

/* Default to <bigaf>.  */
static boolean big_archive = true;

/* Whether to include 32 bit objects.  */
static boolean X32 = true;

/* Whether to include 64 bit objects.  */
static boolean X64 = false;

static void    ar_emul_aix_usage     PARAMS ((FILE *));
static boolean ar_emul_aix_append    PARAMS ((bfd **, char *, boolean));
static boolean ar_emul_aix5_append   PARAMS ((bfd **, char *, boolean));
static boolean ar_emul_aix_replace   PARAMS ((bfd **, char *, boolean));
static boolean ar_emul_aix5_replace  PARAMS ((bfd **, char *, boolean));
static boolean ar_emul_aix_parse_arg PARAMS ((char *));
static boolean ar_emul_aix_internal  PARAMS ((bfd **, char *, boolean,
					      const char *, boolean));

static void
ar_emul_aix_usage (fp)
     FILE *fp;
{
  AR_EMUL_USAGE_PRINT_OPTION_HEADER (fp);
  /* xgettext:c-format */
  fprintf (fp, _("  [-g]         - 32 bit small archive\n"));
  fprintf (fp, _("  [-X32]       - ignores 64 bit objects\n"));
  fprintf (fp, _("  [-X64]       - ignores 32 bit objects\n"));
  fprintf (fp, _("  [-X32_64]    - accepts 32 and 64 bit objects\n"));
}

static boolean
ar_emul_aix_internal (after_bfd, file_name, verbose, target_name, is_append)
     bfd **after_bfd;
     char *file_name;
     boolean verbose;
     const char * target_name;
     boolean is_append;
{
  bfd *temp;
  bfd *try_bfd;

  temp = *after_bfd;

  /* Try 64 bit.  */
  try_bfd = bfd_openr (file_name, target_name);

  /* Failed or the object is possibly 32 bit.  */
  if (NULL == try_bfd || ! bfd_check_format (try_bfd, bfd_object))
    try_bfd = bfd_openr (file_name, "aixcoff-rs6000");

  AR_EMUL_ELEMENT_CHECK (try_bfd, file_name);

  if (bfd_xcoff_is_xcoff64 (try_bfd) && (! X64))
    return false;

  if (bfd_xcoff_is_xcoff32 (try_bfd)
      && bfd_check_format (try_bfd, bfd_object) && (! X32))
    return false;

  if (is_append)
    {
      AR_EMUL_APPEND_PRINT_VERBOSE (verbose, file_name);
    }
  else
    {
      AR_EMUL_REPLACE_PRINT_VERBOSE (verbose, file_name);
    }

  *after_bfd = try_bfd;
  (*after_bfd)->next = temp;

  return true;
}


static boolean
ar_emul_aix_append (after_bfd, file_name, verbose)
     bfd **after_bfd;
     char *file_name;
     boolean verbose;
{
  return ar_emul_aix_internal (after_bfd, file_name, verbose,
			       "aixcoff64-rs6000", true);
}

static boolean
ar_emul_aix5_append (after_bfd, file_name, verbose)
     bfd **after_bfd;
     char *file_name;
     boolean verbose;
{
  return ar_emul_aix_internal (after_bfd, file_name, verbose,
			       "aix5coff64-rs6000", true);
}

static boolean
ar_emul_aix_replace (after_bfd, file_name, verbose)
     bfd **after_bfd;
     char *file_name;
     boolean verbose;
{
  return ar_emul_aix_internal (after_bfd, file_name, verbose,
			       "aixcoff64-rs6000", false);
}

static boolean
ar_emul_aix5_replace (after_bfd, file_name, verbose)
     bfd **after_bfd;
     char *file_name;
     boolean verbose;
{
  return ar_emul_aix_internal (after_bfd, file_name, verbose,
			       "aix5coff64-rs6000", false);
}

boolean
ar_emul_aix_create (abfd_out, archive_file_name, file_name)
     bfd **abfd_out;
     char *archive_file_name;
     char *file_name ATTRIBUTE_UNUSED;
{
  char *target = "aixcoff-rs6000";

  /* Create an empty archive.  */
  *abfd_out = bfd_openw (archive_file_name, target);

  if (*abfd_out == NULL)
    bfd_fatal (archive_file_name);

  /* set to small or big format.  */
  /* not done.  */
  return true;
}

static boolean
ar_emul_aix_parse_arg (arg)
     char *arg;
{
  if (strncmp (arg, "-X32_64", 6) == 0)
    {
      big_archive = true;
      X32 = true;
      X64 = true;
    }
  else if (strncmp (arg, "-X32", 3) == 0)
    {
      big_archive = true;
      X32 = true;
      X64 = false;
    }
  else if (strncmp (arg, "-X64", 3) == 0)
    {
      big_archive = true;
      X32 = false;
      X64 = true;
    }
  else if (strncmp (arg, "-g", 2) == 0)
    {
      big_archive = false;
      X32 = true;
      X64 = false;
    }
  else
    return false;

  return true;
}

struct bin_emulation_xfer_struct bin_aix_emulation =
{
  ar_emul_aix_usage,
  ar_emul_aix_append,
  ar_emul_aix_replace,
  ar_emul_default_create,
  ar_emul_aix_parse_arg,
};

struct bin_emulation_xfer_struct bin_aix5_emulation =
{
  ar_emul_aix_usage,
  ar_emul_aix5_append,
  ar_emul_aix5_replace,
  ar_emul_default_create,
  ar_emul_aix_parse_arg,
};
