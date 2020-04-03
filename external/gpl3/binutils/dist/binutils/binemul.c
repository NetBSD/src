/* Binutils emulation layer.
   Copyright (C) 2002-2020 Free Software Foundation, Inc.
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

extern bin_emulation_xfer_type bin_dummy_emulation;

void
ar_emul_usage (FILE *fp)
{
  if (bin_dummy_emulation.ar_usage)
    bin_dummy_emulation.ar_usage (fp);
}

void
ar_emul_default_usage (FILE *fp)
{
  AR_EMUL_USAGE_PRINT_OPTION_HEADER (fp);
  /* xgettext:c-format */
  fprintf (fp, _("  No emulation specific options\n"));
}

bfd_boolean
ar_emul_append (bfd **after_bfd, char *file_name, const char *target,
		bfd_boolean verbose, bfd_boolean flatten)
{
  if (bin_dummy_emulation.ar_append)
    return bin_dummy_emulation.ar_append (after_bfd, file_name, target,
					  verbose, flatten);

  return FALSE;
}

static bfd_boolean
any_ok (bfd *new_bfd ATTRIBUTE_UNUSED)
{
  return TRUE;
}

bfd_boolean
do_ar_emul_append (bfd **after_bfd, bfd *new_bfd,
		   bfd_boolean verbose, bfd_boolean flatten,
		   bfd_boolean (*check) (bfd *))
{
  /* When flattening, add the members of an archive instead of the
     archive itself.  */
  if (flatten && bfd_check_format (new_bfd, bfd_archive))
    {
      bfd *elt;
      bfd_boolean added = FALSE;

      for (elt = bfd_openr_next_archived_file (new_bfd, NULL);
           elt;
           elt = bfd_openr_next_archived_file (new_bfd, elt))
        {
          if (do_ar_emul_append (after_bfd, elt, verbose, TRUE, check))
            {
              added = TRUE;
              after_bfd = &((*after_bfd)->archive_next);
            }
        }

      return added;
    }

  if (!check (new_bfd))
    return FALSE;

  AR_EMUL_APPEND_PRINT_VERBOSE (verbose, new_bfd->filename);

  new_bfd->archive_next = *after_bfd;
  *after_bfd = new_bfd;

  return TRUE;
}

bfd_boolean
ar_emul_default_append (bfd **after_bfd, char *file_name,
			const char *target, bfd_boolean verbose,
			bfd_boolean flatten)
{
  bfd *new_bfd;

  new_bfd = bfd_openr (file_name, target);
  AR_EMUL_ELEMENT_CHECK (new_bfd, file_name);
  return do_ar_emul_append (after_bfd, new_bfd, verbose, flatten, any_ok);
}

bfd_boolean
ar_emul_replace (bfd **after_bfd, char *file_name, const char *target,
		 bfd_boolean verbose)
{
  if (bin_dummy_emulation.ar_replace)
    return bin_dummy_emulation.ar_replace (after_bfd, file_name,
					   target, verbose);

  return FALSE;
}

bfd_boolean
ar_emul_default_replace (bfd **after_bfd, char *file_name,
			 const char *target, bfd_boolean verbose)
{
  bfd *new_bfd;

  new_bfd = bfd_openr (file_name, target);
  AR_EMUL_ELEMENT_CHECK (new_bfd, file_name);

  AR_EMUL_REPLACE_PRINT_VERBOSE (verbose, file_name);

  new_bfd->archive_next = *after_bfd;
  *after_bfd = new_bfd;

  return TRUE;
}

bfd_boolean
ar_emul_parse_arg (char *arg)
{
  if (bin_dummy_emulation.ar_parse_arg)
    return bin_dummy_emulation.ar_parse_arg (arg);

  return FALSE;
}

bfd_boolean
ar_emul_default_parse_arg (char *arg ATTRIBUTE_UNUSED)
{
  return FALSE;
}
