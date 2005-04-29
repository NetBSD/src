/* Pattern Matchers - Common Utilities.
   Copyright (C) 1992, 1998, 2000, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include "m-common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "exitfail.h"
#include "xalloc.h"
#include "gettext.h"
#define _(str) gettext (str)

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1 
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif
#define ISUPPER(C) (IN_CTYPE_DOMAIN (C) && isupper (C))
#define TOLOWER(C) (ISUPPER(C) ? tolower(C) : (C))

void
kwsinit (struct compiled_kwset *ckwset,
	 bool match_icase, bool match_words, bool match_lines, char eolbyte)
{
  if (match_icase)
    {
      int i;

      ckwset->trans = (char *) xmalloc (NCHAR * sizeof (char));
      for (i = 0; i < NCHAR; i++)
	ckwset->trans[i] = TOLOWER (i);
      ckwset->kwset = kwsalloc (ckwset->trans);
    }
  else
    {
      ckwset->trans = NULL;
      ckwset->kwset = kwsalloc (NULL);
    }
  if (ckwset->kwset == NULL)
    error (exit_failure, 0, _("memory exhausted"));
  ckwset->match_words = match_words;
  ckwset->match_lines = match_lines;
  ckwset->eolbyte = eolbyte;
}

#ifdef MBS_SUPPORT
/* This function allocate the array which correspond to "buf".
   Then this check multibyte string and mark on the positions which
   are not singlebyte character nor the first byte of a multibyte
   character.  Caller must free the array.  */
char*
check_multibyte_string (const char *buf, size_t buf_size)
{
  char *mb_properties = (char *) malloc (buf_size);
  mbstate_t cur_state;
  int i;

  memset (&cur_state, 0, sizeof (mbstate_t));
  memset (mb_properties, 0, sizeof (char) * buf_size);
  for (i = 0; i < buf_size ;)
    {
      size_t mbclen;
      mbclen = mbrlen (buf + i, buf_size - i, &cur_state);

      if (mbclen == (size_t) -1 || mbclen == (size_t) -2 || mbclen == 0)
	{
	  /* An invalid sequence, or a truncated multibyte character.
	     We treat it as a singlebyte character.  */
	  mbclen = 1;
	}
      mb_properties[i] = mbclen;
      i += mbclen;
    }

  return mb_properties;
}
#endif
