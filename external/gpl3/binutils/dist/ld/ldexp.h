/* ldexp.h -
   Copyright 1991, 1992, 1993, 1994, 1995, 1998, 1999, 2000, 2001, 2002,
   2003, 2004, 2005, 2007, 2011, 2012 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifndef LDEXP_H
#define LDEXP_H

/* The result of an expression tree */
typedef struct {
  bfd_vma value;
  char *str;
  asection *section;
  bfd_boolean valid_p;
} etree_value_type;

enum node_tree_enum {
  etree_binary,
  etree_trinary,
  etree_unary,
  etree_name,
  etree_assign,
  etree_provide,
  etree_provided,
  etree_value,
  etree_assert,
  etree_rel
};

typedef struct {
  int node_code;
  unsigned int lineno;
  const char *filename;
  enum node_tree_enum node_class;
} node_type;

typedef union etree_union {
  node_type type;
  struct {
    node_type type;
    union etree_union *lhs;
    union etree_union *rhs;
  } binary;
  struct {
    node_type type;
    union etree_union *cond;
    union etree_union *lhs;
    union etree_union *rhs;
  } trinary;
  struct {
    node_type type;
    const char *dst;
    union etree_union *src;
    bfd_boolean defsym;
    bfd_boolean hidden;
  } assign;
  struct {
    node_type type;
    union etree_union *child;
  } unary;
  struct {
    node_type type;
    const char *name;
  } name;
  struct {
    node_type type;
    bfd_vma value;
    char *str;
  } value;
  struct {
    node_type type;
    asection *section;
    bfd_vma value;
  } rel;
  struct {
    node_type type;
    union etree_union *child;
    const char *message;
  } assert_s;
} etree_type;

/* Expression evaluation control.  */
typedef enum
{
  /* Parsing linker script.  Will only return "valid" for expressions
     that evaluate to a constant.  */
  lang_first_phase_enum,
  /* Prior to section sizing.  */
  lang_mark_phase_enum,
  /* During section sizing.  */
  lang_allocating_phase_enum,
  /* During assignment of symbol values when relaxation in progress.  */
  lang_assigning_phase_enum,
  /* Final assignment of symbol values.  */
  lang_final_phase_enum
} lang_phase_type;

union lang_statement_union;

enum phase_enum {
  /* We step through the first four states here as we see the
     associated linker script tokens.  */
  exp_dataseg_none,
  exp_dataseg_align_seen,
  exp_dataseg_relro_seen,
  exp_dataseg_end_seen,
  /* The last three states are final, and affect the value returned
     by DATA_SEGMENT_ALIGN.  */
  exp_dataseg_relro_adjust,
  exp_dataseg_adjust,
  exp_dataseg_done
};

enum relro_enum {
  exp_dataseg_relro_none,
  exp_dataseg_relro_start,
  exp_dataseg_relro_end,
};

struct ldexp_control {
  /* Modify expression evaluation depending on this.  */
  lang_phase_type phase;

  /* Principally used for diagnostics.  */
  bfd_boolean assigning_to_dot;
  /* If evaluating an assignment, the destination.  Cleared if an
     etree_name NAME matches this, to signal a self-assignment.
     Note that an etree_name DEFINED does not clear this field, nor
     does the false branch of a trinary expression.  */
  const char *assign_name;

  /* Working results.  */
  etree_value_type result;
  bfd_vma dot;

  /* Current dot and section passed to ldexp folder.  */
  bfd_vma *dotp;
  asection *section;

  /* State machine and results for DATASEG.  */
  struct {
    enum phase_enum phase;

    bfd_vma base, min_base, relro_end, end, pagesize, maxpagesize;

    enum relro_enum relro;

    union lang_statement_union *relro_start_stat;
    union lang_statement_union *relro_end_stat;
  } dataseg;
};

extern struct ldexp_control expld;

/* A maps from a segment name to a base address.  */
typedef struct segment_struct {
  /* The next segment in the linked list.  */
  struct segment_struct *next;
  /* The name of the sgement.  */
  const char *name;
  /* The base address for the segment.  */
  bfd_vma value;
  /* True if a SEGMENT_START directive corresponding to this segment
     has been seen.  */
  bfd_boolean used;
} segment_type;

/* The segments specified by the user on the command-line.  */
extern segment_type *segments;

typedef struct _fill_type fill_type;

etree_type *exp_intop
  (bfd_vma);
etree_type *exp_bigintop
  (bfd_vma, char *);
etree_type *exp_relop
  (asection *, bfd_vma);
void exp_fold_tree
  (etree_type *, asection *, bfd_vma *);
void exp_fold_tree_no_dot
  (etree_type *);
etree_type *exp_binop
  (int, etree_type *, etree_type *);
etree_type *exp_trinop
  (int,etree_type *, etree_type *, etree_type *);
etree_type *exp_unop
  (int, etree_type *);
etree_type *exp_nameop
  (int, const char *);
etree_type *exp_assign
  (const char *, etree_type *, bfd_boolean);
etree_type *exp_defsym
  (const char *, etree_type *);
etree_type *exp_provide
  (const char *, etree_type *, bfd_boolean);
etree_type *exp_assert
  (etree_type *, const char *);
void exp_print_tree
  (etree_type *);
bfd_vma exp_get_vma
  (etree_type *, bfd_vma, char *);
int exp_get_value_int
  (etree_type *, int, char *);
fill_type *exp_get_fill
  (etree_type *, fill_type *, char *);
bfd_vma exp_get_abs_int
  (etree_type *, int, char *);

#endif
