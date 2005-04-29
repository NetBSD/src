/* GNU gettext - internationalization aids
   Copyright (C) 1995-1998, 2000-2004 Free Software Foundation, Inc.

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
#include <stdio.h>
#include <stdbool.h>
#include "error.h"
#include "error-progname.h"
#include "xerror.h"
#include "po-error.h"
#include "pos.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Lexical analyzer for reading PO files.  */


/* Global variables from po-lex.c.  */

/* Current position within the PO file.  */
extern DLL_VARIABLE lex_pos_ty gram_pos;
extern DLL_VARIABLE int gram_pos_column;

/* Number of parse errors within a PO file that cause the program to
   terminate.  Cf. error_message_count, declared in <error.h>.  */
extern DLL_VARIABLE unsigned int gram_max_allowed_errors;

/* True if obsolete entries shall be considered as valid.  */
extern DLL_VARIABLE bool pass_obsolete_entries;


/* Prepare lexical analysis.  */
extern void lex_start (FILE *fp, const char *real_filename,
		       const char *logical_filename);

/* Terminate lexical analysis.  */
extern void lex_end (void);

/* Return the next token in the PO file.  The return codes are defined
   in "po-gram-gen2.h".  Associated data is put in 'po_gram_lval.  */
extern int po_gram_lex (void);

/* po_gram_lex() can return comments as COMMENT.  Switch this on or off.  */
extern void po_lex_pass_comments (bool flag);

/* po_gram_lex() can return obsolete entries as if they were normal entries.
   Switch this on or off.  */
extern void po_lex_pass_obsolete_entries (bool flag);


/* ISO C 99 is smart enough to allow optimizations like this.
   Note: OpenVMS 7.3 cc pretends to support ISO C 99 but chokes on '...'.  */
#if __STDC__ && (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L && !defined __DECC)

/* CAUTION: If you change this macro, you must also make identical
   changes to the function of the same name in src/po-lex.c  */

# define po_gram_error(fmt, ...)					    \
  do {									    \
    char *totalfmt = xasprintf ("%s%s", "%s:%lu:%d: ", fmt);		    \
    error_with_progname = false;					    \
    po_error (0, 0, totalfmt, gram_pos.file_name,			    \
	      (unsigned long) gram_pos.line_number, gram_pos_column + 1,    \
	      __VA_ARGS__ + 0);						    \
    error_with_progname = true;						    \
    free (totalfmt);							    \
    if (*fmt == '.')							    \
      --error_message_count;						    \
    else if (error_message_count >= gram_max_allowed_errors)		    \
      po_error (1, 0, _("too many errors, aborting"));			    \
  } while (0)

/* CAUTION: If you change this macro, you must also make identical
   changes to the function of the same name in src/po-lex.c  */

# define po_gram_error_at_line(pos, fmt, ...)				    \
  do {									    \
    error_with_progname = false;					    \
    po_error_at_line (0, 0, (pos)->file_name, (pos)->line_number,	    \
		      fmt, __VA_ARGS__ + 0);				    \
    error_with_progname = true;						    \
    if (*fmt == '.')							    \
      --error_message_count;						    \
    else if (error_message_count >= gram_max_allowed_errors)		    \
      po_error (1, 0, _("too many errors, aborting"));			    \
  } while (0)

/* GCC is also smart enough to allow optimizations like this.  */
#elif __STDC__ && defined __GNUC__ && __GNUC__ >= 2 && !defined __APPLE_CC__

/* CAUTION: If you change this macro, you must also make identical
   changes to the function of the same name in src/po-lex.c  */

# define po_gram_error(fmt, args...)					    \
  do {									    \
    char *totalfmt = xasprintf ("%s%s", "%s:%d:%d: ", fmt);		    \
    error_with_progname = false;					    \
    po_error (0, 0, totalfmt, gram_pos.file_name, gram_pos.line_number,	    \
	      gram_pos_column + 1 , ## args);				    \
    error_with_progname = true;						    \
    free (totalfmt);							    \
    if (*fmt == '.')							    \
      --error_message_count;						    \
    else if (error_message_count >= gram_max_allowed_errors)		    \
      po_error (1, 0, _("too many errors, aborting"));			    \
  } while (0)

/* CAUTION: If you change this macro, you must also make identical
   changes to the function of the same name in src/po-lex.c  */

# define po_gram_error_at_line(pos, fmt, args...)			    \
  do {									    \
    error_with_progname = false;					    \
    po_error_at_line (0, 0, (pos)->file_name, (pos)->line_number,	    \
		      fmt , ## args);					    \
    error_with_progname = true;						    \
    if (*fmt == '.')							    \
      --error_message_count;						    \
    else if (error_message_count >= gram_max_allowed_errors)		    \
      po_error (1, 0, _("too many errors, aborting"));			    \
  } while (0)

#else
extern void po_gram_error (const char *fmt, ...);
extern void po_gram_error_at_line (const lex_pos_ty *pos, const char *fmt, ...);
#endif


/* Contains information about the definition of one translation.  */
struct msgstr_def
{
  char *msgstr;
  size_t msgstr_len;
};


#ifdef __cplusplus
}
#endif


#endif
