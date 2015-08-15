/* Parser definitions for GDB.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

   Modified from expread.y by the Department of Computer Science at the
   State University of New York at Buffalo.

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

#if !defined (PARSER_DEFS_H)
#define PARSER_DEFS_H 1

#include "doublest.h"
#include "vec.h"
#include "expression.h"

struct block;

extern int parser_debug;

extern struct expression *expout;
extern int expout_size;
extern int expout_ptr;

#define parse_gdbarch (expout->gdbarch)
#define parse_language (expout->language_defn)

/* If this is nonzero, this block is used as the lexical context
   for symbol names.  */

extern const struct block *expression_context_block;

/* If expression_context_block is non-zero, then this is the PC within
   the block that we want to evaluate expressions at.  When debugging
   C or C++ code, we use this to find the exact line we're at, and
   then look up the macro definitions active at that point.  */
extern CORE_ADDR expression_context_pc;

/* The innermost context required by the stack and register variables
   we've encountered so far.  */
extern const struct block *innermost_block;

/* The block in which the most recently discovered symbol was found.
   FIXME: Should be declared along with lookup_symbol in symtab.h; is not
   related specifically to parsing.  */
extern const struct block *block_found;

/* Number of arguments seen so far in innermost function call.  */
extern int arglist_len;

/* A string token, either a char-string or bit-string.  Char-strings are
   used, for example, for the names of symbols.  */

struct stoken
  {
    /* Pointer to first byte of char-string or first bit of bit-string.  */
    const char *ptr;
    /* Length of string in bytes for char-string or bits for bit-string.  */
    int length;
  };

struct typed_stoken
  {
    /* A language-specific type field.  */
    int type;
    /* Pointer to first byte of char-string or first bit of bit-string.  */
    char *ptr;
    /* Length of string in bytes for char-string or bits for bit-string.  */
    int length;
  };

struct stoken_vector
  {
    int len;
    struct typed_stoken *tokens;
  };

struct ttype
  {
    struct stoken stoken;
    struct type *type;
  };

struct symtoken
  {
    struct stoken stoken;
    struct symbol *sym;
    int is_a_field_of_this;
  };

struct objc_class_str
  {
    struct stoken stoken;
    struct type *type;
    int class;
  };

typedef struct type *type_ptr;
DEF_VEC_P (type_ptr);

/* For parsing of complicated types.
   An array should be preceded in the list by the size of the array.  */
enum type_pieces
  {
    tp_end = -1, 
    tp_pointer, 
    tp_reference, 
    tp_array, 
    tp_function,
    tp_function_with_arguments,
    tp_const, 
    tp_volatile, 
    tp_space_identifier,
    tp_type_stack
  };
/* The stack can contain either an enum type_pieces or an int.  */
union type_stack_elt
  {
    enum type_pieces piece;
    int int_val;
    struct type_stack *stack_val;
    VEC (type_ptr) *typelist_val;
  };

/* The type stack is an instance of this structure.  */

struct type_stack
{
  /* Elements on the stack.  */
  union type_stack_elt *elements;
  /* Current stack depth.  */
  int depth;
  /* Allocated size of stack.  */
  int size;
};

/* Helper function to initialize the expout, expout_size, expout_ptr
   trio before it is used to store expression elements created during
   the parsing of an expression.  INITIAL_SIZE is the initial size of
   the expout array.  LANG is the language used to parse the expression.
   And GDBARCH is the gdbarch to use during parsing.  */

extern void initialize_expout (int, const struct language_defn *,
			       struct gdbarch *);

/* Helper function that frees any unsed space in the expout array.
   It is generally used when the parser has just been parsed and
   created.  */

extern void reallocate_expout (void);

/* Reverse an expression from suffix form (in which it is constructed)
   to prefix form (in which we can conveniently print or execute it).
   Ordinarily this always returns -1.  However, if EXPOUT_LAST_STRUCT
   is not -1 (i.e., we are trying to complete a field name), it will
   return the index of the subexpression which is the left-hand-side
   of the struct operation at EXPOUT_LAST_STRUCT.  */

extern int prefixify_expression (struct expression *expr);

extern void write_exp_elt_opcode (enum exp_opcode);

extern void write_exp_elt_sym (struct symbol *);

extern void write_exp_elt_longcst (LONGEST);

extern void write_exp_elt_dblcst (DOUBLEST);

extern void write_exp_elt_decfloatcst (gdb_byte *);

extern void write_exp_elt_type (struct type *);

extern void write_exp_elt_intern (struct internalvar *);

extern void write_exp_string (struct stoken);

void write_exp_string_vector (int type, struct stoken_vector *vec);

extern void write_exp_bitstring (struct stoken);

extern void write_exp_elt_block (const struct block *);

extern void write_exp_elt_objfile (struct objfile *objfile);

extern void write_exp_msymbol (struct bound_minimal_symbol);

extern void write_dollar_variable (struct stoken str);

extern void mark_struct_expression (void);

extern const char *find_template_name_end (const char *);

extern void start_arglist (void);

extern int end_arglist (void);

extern char *copy_name (struct stoken);

extern void insert_type (enum type_pieces);

extern void push_type (enum type_pieces);

extern void push_type_int (int);

extern void insert_type_address_space (char *);

extern enum type_pieces pop_type (void);

extern int pop_type_int (void);

extern struct type_stack *get_type_stack (void);

