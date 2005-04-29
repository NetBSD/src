/* Reading PO files.
   Copyright (C) 1995-1998, 2000-2003 Free Software Foundation, Inc.
   This file was written by Bruno Haible <haible@clisp.cons.org>.

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

#ifndef _READ_PO_H
#define _READ_PO_H

#include "message.h"
#include "read-po-abstract.h"

#include <stdbool.h>
#include <stdio.h>


/* For including this file in C++ mode.  */
#ifdef __cplusplus
# define this thiss
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* The following pair of structures cooperate to create a derived class from
   class abstract_po_reader_ty.  (See read-po-abstract.h for an explanation.)
   It implements the default behaviour of reading a PO file and converting it
   to an 'msgdomain_list_ty *'.  */

/* Forward declaration.  */
struct default_po_reader_ty;


typedef struct default_po_reader_class_ty default_po_reader_class_ty;
struct default_po_reader_class_ty
{
  /* Methods inherited from superclass.  */
  struct abstract_po_reader_class_ty super;

  /* How to change the current domain.  */
  void (*set_domain) (struct default_po_reader_ty *pop, char *name);

  /* How to add a message to the list.  */
  void (*add_message) (struct default_po_reader_ty *pop,
		       char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
		       char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
		       bool force_fuzzy, bool obsolete);

  /* How to modify a new message before adding it to the list.  */
  void (*frob_new_message) (struct default_po_reader_ty *pop, message_ty *mp,
			    const lex_pos_ty *msgid_pos,
			    const lex_pos_ty *msgstr_pos);
};


#define DEFAULT_PO_READER_TY \
  ABSTRACT_PO_READER_TY							\
									\
  /* If true, pay attention to comments and filepos comments.  */	\
  bool handle_comments;							\
									\
  /* If true, remember comments for file name and line number for each	\
     msgid, if present in the reference input.  */			\
  bool handle_filepos_comments;						\
									\
  /* If false, domain directives lead to an error messsage.  */		\
  bool allow_domain_directives;						\
									\
  /* If false, duplicate msgids in the same domain and file generate an	\
     error.  If true, such msgids are allowed; the caller should treat	\
     them appropriately.  */						\
  bool allow_duplicates;						\
									\
  /* If true, allow duplicates if they have the same translation.  */	\
  bool allow_duplicates_if_same_msgstr;					\
									\
  /* List of messages already appeared in the current file.  */		\
  msgdomain_list_ty *mdlp;						\
									\
  /* Name of domain we are currently examining.  */			\
  const char *domain;							\
									\
  /* List of messages belonging to the current domain.  */		\
  message_list_ty *mlp;							\
									\
  /* Accumulate comments for next message directive.  */		\
  string_list_ty *comment;						\
  string_list_ty *comment_dot;						\
									\
  /* Accumulate filepos comments for the next message directive.  */	\
  size_t filepos_count;							\
  lex_pos_ty *filepos;							\
									\
  /* Flags transported in special comments.  */				\
  bool is_fuzzy;							\
  enum is_format is_format[NFORMATS];					\
  enum is_wrap do_wrap;							\

typedef struct default_po_reader_ty default_po_reader_ty;
struct default_po_reader_ty
{
  DEFAULT_PO_READER_TY
};

extern void default_constructor (abstract_po_reader_ty *that);
extern void default_destructor (abstract_po_reader_ty *that);
extern void default_parse_brief (abstract_po_reader_ty *that);
extern void default_parse_debrief (abstract_po_reader_ty *that);
extern void default_directive_domain (abstract_po_reader_ty *that, char *name);
extern void default_directive_message (abstract_po_reader_ty *that,
				       char *msgid,
				       lex_pos_ty *msgid_pos,
				       char *msgid_plural,
				       char *msgstr, size_t msgstr_len,
				       lex_pos_ty *msgstr_pos,
				       bool force_fuzzy, bool obsolete);
extern void default_comment (abstract_po_reader_ty *that, const char *s);
extern void default_comment_dot (abstract_po_reader_ty *that, const char *s);
extern void default_comment_filepos (abstract_po_reader_ty *that,
				     const char *name, size_t line);
extern void default_comment_special (abstract_po_reader_ty *that,
				     const char *s);
extern void default_set_domain (default_po_reader_ty *this, char *name);
extern void default_add_message (default_po_reader_ty *this,
				 char *msgid,
				 lex_pos_ty *msgid_pos,
				 char *msgid_plural,
				 char *msgstr, size_t msgstr_len,
				 lex_pos_ty *msgstr_pos,
				 bool force_fuzzy, bool obsolete);

/* Allocate a fresh default_po_reader_ty (or derived class) instance and
   call its constructor.  */
extern default_po_reader_ty *
       default_po_reader_alloc (default_po_reader_class_ty *method_table);


/* If nonzero, remember comments for file name and line number for each
   msgid, if present in the reference input.  Defaults to true.  */
extern DLL_VARIABLE int line_comment;

/* If false, duplicate msgids in the same domain and file generate an error.
   If true, such msgids are allowed; the caller should treat them
   appropriately.  Defaults to false.  */
extern DLL_VARIABLE bool allow_duplicates;

/* Expected syntax of the input files.  */
extern DLL_VARIABLE input_syntax_ty input_syntax;

/* Read the input file from a stream.  Returns a list of messages.  */
extern msgdomain_list_ty *read_po (FILE *fp, const char *real_filename,
				   const char *logical_filename);

/* Read the input file with the name INPUT_NAME.  The ending .po is added
   if necessary.  If INPUT_NAME is not an absolute file name and the file is
   not found, the list of directories in "dir-list.h" is searched.  Returns
   a list of messages.  */
extern msgdomain_list_ty *read_po_file (const char *input_name);


#ifdef __cplusplus
}
#endif


#endif /* _READ_PO_H */
