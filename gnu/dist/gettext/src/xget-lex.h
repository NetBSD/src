/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1996, 1998 Free Software Foundation, Inc.

   This file was written by Peter Miller <millerp@canb.auug.org.au>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef SRC_XGET_LEX_H
#define SRC_XGET_LEX_H

enum xgettext_token_type_ty
{
  xgettext_token_type_eof,
  xgettext_token_type_keyword1,
  xgettext_token_type_keyword2,
  xgettext_token_type_lp,
  xgettext_token_type_comma,
  xgettext_token_type_string_literal,
  xgettext_token_type_symbol
};
typedef enum xgettext_token_type_ty xgettext_token_type_ty;

typedef struct xgettext_token_ty xgettext_token_ty;
struct xgettext_token_ty
{
  xgettext_token_type_ty type;

  /* These 3 are only set for xgettext_token_type_string_literal.  */
  char *string;
  int line_number;
  char *file_name;
};


void xgettext_lex_open PARAMS ((const char *__file_name));
void xgettext_lex_close PARAMS ((void));
void xgettext_lex PARAMS ((xgettext_token_ty *__tp));
const char *xgettext_lex_comment PARAMS ((size_t __n));
void xgettext_lex_comment_reset PARAMS ((void));
/* void xgettext_lex_filepos PARAMS ((char **, int *)); FIXME needed?  */
void xgettext_lex_keyword PARAMS ((char *__name));
void xgettext_lex_cplusplus PARAMS ((void));
void xgettext_lex_trigraphs PARAMS ((void));

#endif
