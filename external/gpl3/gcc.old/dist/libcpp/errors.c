/* Default error handlers for CPP Library.
   Copyright (C) 1986, 1987, 1989, 1992, 1993, 1994, 1995, 1998, 1999, 2000,
   2001, 2002, 2004, 2008, 2009 Free Software Foundation, Inc.
   Written by Per Bothner, 1994.
   Based on CCCP program by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

#include "config.h"
#include "system.h"
#include "cpplib.h"
#include "internal.h"

/* Print an error at the location of the previously lexed token.  */
bool
cpp_error (cpp_reader * pfile, int level, const char *msgid, ...)
{
  source_location src_loc;
  va_list ap;
  bool ret;

  va_start (ap, msgid);

  if (CPP_OPTION (pfile, traditional))
    {
      if (pfile->state.in_directive)
	src_loc = pfile->directive_line;
      else
	src_loc = pfile->line_table->highest_line;
    }
  /* We don't want to refer to a token before the beginning of the
     current run -- that is invalid.  */
  else if (pfile->cur_token == pfile->cur_run->base)
    {
      if (pfile->cur_run->prev != NULL)
	src_loc = pfile->cur_run->prev->limit->src_loc;
      else
	src_loc = 0;
    }
  else
    {
      src_loc = pfile->cur_token[-1].src_loc;
    }

  if (!pfile->cb.error)
    abort ();
  ret = pfile->cb.error (pfile, level, src_loc, 0, _(msgid), &ap);

  va_end (ap);
  return ret;
}

/* Print an error at a specific location.  */
bool
cpp_error_with_line (cpp_reader *pfile, int level,
		     source_location src_loc, unsigned int column,
		     const char *msgid, ...)
{
  va_list ap;
  bool ret;
  
  va_start (ap, msgid);

  if (!pfile->cb.error)
    abort ();
  ret = pfile->cb.error (pfile, level, src_loc, column, _(msgid), &ap);

  va_end (ap);
  return ret;
}

bool
cpp_errno (cpp_reader *pfile, int level, const char *msgid)
{
  if (msgid[0] == '\0')
    msgid = _("stdout");

  return cpp_error (pfile, level, "%s: %s", msgid, xstrerror (errno));
}
