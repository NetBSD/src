/* Definitions for expressions stored in reversed prefix form, for GDB.

   Copyright (C) 1986, 1989, 1992, 1994, 2000, 2003, 2005, 2007, 2008, 2009,
   2010, 2011 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#if !defined (EXPRESSION_H)
#define EXPRESSION_H 1


#include "symtab.h"		/* Needed for "struct block" type.  */
#include "doublest.h"		/* Needed for DOUBLEST.  */


/* Definitions for saved C expressions.  */

/* An expression is represented as a vector of union exp_element's.
   Each exp_element is an opcode, except that some opcodes cause
   the following exp_element to be treated as a long or double constant
   or as a variable.  The opcodes are obeyed, using a stack for temporaries.
   The value is left on the temporary stack at the end.  */

/* When it is necessary to include a string,
   it can occupy as many exp_elements as it needs.
   We find the length of the string using strlen,
   divide to find out how many exp_elements are used up,
   and skip that many.  Strings, like numbers, are indicated
   by the preceding opcode.  */

enum exp_opcode
  {
#define OP(name) name ,

#include "std-operator.def"

    /* First extension operator.  Individual language modules define extra
       operators in *.def include files below with numbers higher than
       OP_EXTENDED0.  */
    OP (OP_EXTENDED0)

/* Language specific operators.  */
#include "ada-operator.def"

#undef OP

    /* Existing only to swallow the last comma (',') from last .inc file.  */
    OP_UNUSED_LAST
  };

union exp_element
  {
    enum exp_opcode opcode;
    struct symbol *symbol;
    LONGEST longconst;
    DOUBLEST doubleconst;
    gdb_byte decfloatconst[16];
    /* Really sizeof (union exp_element) characters (or less for the last
       element of a string).  */
    char string;
    struct type *type;
    struct internalvar *internalvar;
    struct block *block;
    struct objfile *objfile;
  };

struct expression
  {
    const struct language_defn *language_defn;	/* language it was
						   entered in.  */
    struct gdbarch *gdbarch;  /* architecture it was parsed in.  */
    int nelts;
    union exp_element elts[1];
  };

/* Macros for converting between number of expression elements and bytes
   to store that many expression elements.  */

#define EXP_ELEM_TO_BYTES(elements) \
    ((elements) * sizeof (union exp_element))
#define BYTES_TO_EXP_ELEM(bytes) \
    (((bytes) + sizeof (union exp_element) - 1) / sizeof (union exp_element))

/* From parse.c */

extern struct expression *parse_expression (char *);

extern struct type *parse_field_expression (char *, char **);

extern struct expression *parse_exp_1 (char **, struct block *, int);

/* For use by parsers; set if we want to parse an expression and
   attempt to complete a field name.  */
extern int in_parse_field;

/* The innermost context required by the stack and register variables
   we've encountered so far.  To use this, set it to NULL, then call
   parse_<whatever>, then look at it.  */
extern struct block *innermost_block;

/* From eval.c */

/* Values of NOSIDE argument to eval_subexp.  */

enum noside
  {
    EVAL_NORMAL,
    EVAL_SKIP,			/* Only effect is to increment pos.  */
    EVAL_AVOID_SIDE_EFFECTS	/* Don't modify any variables or
				   call any functions.  The value
				   returned will have the correct
				   type, and will have an
				   approximately correct lvalue
				   type (inaccuracy: anything that is
				   listed as being in a register in
				   the function in which it was
				   declared will be lval_register).  */
  };

extern struct value *evaluate_subexp_standard
  (struct type *, struct expression *, int *, enum noside);

/* From expprint.c */

extern void print_expression (struct expression *, struct ui_file *);

extern char *op_string (enum exp_opcode);

extern void dump_raw_expression (struct expression *,
				 struct ui_file *, char *);
extern void dump_prefix_expression (struct expression *, struct ui_file *);

#endif /* !defined (EXPRESSION_H) */
