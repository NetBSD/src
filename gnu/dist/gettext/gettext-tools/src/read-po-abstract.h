/* Reading PO files, abstract class.
   Copyright (C) 1995-1996, 1998, 2000-2003, 2005 Free Software Foundation, Inc.

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

#ifndef _READ_PO_ABSTRACT_H
#define _READ_PO_ABSTRACT_H

#include "po-lex.h"
#include "message.h"

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Note: the _t suffix is reserved by ANSI C, so the _ty suffix is
   used to indicate a type name.  */

/* The following pair of structures cooperate to create an "Object" in
   the OO sense.  We are simply doing it manually, rather than with the
   help of an OO compiler.  This implementation allows polymorphism
   and inheritance - more than enough for the immediate needs.  */

/* Forward declaration.  */
struct abstract_po_reader_ty;


/* This first structure, playing the role of the "Class" in OO sense,
   contains pointers to functions.  Each function is a method for the
   class (base or derived).  Use a NULL pointer where no action is
   required.  */

typedef struct abstract_po_reader_class_ty abstract_po_reader_class_ty;
struct abstract_po_reader_class_ty
{
  /* how many bytes to malloc for an instance of this class */
  size_t size;

  /* what to do immediately after the instance is malloc()ed */
  void (*constructor) (struct abstract_po_reader_ty *pop);

  /* what to do immediately before the instance is free()ed */
  void (*destructor) (struct abstract_po_reader_ty *pop);

  /* This method is invoked before the parse, but after the file is
     opened by the lexer.  */
  void (*parse_brief) (struct abstract_po_reader_ty *pop);

  /* This method is invoked after the parse, but before the file is
     closed by the lexer.  The intention is to make consistency checks
     against the file here, and emit the errors through the lex_error*
     functions.  */
  void (*parse_debrief) (struct abstract_po_reader_ty *pop);

  /* what to do with a domain directive */
  void (*directive_domain) (struct abstract_po_reader_ty *pop, char *name);

  /* what to do with a message directive */
  void (*directive_message) (struct abstract_po_reader_ty *pop,
			     char *msgid, lex_pos_ty *msgid_pos,
			     char *msgid_plural,
			     char *msgstr, size_t msgstr_len,
			     lex_pos_ty *msgstr_pos,
			     bool force_fuzzy, bool obsolete);

  /* What to do with a plain-vanilla comment - the expectation is that
     they will be accumulated, and added to the next message
     definition seen.  Or completely ignored.  */
  void (*comment) (struct abstract_po_reader_ty *pop, const char *s);

  /* What to do with a comment that starts with a dot (i.e.  extracted
     by xgettext) - the expectation is that they will be accumulated,
     and added to the next message definition seen.  Or completely
     ignored.  */
  void (*comment_dot) (struct abstract_po_reader_ty *pop, const char *s);

  /* What to do with a file position seen in a comment (i.e. a message
     location comment extracted by xgettext) - the expectation is that
     they will be accumulated, and added to the next message
     definition seen.  Or completely ignored.  */
  void (*comment_filepos) (struct abstract_po_reader_ty *pop,
			   const char *s, size_t line);

  /* What to do with a comment that starts with a ',' or '!' - this is a
     special comment.  One of the possible uses is to indicate a
     inexact translation.  */
  void (*comment_special) (struct abstract_po_reader_ty *pop, const char *s);
};


/* This next structure defines the base class passed to the methods.
   Derived methods will often need to cast their first argument before
   using it (this corresponds to the implicit ``this'' argument in C++).

   When declaring derived classes, use the ABSTRACT_PO_READER_TY define
   at the start of the structure, to declare inherited instance variables,
   etc.  */

#define ABSTRACT_PO_READER_TY \
  abstract_po_reader_class_ty *methods;

typedef struct abstract_po_reader_ty abstract_po_reader_ty;
struct abstract_po_reader_ty
{
  ABSTRACT_PO_READER_TY
};


/* Allocate a fresh abstract_po_reader_ty (or derived class) instance and
   call its constructor.  */
extern abstract_po_reader_ty *
       po_reader_alloc (abstract_po_reader_class_ty *method_table);

/* Kinds of PO file input syntaxes.  */
enum input_syntax_ty
{
  syntax_po,
  syntax_properties,
  syntax_stringtable
};
typedef enum input_syntax_ty input_syntax_ty;

/* Read a PO file from a stream, and dispatch to the various
   abstract_po_reader_class_ty methods.  */
extern void
       po_scan (abstract_po_reader_ty *pop, FILE *fp,
		const char *real_filename, const char *logical_filename,
		input_syntax_ty syntax);

/* Call the destructor and deallocate a abstract_po_reader_ty (or derived
   class) instance.  */
extern void
       po_reader_free (abstract_po_reader_ty *pop);


/* Callbacks used by po-gram.y or po-lex.c, indirectly from po_scan.  */
extern void po_callback_domain (char *name);
extern void po_callback_message (char *msgid, lex_pos_ty *msgid_pos,
				 char *msgid_plural,
				 char *msgstr, size_t msgstr_len,
				 lex_pos_ty *msgstr_pos,
				 bool force_fuzzy, bool obsolete);
extern void po_callback_comment (const char *s);
extern void po_callback_comment_dot (const char *s);
extern void po_callback_comment_filepos (const char *s, size_t line);
extern void po_callback_comment_special (const char *s);
extern void po_callback_comment_dispatcher (const char *s);

/* Parse a special comment and put the result in *fuzzyp, formatp, *wrapp.  */
extern void po_parse_comment_special (const char *s, bool *fuzzyp,
				      enum is_format formatp[NFORMATS],
				      enum is_wrap *wrapp);


#ifdef __cplusplus
}
#endif


#endif /* _READ_PO_ABSTRACT_H */
