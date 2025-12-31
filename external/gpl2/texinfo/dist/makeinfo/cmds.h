/*	$NetBSD: cmds.h,v 1.4 2025/12/31 22:18:50 oster Exp $	*/

/* cmds.h -- declarations for cmds.c.
   Id: cmds.h,v 1.9 2004/11/26 00:48:35 karl Exp 

   Copyright (C) 1998, 1999, 2002, 2003, 2004 Free Software Foundation,
   Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef CMDS_H
#define CMDS_H

/* The three arguments a command can get are a flag saying whether it is
   before argument parsing (START) or after (END), the starting position
   of the arguments, and the ending position.  */
typedef void COMMAND_FUNCTION (int, int, int);

/* Each command has an associated function.  When the command is
   encountered in the text, the associated function is called with START
   as the argument.  If the function expects arguments in braces, it
   remembers itself on the stack.  When the corresponding close brace is
   encountered, the function is called with END as the argument. */
#define START 0
#define END 1

/* Does the command expect braces?  */
#define NO_BRACE_ARGS 0
#define BRACE_ARGS 1
#define MAYBE_BRACE_ARGS 2

typedef struct
{
  char *name;
  COMMAND_FUNCTION *proc;
  int argument_in_braces;
} COMMAND;

extern COMMAND command_table[];

typedef struct acronym_desc
{
  struct acronym_desc *next;
  char *acronym;
  char *description;
} ACRONYM_DESC;

/* Texinfo commands.  */
extern void insert_self (int arg, int arg2, int arg3),
  insert_space (int arg, int arg2, int arg3),
  cm_ignore_line (int arg, int arg2, int arg3),
  cm_ignore_arg (int arg, int start_pos, int end_pos),
  cm_comment (int arg, int arg2, int arg3),
  cm_no_op (int arg, int arg2, int arg3);

/* Document structure and meta information.  */
extern void cm_setfilename (int arg, int arg2, int arg3),
  cm_settitle (int arg, int arg2, int arg3),
  cm_documentdescription (int arg, int arg2, int arg3),
  cm_node (int arg, int arg2, int arg3),
  cm_menu (int arg, int arg2, int arg3),
  cm_detailmenu (int arg, int arg2, int arg3),
  cm_dircategory (int arg, int arg2, int arg3),
  cm_direntry (int arg, int arg2, int arg3),
  cm_bye (int arg, int arg2, int arg3);

/* File inclusion.  */
extern void cm_include (int arg, int arg2, int arg3),
  cm_verbatiminclude (int arg, int arg2, int arg3);

/* Cross referencing commands.  */
extern void cm_anchor (int arg, int arg2, int arg3),
  cm_xref (int arg, int arg2, int arg3),
  cm_pxref (int arg, int arg2, int arg3),
  cm_ref (int arg, int arg2, int arg3),
  cm_inforef (int arg, int arg2, int arg3),
  cm_uref (int arg, int arg2, int arg3);

/* Special insertions.  */
extern void cm_LaTeX (int arg, int arg2, int arg3),
  cm_TeX (int arg, int arg2, int arg3),
  cm_bullet (int arg, int arg2, int arg3),
  cm_colon (int arg, int arg2, int arg3),
  cm_comma (int arg, int arg2, int arg3),
  cm_copyright (int arg, int arg2, int arg3),
  cm_dots (int arg, int arg2, int arg3),
  cm_enddots (int arg, int arg2, int arg3),
  cm_equiv (int arg, int arg2, int arg3),
  cm_error (int arg, int arg2, int arg3),
  cm_expansion (int arg, int arg2, int arg3),
  cm_image (int arg, int arg2, int arg3),
  cm_insert_copying (int arg, int arg2, int arg3),
  cm_minus (int arg, int arg2, int arg3),
  cm_point (int arg, int arg2, int arg3),
  cm_print (int arg, int arg2, int arg3),
  cm_punct (int arg, int arg2, int arg3),
  cm_registeredsymbol (int arg, int arg2, int arg3),
  cm_result (int arg, int arg2, int arg3);

