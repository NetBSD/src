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

#ifndef SRC_PO_H
#define SRC_PO_H

#include "po-lex.h"

/* Note: the _t suffix is reserved by ANSI C, so the _ty suffix is
   used to indicate a type name  */

/* The following pair of structures cooperate to create an "Object" in
   the OO sense, we are simply doing it manually, rather than with the
   help of an OO compiler.  This implementation allows polymorphism
   and inheritance - more than enough for the immediate needs.

   This first structure contains pointers to functions.  Each function
   is a method for the class (base or derived).

   Use a NULL pointer where no action is required.  */

/* Forward decaration.  */
struct po_ty;


typedef struct po_method_ty po_method_ty;
struct po_method_ty
{
  /* how many bytes to malloc for this class */
  size_t size;

  /* what to do immediately after the instance is malloc()ed */
  void (*constructor) PARAMS ((struct po_ty *__pop));

  /* what to do immediately before the instance is free()ed */
  void (*destructor) PARAMS ((struct po_ty *__pop));

  /* what to do with a domain directive */
  void (*directive_domain) PARAMS ((struct po_ty *__pop, char *__name));

  /* what to do with a message directive */
  void (*directive_message) PARAMS ((struct po_ty *__pop, char *__msgid,
				     lex_pos_ty *__msgid_pos, char *__msgstr,
				     lex_pos_ty *__msgstr_pos));

  /* This method is invoked before the parse, but after the file is
     opened by the lexer.  */
  void (*parse_brief) PARAMS ((struct po_ty *__pop));

  /* This method is invoked after the parse, but before the file is
     closed by the lexer.  The intention is to make consistency checks
     against the file here, and emit the errors through the lex_error*
     functions.  */
  void (*parse_debrief) PARAMS ((struct po_ty *__pop));

  /* What to do with a plain-vanilla comment - the expectation is that
     they will be accumulated, and added to the next message
     definition seen.  Or completely ignored.  */
  void (*comment) PARAMS ((struct po_ty *__pop, const char *__s));

  /* What to do with a comment that starts with a dot (i.e.  extracted
     by xgettext) - the expectation is that they will be accumulated,
     and added to the next message definition seen.  Or completely
     ignored.  */
  void (*comment_dot) PARAMS ((struct po_ty *__pop, const char *__s));

  /* What to do with a file position seen in a comment (i.e. a message
     location comment extracted by xgettext) - the expectation is that
     they will be accumulated, and added to the next message
     definition seen.  Or completely ignored.  */
  void (*comment_filepos) PARAMS ((struct po_ty *__pop, const char *__s,
				   int __line));

  /* What to do with a comment that starts with a `!' - this is a
     special comment.  One of the possible uses is to indicate a
     inexact translation.  */
  void (*comment_special) PARAMS ((struct po_ty *__pop, const char *__s));
};


/* This next structure defines the base class passed to the methods.
   Derived methods will often need to cast their first argument before
   using it (this correponds to the implicit ``this'' argument of many
   C++ implementations).

   When declaring derived classes, use the PO_BASE_TY define at the
   start of the structure, to declare inherited instance variables,
   etc.  */

#define PO_BASE_TY \
  po_method_ty *method;

typedef struct po_ty po_ty;
struct po_ty
{
  PO_BASE_TY
};


po_ty *po_alloc PARAMS ((po_method_ty *__jtable));
void po_scan PARAMS ((po_ty *__pop, const char *__filename));
void po_free PARAMS ((po_ty *__pop));

/* Callbacks used by po-gram.y or po-hash.y or po-lex.c, indirectly
   from po_scan.  */
void po_callback_domain PARAMS ((char *__name));
void po_callback_message PARAMS ((char *__msgid, lex_pos_ty *__msgid_pos,
				  char *__msgstr, lex_pos_ty *__msgstr_pos));
void po_callback_comment PARAMS ((const char *__s));
void po_callback_comment_dot PARAMS ((const char *__s));
void po_callback_comment_filepos PARAMS ((const char *__s, int __line));

#endif
