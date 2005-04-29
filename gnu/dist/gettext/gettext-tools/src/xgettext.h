/* xgettext common functions.
   Copyright (C) 2001-2003 Free Software Foundation, Inc.
   Written by Peter Miller <millerp@canb.auug.org.au>
   and Bruno Haible <haible@clisp.cons.org>, 2001.

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

#ifndef _XGETTEXT_H
#define _XGETTEXT_H

#include <stddef.h>
#include <stdlib.h>

#if HAVE_ICONV
#include <iconv.h>
#endif

#include "message.h"
#include "pos.h"
#include "str-list.h"

/* Declare 'line_comment' and 'input_syntax'.  */
#include "read-po.h"


#ifdef __cplusplus
extern "C" {
#endif


/* If true, omit the header entry.
   If false, keep the header entry present in the input.  */
extern int xgettext_omit_header;

extern bool substring_match;


/* Split keyword spec into keyword, argnum1, argnum2.  */
extern void split_keywordspec (const char *spec, const char **endp,
			       int *argnum1p, int *argnum2p);


/* Context representing some flags.  */
typedef struct flag_context_ty flag_context_ty;
struct flag_context_ty
{
  /* Regarding the primary formatstring type.  */
  /*enum is_format*/ unsigned int is_format1    : 3;
  /*bool*/           unsigned int pass_format1  : 1;
  /* Regarding the secondary formatstring type.  */
  /*enum is_format*/ unsigned int is_format2    : 3;
  /*bool*/           unsigned int pass_format2  : 1;
};
/* Null context.  */
extern flag_context_ty null_context;
/* Transparent context.  */
extern flag_context_ty passthrough_context;
/* Compute an inherited context.
   The outer_context is assumed to have all pass_format* flags = false.
   The result will then also have all pass_format* flags = false.  */
extern flag_context_ty
       inherited_context (flag_context_ty outer_context,
			  flag_context_ty modifier_context);

/* Context representing some flags, for each possible argument number.
   This is a linked list, sorted according to the argument number.  */
typedef struct flag_context_list_ty flag_context_list_ty;
struct flag_context_list_ty
{
  int argnum;			/* current argument number, > 0 */
  flag_context_ty flags;	/* flags for current argument */
  flag_context_list_ty *next;
};

/* Iterator through a flag_context_list_ty.  */
typedef struct flag_context_list_iterator_ty flag_context_list_iterator_ty;
struct flag_context_list_iterator_ty
{
  int argnum;				/* current argument number, > 0 */
  const flag_context_list_ty* head;	/* tail of list */
};
extern flag_context_list_iterator_ty null_context_list_iterator;
extern flag_context_list_iterator_ty passthrough_context_list_iterator;
extern flag_context_list_iterator_ty
       flag_context_list_iterator (flag_context_list_ty *list);
extern flag_context_ty
       flag_context_list_iterator_advance (flag_context_list_iterator_ty *iter);

/* For nearly each backend, we have a separate table mapping a keyword to
   a flag_context_list_ty *.  */
typedef hash_table /* char[] -> flag_context_list_ty * */
        flag_context_list_table_ty;
extern flag_context_list_ty *
       flag_context_list_table_lookup (flag_context_list_table_ty *flag_table,
				       const void *key, size_t keylen);
/* Record a flag in the appropriate backend's table.  */
extern void xgettext_record_flag (const char *optionstring);


/* Canonicalized encoding name for all input files.  */
extern const char *xgettext_global_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_global_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
extern iconv_t xgettext_global_source_iconv;
#endif

/* Canonicalized encoding name for the current input file.  */
extern const char *xgettext_current_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_current_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
extern iconv_t xgettext_current_source_iconv;
#endif

/* Convert the given string from xgettext_current_source_encoding to
   the output file encoding (i.e. ASCII or UTF-8).
   The resulting string is either the argument string, or freshly allocated.
   The file_name and line_number are only used for error message purposes.  */
extern char *from_current_source_encoding (const char *string,
					   const char *file_name,
					   size_t line_number);


/* List of messages whose msgids must not be extracted, or NULL.
   Used by remember_a_message().  */
extern message_list_ty *exclude;


/* Comment handling: There is a list of automatic comments that may be appended
   to the next message.  Used by remember_a_message().  */
extern void xgettext_comment_add (const char *str);
extern const char *xgettext_comment (size_t n);
extern void xgettext_comment_reset (void);


/* Comment handling for backends which support combining adjacent strings
   even across lines.
   In these backends we cannot use the xgettext_comment* functions directly,
   because in multiline string expressions like
           "string1" +
           "string2"
   the newline between "string1" and "string2" would cause a call to
   xgettext_comment_reset(), thus destroying the accumulated comments
   that we need a little later, when we have concatenated the two strings
   and pass them to remember_a_message().
   Instead, we do the bookkeeping of the accumulated comments directly,
   and save a pointer to the accumulated comments when we read "string1".
   In order to avoid excessive copying of strings, we use reference
   counting.  */

typedef struct refcounted_string_list_ty refcounted_string_list_ty;
struct refcounted_string_list_ty
{
  unsigned int refcount;
  struct string_list_ty contents;
};

static inline refcounted_string_list_ty *
add_reference (refcounted_string_list_ty *rslp)
{
  if (rslp != NULL)
    rslp->refcount++;
  return rslp;
}

static inline void
drop_reference (refcounted_string_list_ty *rslp)
{
  if (rslp != NULL)
    {
      if (rslp->refcount > 1)
	rslp->refcount--;
      else
	{
	  string_list_destroy (&rslp->contents);
	  free (rslp);
	}
    }
}

extern refcounted_string_list_ty *savable_comment;
extern void savable_comment_add (const char *str);
extern void savable_comment_reset (void);
extern void savable_comment_to_xgettext_comment (refcounted_string_list_ty *rslp);


/* Add a message to the list of extracted messages.
   string must be malloc()ed string; its ownership is passed to the callee.
   pos->file_name must be allocated with indefinite extent.  */
extern message_ty *remember_a_message (message_list_ty *mlp,
				       char *string,
				       flag_context_ty context,
				       lex_pos_ty *pos);
/* Add an msgid_plural to a message previously returned by
   remember_a_message.
   string must be malloc()ed string; its ownership is passed to the callee.
   pos->file_name must be allocated with indefinite extent.  */
extern void remember_a_message_plural (message_ty *mp,
				       char *string,
				       flag_context_ty context,
				       lex_pos_ty *pos);


#ifdef __cplusplus
}
#endif


#endif /* _XGETTEXT_H */