/* Emphasis and markup.  */
extern void cm_acronym (int arg, int arg2, int arg3),
  cm_abbr (int arg, int arg2, int arg3),
  cm_b (int arg, int arg2, int arg3),
  cm_cite (int arg, int position, int arg3),
  cm_code (int arg, int arg2, int arg3),
  cm_dfn (int arg, int position, int arg3),
  cm_dmn (int arg, int arg2, int arg3),
  cm_email (int arg, int arg2, int arg3),
  cm_emph (int arg, int arg2, int arg3),
  cm_i (int arg, int arg2, int arg3),
  cm_kbd (int arg, int arg2, int arg3),
  cm_key (int arg, int arg2, int arg3),
  cm_math (int arg, int arg2, int arg3),
  cm_not_fixed_width (int arg, int start, int end),
  cm_r (int arg, int arg2, int arg3),
  cm_sansserif (int arg, int arg2, int arg3),
  cm_sc (int arg, int start_pos, int end_pos),
  cm_slanted (int arg, int arg2, int arg3),
  cm_strong (int arg, int start_pos, int end_pos),
  cm_tt (int arg, int arg2, int arg3),
  cm_indicate_url (int arg, int start, int end),
  cm_var (int arg, int start_pos, int end_pos),
  cm_verb (int arg, int arg2, int arg3);

/* Block environments.  */
extern void cm_cartouche (int arg, int arg2, int arg3),
  cm_group (int arg, int arg2, int arg3),
  cm_display (int arg, int arg2, int arg3),
  cm_smalldisplay (int arg, int arg2, int arg3),
  cm_example (int arg, int arg2, int arg3),
  cm_smallexample (int arg, int arg2, int arg3),
  cm_smalllisp (int arg, int arg2, int arg3),
  cm_lisp (int arg, int arg2, int arg3),
  cm_format (int arg, int arg2, int arg3),
  cm_smallformat (int arg, int arg2, int arg3),
  cm_quotation (int arg, int arg2, int arg3),
  cm_copying (int arg, int arg2, int arg3),
  cm_flushleft (int arg, int arg2, int arg3),
  cm_flushright (int arg, int arg2, int arg3),
  cm_verbatim (int arg, int arg2, int arg3),
  cm_end (int arg, int arg2, int arg3);

/* Tables, lists, enumerations.  */
extern void cm_table (int arg, int arg2, int arg3),
  cm_ftable (int arg, int arg2, int arg3),
  cm_vtable (int arg, int arg2, int arg3),
  cm_itemize (int arg, int arg2, int arg3),
  cm_enumerate (int arg, int arg2, int arg3),
  cm_multitable (int arg, int arg2, int arg3),
  cm_headitem (int arg, int arg2, int arg3),
  cm_item (int arg, int arg2, int arg3),
  cm_itemx (int arg, int arg2, int arg3),
  cm_tab (int arg, int arg2, int arg3);

extern void cm_center (int arg, int arg2, int arg3),
  cm_exdent (int arg, int arg2, int arg3),
  cm_indent (int arg, int arg2, int arg3),
  cm_noindent (int arg, int arg2, int arg3),
  cm_noindent_cmd (int arg, int arg2, int arg3);

/* Line and page breaks.  */
extern void cm_asterisk (int arg, int arg2, int arg3),
  cm_sp (int arg, int arg2, int arg3),
  cm_page (int arg, int arg2, int arg3);

/* Non breaking words.  */
extern void cm_tie (int arg, int arg2, int arg3),
  cm_w (int arg, int arg2, int arg3);

/* Title page creation.  */
extern void cm_titlepage (int arg, int arg2, int arg3),
  cm_author (int arg, int arg2, int arg3),
  cm_titlepage_cmds (int arg, int arg2, int arg3),
  cm_titlefont (int arg, int arg2, int arg3),
  cm_today (int arg, int arg2, int arg3);

/* Floats.  */
extern void cm_float (int arg, int arg2, int arg3),
  cm_caption (int arg, int arg2, int arg3),
  cm_shortcaption (int arg, int arg2, int arg3),
  cm_listoffloats (int arg, int arg2, int arg3);

/* Indices.  */
extern void cm_kindex (int arg, int arg2, int arg3),
  cm_cindex (int arg, int arg2, int arg3),
  cm_findex (int arg, int arg2, int arg3),
  cm_pindex (int arg, int arg2, int arg3),
  cm_vindex (int arg, int arg2, int arg3),
  cm_tindex (int arg, int arg2, int arg3),
  cm_defindex (int arg, int arg2, int arg3),
  cm_defcodeindex (int arg, int arg2, int arg3),
  cm_synindex (int arg, int arg2, int arg3),
  cm_printindex (int arg, int arg2, int arg3);

/* Conditionals. */
extern void cm_set (int arg, int arg2, int arg3),
  cm_clear (int arg, int arg2, int arg3),
  cm_ifset (int arg, int arg2, int arg3),
  cm_ifclear (int arg, int arg2, int arg3),
  cm_ifeq (int arg, int arg2, int arg3),
  cm_value (int arg, int start_pos, int end_pos);

#endif /* !CMDS_H */
