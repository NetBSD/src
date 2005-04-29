/* Pattern Matcher for Fixed String search.
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
#include "libgrep.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "exitfail.h"
#include "xalloc.h"
#include "m-common.h"

#if defined (STDC_HEADERS) || (!defined (isascii) && !defined (HAVE_ISASCII))
# define IN_CTYPE_DOMAIN(c) 1 
#else
# define IN_CTYPE_DOMAIN(c) isascii(c)
#endif
#define ISALNUM(C) (IN_CTYPE_DOMAIN (C) && isalnum (C))

static void *
Fcompile (const char *pattern, size_t pattern_size,
	  bool match_icase, bool match_words, bool match_lines,
	  char eolbyte)
{
  struct compiled_kwset *ckwset;
  const char *beg, *lim, *err;

  ckwset = (struct compiled_kwset *) xmalloc (sizeof (struct compiled_kwset));
  kwsinit (ckwset, match_icase, match_words, match_lines, eolbyte);

  beg = pattern;
  do
    {
      for (lim = beg; lim < pattern + pattern_size && *lim != '\n'; ++lim)
	;
      if ((err = kwsincr (ckwset->kwset, beg, lim - beg)) != NULL)
	error (exit_failure, 0, err);
      if (lim < pattern + pattern_size)
	++lim;
      beg = lim;
    }
  while (beg < pattern + pattern_size);

  if ((err = kwsprep (ckwset->kwset)) != NULL)
    error (exit_failure, 0, err);
  return ckwset;
}

static size_t
Fexecute (const void *compiled_pattern, const char *buf, size_t buf_size,
	  size_t *match_size, bool exact)
{
  struct compiled_kwset *ckwset = (struct compiled_kwset *) compiled_pattern;
  register const char *beg, *try, *end;
  register size_t len;
  char eol = ckwset->eolbyte;
  struct kwsmatch kwsmatch;
#ifdef MBS_SUPPORT
  char *mb_properties;
  if (MB_CUR_MAX > 1)
    mb_properties = check_multibyte_string (buf, buf_size);
#endif /* MBS_SUPPORT */

  for (beg = buf; beg <= buf + buf_size; ++beg)
    {
      size_t offset =
	kwsexec (ckwset->kwset, beg, buf + buf_size - beg, &kwsmatch);
      if (offset == (size_t) -1)
	{
#ifdef MBS_SUPPORT
	  if (MB_CUR_MAX > 1)
	    free (mb_properties);
#endif /* MBS_SUPPORT */
	  return offset;
	}
#ifdef MBS_SUPPORT
      if (MB_CUR_MAX > 1 && mb_properties[offset+beg-buf] == 0)
	continue; /* It is a part of multibyte character.  */
#endif /* MBS_SUPPORT */
      beg += offset;
      len = kwsmatch.size[0];
      if (exact)
	{
	  *match_size = len;
#ifdef MBS_SUPPORT
	  if (MB_CUR_MAX > 1)
	    free (mb_properties);
#endif /* MBS_SUPPORT */
	  return beg - buf;
	}
      if (ckwset->match_lines)
	{
	  if (beg > buf && beg[-1] != eol)
	    continue;
	  if (beg + len < buf + buf_size && beg[len] != eol)
	    continue;
	  goto success;
	}
      else if (ckwset->match_words)
	for (try = beg; len; )
	  {
	    if (try > buf && IS_WORD_CONSTITUENT ((unsigned char) try[-1]))
	      break;
	    if (try + len < buf + buf_size
		&& IS_WORD_CONSTITUENT ((unsigned char) try[len]))
	      {
		offset = kwsexec (ckwset->kwset, beg, --len, &kwsmatch);
		if (offset == (size_t) -1)
		  {
#ifdef MBS_SUPPORT
		    if (MB_CUR_MAX > 1)
		      free (mb_properties);
#endif /* MBS_SUPPORT */
		    return offset;
		  }
		try = beg + offset;
		len = kwsmatch.size[0];
	      }
	    else
	      goto success;
	  }
      else
	goto success;
    }

#ifdef MBS_SUPPORT
  if (MB_CUR_MAX > 1)
    free (mb_properties);
#endif /* MBS_SUPPORT */
  return -1;

 success:
  end = memchr (beg + len, eol, (buf + buf_size) - (beg + len));
  end++;
  while (buf < beg && beg[-1] != eol)
    --beg;
  *match_size = end - beg;
#ifdef MBS_SUPPORT
  if (MB_CUR_MAX > 1)
    free (mb_properties);
#endif /* MBS_SUPPORT */
  return beg - buf;
}

static void
Ffree (void *compiled_pattern)
{
  struct compiled_kwset *ckwset = (struct compiled_kwset *) compiled_pattern;

  free (ckwset->trans);
  free (ckwset);
}

matcher_t matcher_fgrep =
  {
    Fcompile,
    Fexecute,
    Ffree
  };

