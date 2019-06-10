/* Parser definitions for GDB.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

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

#include "common/vec.h"
#include "expression.h"

struct block;
struct language_defn;
struct internalvar;

extern int parser_debug;

#define parse_gdbarch(ps) ((ps)->expout->gdbarch)
#define parse_language(ps) ((ps)->expout->language_defn)

struct parser_state
{
  /* Constructor.  INITIAL_SIZE is the initial size of the expout
     array.  LANG is the language used to parse the expression.  And
     GDBARCH is the gdbarch to use during parsing.  */

  parser_state (size_t initial_size, const struct language_defn *lang,
		struct gdbarch *gdbarch);

  DISABLE_COPY_AND_ASSIGN (parser_state);

  /* Resize the allocated expression to the correct size, and return
     it as an expression_up -- passing ownership to the caller.  */
  expression_up release ();

  /* The size of the expression above.  */

  size_t expout_size;

  /* The expression related to this parser state.  */

  expression_up expout;

  /* The number of elements already in the expression.  This is used
     to know where to put new elements.  */

  size_t expout_ptr;
};

/* If this is nonzero, this block is used as the lexical context
   for symbol names.  */

extern const struct block *expression_context_block;

/* If expression_context_block is non-zero, then this is the PC within
   the block that we want to evaluate expressions at.  When debugging
   C or C++ code, we use this to find the exact line we're at, and
   then look up the macro definitions active at that point.  */
extern CORE_ADDR expression_context_pc;

/* While parsing expressions we need to track the innermost lexical block
   that we encounter.  In some situations we need to track the innermost
   block just for symbols, and in other situations we want to track the
   innermost block for symbols and registers.  These flags are used by the
   innermost block tracker to control which blocks we consider for the
   innermost block.  These flags can be combined together as needed.  */

enum innermost_block_tracker_type
{
  /* Track the innermost block for symbols within an expression.  */
  INNERMOST_BLOCK_FOR_SYMBOLS = (1 << 0),

  /* Track the innermost block for registers within an expression.  */
  INNERMOST_BLOCK_FOR_REGISTERS = (1 << 1)
};
DEF_ENUM_FLAGS_TYPE (enum innermost_block_tracker_type,
		     innermost_block_tracker_types);

/* When parsing expressions we track the innermost block that was
   referenced.  */

class innermost_block_tracker
{
public:
  innermost_block_tracker ()
    : m_types (INNERMOST_BLOCK_FOR_SYMBOLS),
      m_innermost_block (NULL)
  { /* Nothing.  */ }

  /* Reset the currently stored innermost block.  Usually called before
     parsing a new expression.  As the most common case is that we only
     want to gather the innermost block for symbols in an expression, this
     becomes the default block tracker type.  */
  void reset (innermost_block_tracker_types t = INNERMOST_BLOCK_FOR_SYMBOLS)
  {
    m_types = t;
    m_innermost_block = NULL;
  }

  /* Update the stored innermost block if the new block B is more inner
     than the currently stored block, or if no block is stored yet.  The
     type T tells us whether the block B was for a symbol or for a
     register.  The stored innermost block is only updated if the type T is
     a type we are interested in, the types we are interested in are held
     in M_TYPES and set during RESET.  */
  void update (const struct block *b, innermost_block_tracker_types t);

  /* Overload of main UPDATE method which extracts the block from BS.  */
  void update (const struct block_symbol &bs)
  {
    update (bs.block, INNERMOST_BLOCK_FOR_SYMBOLS);
  }

  /* Return the stored innermost block.  Can be nullptr if no symbols or
     registers were found during an expression parse, and so no innermost
     block was defined.  */
  const struct block *block () const
  {
    return m_innermost_block;
  }

private:
  /* The type of innermost block being looked for.  */
  innermost_block_tracker_types m_types;

  /* The currently stored innermost block found while parsing an
     expression.  */
  const struct block *m_innermost_block;
};

/* The innermost context required by the stack and register variables
   we've encountered so far.  This should be cleared before parsing an
   expression, and queried once the parse is complete.  */
extern innermost_block_tracker innermost_block;

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
    struct block_symbol sym;
    int is_a_field_of_this;
  };