extern struct type_stack *append_type_stack (struct type_stack *to,
					     struct type_stack *from);

extern void push_type_stack (struct type_stack *stack);

extern void type_stack_cleanup (void *arg);

extern void push_typelist (VEC (type_ptr) *typelist);

extern int length_of_subexp (struct expression *, int);

extern int dump_subexp (struct expression *, struct ui_file *, int);

extern int dump_subexp_body_standard (struct expression *, 
				      struct ui_file *, int);

extern void operator_length (const struct expression *, int, int *, int *);

extern void operator_length_standard (const struct expression *, int, int *,
				      int *);

extern int operator_check_standard (struct expression *exp, int pos,
				    int (*objfile_func)
				      (struct objfile *objfile, void *data),
				    void *data);

extern char *op_name_standard (enum exp_opcode);

extern struct type *follow_types (struct type *);

extern void null_post_parser (struct expression **, int);

extern int parse_float (const char *p, int len, DOUBLEST *d,
			const char **suffix);

extern int parse_c_float (struct gdbarch *gdbarch, const char *p, int len,
			  DOUBLEST *d, struct type **t);

/* During parsing of a C expression, the pointer to the next character
   is in this variable.  */

extern const char *lexptr;

/* After a token has been recognized, this variable points to it.
   Currently used only for error reporting.  */
extern const char *prev_lexptr;

/* Current depth in parentheses within the expression.  */

extern int paren_depth;

/* Nonzero means stop parsing on first comma (if not within parentheses).  */

extern int comma_terminates;

/* These codes indicate operator precedences for expression printing,
   least tightly binding first.  */
/* Adding 1 to a precedence value is done for binary operators,
   on the operand which is more tightly bound, so that operators
   of equal precedence within that operand will get parentheses.  */
/* PREC_HYPER and PREC_ABOVE_COMMA are not the precedence of any operator;
   they are used as the "surrounding precedence" to force
   various kinds of things to be parenthesized.  */
enum precedence
  {
    PREC_NULL, PREC_COMMA, PREC_ABOVE_COMMA, PREC_ASSIGN, PREC_LOGICAL_OR,
    PREC_LOGICAL_AND, PREC_BITWISE_IOR, PREC_BITWISE_AND, PREC_BITWISE_XOR,
    PREC_EQUAL, PREC_ORDER, PREC_SHIFT, PREC_ADD, PREC_MUL, PREC_REPEAT,
    PREC_HYPER, PREC_PREFIX, PREC_SUFFIX, PREC_BUILTIN_FUNCTION
  };

/* Table mapping opcodes into strings for printing operators
   and precedences of the operators.  */

struct op_print
  {
    char *string;
    enum exp_opcode opcode;
    /* Precedence of operator.  These values are used only by comparisons.  */
    enum precedence precedence;

    /* For a binary operator:  1 iff right associate.
       For a unary operator:  1 iff postfix.  */
    int right_assoc;
  };

/* Information needed to print, prefixify, and evaluate expressions for 
   a given language.  */

struct exp_descriptor
  {
    /* Print subexpression.  */
    void (*print_subexp) (struct expression *, int *, struct ui_file *,
			  enum precedence);

    /* Returns number of exp_elements needed to represent an operator and
       the number of subexpressions it takes.  */
    void (*operator_length) (const struct expression*, int, int*, int *);

    /* Call TYPE_FUNC and OBJFILE_FUNC for any TYPE and OBJFILE found being
       referenced by the single operator of EXP at position POS.  Operator
       parameters are located at positive (POS + number) offsets in EXP.
       The functions should never be called with NULL TYPE or NULL OBJFILE.
       Functions should get passed an arbitrary caller supplied DATA pointer.
       If any of the functions returns non-zero value then (any other) non-zero
       value should be immediately returned to the caller.  Otherwise zero
       should be returned.  */
    int (*operator_check) (struct expression *exp, int pos,
			   int (*objfile_func) (struct objfile *objfile,
						void *data),
			   void *data);

    /* Name of this operator for dumping purposes.
       The returned value should never be NULL, even if EXP_OPCODE is
       an unknown opcode (a string containing an image of the numeric
       value of the opcode can be returned, for instance).  */
    char *(*op_name) (enum exp_opcode);

    /* Dump the rest of this (prefix) expression after the operator
       itself has been printed.  See dump_subexp_body_standard in
       (expprint.c).  */
    int (*dump_subexp_body) (struct expression *, struct ui_file *, int);

    /* Evaluate an expression.  */
    struct value *(*evaluate_exp) (struct type *, struct expression *,
				   int *, enum noside);
  };


/* Default descriptor containing standard definitions of all
   elements.  */
extern const struct exp_descriptor exp_descriptor_standard;

/* Functions used by language-specific extended operators to (recursively)
   print/dump subexpressions.  */

extern void print_subexp (struct expression *, int *, struct ui_file *,
			  enum precedence);

extern void print_subexp_standard (struct expression *, int *, 
				   struct ui_file *, enum precedence);

/* Function used to avoid direct calls to fprintf
   in the code generated by the bison parser.  */

extern void parser_fprintf (FILE *, const char *, ...) ATTRIBUTE_PRINTF (2, 3);

extern int exp_uses_objfile (struct expression *exp, struct objfile *objfile);

extern void mark_completion_tag (enum type_code, const char *ptr,
				 int length);

#endif /* PARSER_DEFS_H */

