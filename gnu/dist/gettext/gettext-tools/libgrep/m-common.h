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

#include <stdbool.h>
#include <stddef.h>
#include <limits.h>
#include "kwset.h"

#if defined HAVE_WCTYPE_H && defined HAVE_WCHAR_H && defined HAVE_MBRTOWC
/* We can handle multibyte string.  */
# define MBS_SUPPORT
# include <wchar.h>
# include <wctype.h>
#endif

#define NCHAR (UCHAR_MAX + 1)

struct compiled_kwset {
  kwset_t kwset;
  char *trans;
  bool match_words;
  bool match_lines;
  char eolbyte;
};

extern void
       kwsinit (struct compiled_kwset *ckwset,
		bool match_icase, bool match_words, bool match_lines,
		char eolbyte);

#ifdef MBS_SUPPORT
extern char*
       check_multibyte_string (const char *buf, size_t buf_size);
#endif

#define IS_WORD_CONSTITUENT(C) (ISALNUM(C) || (C) == '_')
