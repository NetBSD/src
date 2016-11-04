/* Binutils emulation layer.
   Copyright (C) 2002-2016 Free Software Foundation, Inc.
   Written by Tom Rix, Red Hat Inc.

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
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "binemul.h"
#include "bfdlink.h"
#include "coff/internal.h"
#include "coff/xcoff.h"
#include "libcoff.h"
#include "libxcoff.h"

/* Default to <bigaf>.  */
/* FIXME: write only variable.  */
static bfd_boolean big_archive = TRUE;

/* Whether to include 32 bit objects.  */
static bfd_boolean X32 = TRUE;

/* Whether to include 64 bit objects.  */
static bfd_boolean X64 = FALSE;

static void
ar_emul_aix_usage (FILE *fp)
{
  AR_EMUL_USAGE_PRINT_OPTION_HEADER (fp);
  /* xgettext:c-format */
  fprintf (fp, _("  [-g]         - 32 bit small archive\n"));
  fprintf (fp, _("  [-X32]       - ignores 64 bit objects\n"));
  fprintf (fp, _("  [-X64]       - ignores 32 bit objects\n"));
  fprintf (fp, _("  [-X32_64]    - accepts 32 and 64 bit objects\n"));
}

static bfd_boolean
check_aix (bfd *try_bfd)
{
  extern const bfd_target rs6000_xcoff_vec;
  extern const bfd_target rs6000_xcoff64_vec;
  extern const bfd_target rs6000_xcoff64_aix_vec;

  if (bfd_check_format (try_bfd, bfd_object))
    {
      if (!X32 && try_bfd->xvec == &rs6000_xcoff_vec)
	return FALSE;

      if (!X64 && (try_bfd->xvec == &rs6000_xcoff64_vec
		   || try_bfd->xvec == &rs6000_xcoff64_aix_vec))
	return FALSE;
    }
  return TRUE;
}

static bfd_boolean
ar_emul_aix_append (bfd **after_bfd, char *file_name, const char *target,
		    bfd_boolean verbose, bfd_boolean flatten)
{
  bfd *new_bfd;

  new_bfd = bfd_openr (file_name, target);
  AR_EMUL_ELEMENT_CHECK (new_bfd, file_name);

  return do_ar_emul_append (after_bfd, new_bfd, verbose, flatten, check_aix);
}

static bfd_boolean
ar_emul_aix_replace (bfd **after_bfd, char *file_name, const char *target,
		     bfd_boolean verbose)
{
  bfd *new_bfd;

  new_bfd = bfd_openr (file_name, target);
  AR_EMUL_ELEMENT_CHECK (new_bfd, file_name);

  if (!check_aix (new_bfd))
    return FALSE;

  AR_EMUL_REPLACE_PRINT_VERBOSE (verbose, file_name);

  new_bfd->archive_next = *after_bfd;
  *after_bfd = new_bfd;

  return TRUE;
}

static bfd_boolean
ar_emul_aix_parse_arg (char *arg)
{
  if (CONST_STRNEQ (arg, "-X32_64"))
    {
      big_archive = TRUE;
      X32 = TRUE;
      X64 = TRUE;
    }
  else if (CONST_STRNEQ (arg, "-X32"))
    {
      big_archive = TRUE;
      X32 = TRUE;
      X64 = FALSE;
    }
  else if (CONST_STRNEQ (arg, "-X64"))
    {
      big_archive = TRUE;
      X32 = FALSE;
      X64 = TRUE;
    }
  else if (CONST_STRNEQ (arg, "-g"))
    {
      big_archive = FALSE;
      X32 = TRUE;
      X64 = FALSE;
    }
  else
    return FALSE;

  return TRUE;
}

struct bin_emulation_xfer_struct bin_aix_emulation =
{
  ar_emul_aix_usage,
  ar_emul_aix_append,
  ar_emul_aix_replace,
  ar_emul_aix_parse_arg,
};