struct objc_class_str
  {
    struct stoken stoken;
    struct type *type;
    int theclass;
  };

/* For parsing of complicated types.
   An array should be preceded in the list by the size of the array.  */
enum type_pieces
  {
    tp_end = -1, 
    tp_pointer, 
    tp_reference, 
    tp_rvalue_reference,
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
    std::vector<struct type *> *typelist_val;
  };

/* The type stack is an instance of this structure.  */

struct type_stack
{
  /* Elements on the stack.  */
  std::vector<union type_stack_elt> elements;
};

/* Reverse an expression from suffix form (in which it is constructed)
   to prefix form (in which we can conveniently print or execute it).
   Ordinarily this always returns -1.  However, if EXPOUT_LAST_STRUCT
   is not -1 (i.e., we are trying to complete a field name), it will
   return the index of the subexpression which is the left-hand-side
   of the struct operation at EXPOUT_LAST_STRUCT.  */

extern int prefixify_expression (struct expression *expr);

extern void write_exp_elt_opcode (struct parser_state *, enum exp_opcode);

extern void write_exp_elt_sym (struct parser_state *, struct symbol *);

extern void write_exp_elt_longcst (struct parser_state *, LONGEST);

extern void write_exp_elt_floatcst (struct parser_state *, const gdb_byte *);

extern void write_exp_elt_type (struct parser_state *, struct type *);

extern void write_exp_elt_intern (struct parser_state *, struct internalvar *);

extern void write_exp_string (struct parser_state *, struct stoken);

void write_exp_string_vector (struct parser_state *, int type,
			      struct stoken_vector *vec);

extern void write_exp_bitstring (struct parser_state *, struct stoken);

extern void write_exp_elt_block (struct parser_state *, const struct block *);

extern void write_exp_elt_objfile (struct parser_state *,
				   struct objfile *objfile);

extern void write_exp_msymbol (struct parser_state *,
			       struct bound_minimal_symbol);

extern void write_dollar_variable (struct parser_state *, struct stoken str);

extern void mark_struct_expression (struct parser_state *);

extern const char *find_template_name_end (const char *);

extern void start_arglist (void);

extern int end_arglist (void);

extern char *copy_name (struct stoken);

extern void insert_type (enum type_pieces);

extern void push_type (enum type_pieces);

extern void push_type_int (int);

extern void insert_type_address_space (struct parser_state *, char *);

extern enum type_pieces pop_type (void);

extern int pop_type_int (void);

extern struct type_stack *get_type_stack (void);

extern struct type_stack *append_type_stack (struct type_stack *to,
					     struct type_stack *from);

extern void push_type_stack (struct type_stack *stack);

extern void push_typelist (std::vector<struct type *> *typelist);

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

extern const char *op_name_standard (enum exp_opcode);

extern struct type *follow_types (struct type *);

extern type_instance_flags follow_type_instance_flags ();

extern void null_post_parser (expression_up *, int);

extern bool parse_float (const char *p, int len,
			 const struct type *type, gdb_byte *data);

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
    const char *string;
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

    /* Call OBJFILE_FUNC for any objfile found being referenced by the
       single operator of EXP at position POS.  Operator parameters are
       located at positive (POS + number) offsets in EXP.  OBJFILE_FUNC
       should never be called with NULL OBJFILE.  OBJFILE_FUNC should
       get passed an arbitrary caller supplied DATA pointer.  If it
       returns non-zero value then (any other) non-zero value should be
       immediately returned to the caller.  Otherwise zero should be
       returned.  */
    int (*operator_check) (struct expression *exp, int pos,
			   int (*objfile_func) (struct objfile *objfile,
						void *data),
			   void *data);

    /* Name of this operator for dumping purposes.
       The returned value should never be NULL, even if EXP_OPCODE is
       an unknown opcode (a string containing an image of the numeric
       value of the opcode can be returned, for instance).  */
    const char *(*op_name) (enum exp_opcode);

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

/* Reallocate the `expout' pointer inside PS so that it can accommodate
   at least LENELT expression elements.  This function does nothing if
   there is enough room for the elements.  */

extern void increase_expout_size (struct parser_state *ps, size_t lenelt);

#endif /* PARSER_DEFS_H */

