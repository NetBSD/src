/* GNU gettext - internationalization aids
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.

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
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _PO_LEX_H
#define _PO_LEX_H

#include <sys/types.h>
#include "error.h"

typedef struct lex_pos_ty lex_pos_ty;
struct lex_pos_ty
{
  char *file_name;
  size_t line_number;
};


/* Global variables from po-lex.c.  */
extern lex_pos_ty gram_pos;
extern size_t gram_max_allowed_errors;


void lex_open PARAMS ((const char *__fname));
void lex_close PARAMS ((void));
int po_gram_lex PARAMS ((void));
void po_lex_pass_comments PARAMS ((int __flag));
void po_lex_pass_obsolete_entries PARAMS ((int __flag));


/* GCC is smart enough to allow optimizations like this.  */
#if __STDC__ && defined __GNUC__ && __GNUC__ >= 2


/* CAUTION: If you change this macro, you must also make identical
   changes to the function of the same name in src/po-lex.c  */

# define po_gram_error(fmt, args...)					    \
  do {									    \
    error_at_line (0, 0, gram_pos.file_name, gram_pos.line_number,	    \
		    fmt, ## args);					    \
    if (*fmt == '.')							    \
      --error_message_count;						    \
    else if (error_message_count >= gram_max_allowed_errors)			    \
      error (1, 0, _("too many errors, aborting"));			    \
  } while (0)


/* CAUTION: If you change this macro, you must also make identical
   changes to the function of the same name in src/po-lex.c  */

# define gram_error_at_line(pos, fmt, args...)				    \
  do {									    \
    error_at_line (0, 0, (pos)->file_name, (pos)->line_number,		    \
		    fmt, ## args);					    \
    if (*fmt == '.')							    \
      --error_message_count;						    \
    else if (error_message_count >= gram_max_allowed_errors)		    \
      error (1, 0, _("too many errors, aborting"));			    \
  } while (0)
#else
void po_gram_error PARAMS ((const char *__fmt, ...));
void gram_error_at_line PARAMS ((const lex_pos_ty *__pos, const char *__fmt,
				 ...));
#endif


#endif
