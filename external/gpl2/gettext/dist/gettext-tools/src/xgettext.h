/* xgettext common functions.
   Copyright (C) 2001-2003, 2005-2006 Free Software Foundation, Inc.
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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _XGETTEXT_H
#define _XGETTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#if HAVE_ICONV
#include <iconv.h>
#endif

#include "message.h"
#include "pos.h"
#include "str-list.h"

/* Declare 'line_comment' and 'input_syntax'.  */
#include "read-catalog.h"


#ifdef __cplusplus
extern "C" {
#endif


/* If true, omit the header entry.
   If false, keep the header entry present in the input.  */
extern int xgettext_omit_header;

extern bool substring_match;


/* Calling convention for a given keyword.  */
struct callshape
{
  int argnum1; /* argument number to use for msgid */
  int argnum2; /* argument number to use for msgid_plural */
  int argnumc; /* argument number to use for msgctxt */
  bool argnum1_glib_context; /* argument argnum1 has the syntax "ctxt|msgid" */
  bool argnum2_glib_context; /* argument argnum2 has the syntax "ctxt|msgid" */
  int argtotal; /* total number of arguments */
  string_list_ty xcomments; /* auto-extracted comments */
};

/* Split keyword spec into keyword, argnum1, argnum2, argnumc.  */
extern void split_keywordspec (const char *spec, const char **endp,
			       struct callshape *shapep);

/* Set of alternative calling conventions for a given keyword.  */
struct callshapes
{
  const char *keyword;          /* the keyword, not NUL terminated */
  size_t keyword_len;           /* the keyword's length */
  size_t nshapes;
  struct callshape shapes[1];   /* actually nshapes elements */
};

/* Insert a (keyword, callshape) pair into a hash table mapping keyword to
   'struct callshapes *'.  */
extern void insert_keyword_callshape (hash_table *table,
				      const char *keyword, size_t keyword_len,
				      const struct callshape *shape);


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


/* Add a message to the list of extracted messages.
   msgctxt must be either NULL or a malloc()ed string; its ownership is passed
   to the callee.
   MSGID must be a malloc()ed string; its ownership is passed to the callee.
   POS->file_name must be allocated with indefinite extent.
   COMMENT may be savable_comment, or it may be a saved copy of savable_comment
   (then add_reference must be used when saving it, and drop_reference while
   dropping it).  Clear savable_comment.  */
extern message_ty *remember_a_message (message_list_ty *mlp,
				       char *msgctxt,
				       char *msgid,
				       flag_context_ty context,
				       lex_pos_ty *pos,
				       refcounted_string_list_ty *comment);
/* Add an msgid_plural to a message previously returned by
   remember_a_message.
   STRING must be a malloc()ed string; its ownership is passed to the callee.
   POS->file_name must be allocated with indefinite extent.
   COMMENT may be savable_comment, or it may be a saved copy of savable_comment
   (then add_reference must be used when saving it, and drop_reference while
   dropping it).  Clear savable_comment.  */
extern void remember_a_message_plural (message_ty *mp,
				       char *string,
				       flag_context_ty context,
				       lex_pos_ty *pos,
				       refcounted_string_list_ty *comment);


/* Represents the progressive parsing of an argument list w.r.t. a single
   'struct callshape'.  */
struct partial_call
{
  int argnumc;                  /* number of context argument, 0 when seen */
  int argnum1;                  /* number of singular argument, 0 when seen */
  int argnum2;                  /* number of plural argument, 0 when seen */
  bool argnum1_glib_context;    /* argument argnum1 has the syntax "ctxt|msgid" */
  bool argnum2_glib_context;    /* argument argnum2 has the syntax "ctxt|msgid" */
  int argtotal;                 /* total number of arguments, 0 if unspecified */
  string_list_ty xcomments;     /* auto-extracted comments */
  char *msgctxt;                /* context - owned string, or NULL */
  lex_pos_ty msgctxt_pos;
  char *msgid;                  /* msgid - owned string, or NULL */
  flag_context_ty msgid_context;
  lex_pos_ty msgid_pos;
  refcounted_string_list_ty *msgid_comment;
  char *msgid_plural;           /* msgid_plural - owned string, or NULL */
  flag_context_ty msgid_plural_context;
  lex_pos_ty msgid_plural_pos;
};

/* Represents the progressive parsing of an argument list w.r.t. an entire
   'struct callshapes'.  */
struct arglist_parser
{
  message_list_ty *mlp;         /* list where the message shall be added */
  const char *keyword;          /* the keyword, not NUL terminated */
  size_t keyword_len;           /* the keyword's length */
  size_t nalternatives;         /* number of partial_call alternatives */
  struct partial_call alternative[1]; /* partial_call alternatives */
};

/* Creates a fresh arglist_parser recognizing calls.
   You can pass shapes = NULL for a parser not recognizing any calls.  */
extern struct arglist_parser * arglist_parser_alloc (message_list_ty *mlp,
						     const struct callshapes *shapes);
/* Clones an arglist_parser.  */
extern struct arglist_parser * arglist_parser_clone (struct arglist_parser *ap);
/* Adds a string argument to an arglist_parser.  ARGNUM must be > 0.
   STRING must be malloc()ed string; its ownership is passed to the callee.
   FILE_NAME must be allocated with indefinite extent.
   COMMENT may be savable_comment, or it may be a saved copy of savable_comment
   (then add_reference must be used when saving it, and drop_reference while
   dropping it).  Clear savable_comment.  */
extern void arglist_parser_remember (struct arglist_parser *ap,
				     int argnum, char *string,
				     flag_context_ty context,
				     char *file_name, size_t line_number,
				     refcounted_string_list_ty *comment);
/* Tests whether an arglist_parser has is not waiting for more arguments after
   argument ARGNUM.  */
extern bool arglist_parser_decidedp (struct arglist_parser *ap, int argnum);
/* Terminates the processing of an arglist_parser after argument ARGNUM and
   deletes it.  */
extern void arglist_parser_done (struct arglist_parser *ap, int argnum);


#ifdef __cplusplus
}
#endif


#endif /* _XGETTEXT_H */
