/* Parser definitions for GDB.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

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

#include "expression.h"
#include "symtab.h"

struct block;
struct language_defn;
struct internalvar;
class innermost_block_tracker;

extern bool parser_debug;

/* A class that can be used to build a "struct expression".  */

struct expr_builder
{
  /* Constructor.  LANG is the language used to parse the expression.
     And GDBARCH is the gdbarch to use during parsing.  */

  expr_builder (const struct language_defn *lang,
		struct gdbarch *gdbarch);

  DISABLE_COPY_AND_ASSIGN (expr_builder);

  /* Resize the allocated expression to the correct size, and return
     it as an expression_up -- passing ownership to the caller.  */
  ATTRIBUTE_UNUSED_RESULT expression_up release ();

  /* Return the gdbarch that was passed to the constructor.  */

  struct gdbarch *gdbarch ()
  {
    return expout->gdbarch;
  }

  /* Return the language that was passed to the constructor.  */

  const struct language_defn *language ()
  {
    return expout->language_defn;
  }

  /* The size of the expression above.  */

  size_t expout_size;

  /* The expression related to this parser state.  */

  expression_up expout;

  /* The number of elements already in the expression.  This is used
     to know where to put new elements.  */

  size_t expout_ptr;
};

/* This is used for expression completion.  */

struct expr_completion_state
{
  /* The index of the last struct expression directly before a '.' or
     '->'.  This is set when parsing and is only used when completing a
     field name.  It is -1 if no dereference operation was found.  */
  int expout_last_struct = -1;

  /* If we are completing a tagged type name, this will be nonzero.  */
  enum type_code expout_tag_completion_type = TYPE_CODE_UNDEF;

  /* The token for tagged type name completion.  */
  gdb::unique_xmalloc_ptr<char> expout_completion_name;
};

/* An instance of this type is instantiated during expression parsing,
   and passed to the appropriate parser.  It holds both inputs to the
   parser, and result.  */

struct parser_state : public expr_builder
{
  /* Constructor.  LANG is the language used to parse the expression.
     And GDBARCH is the gdbarch to use during parsing.  */

  parser_state (const struct language_defn *lang,
		struct gdbarch *gdbarch,
		const struct block *context_block,
		CORE_ADDR context_pc,
		int comma,
		const char *input,
		int completion,
		innermost_block_tracker *tracker)
    : expr_builder (lang, gdbarch),
      expression_context_block (context_block),
      expression_context_pc (context_pc),
      comma_terminates (comma),
      lexptr (input),
      parse_completion (completion),
      block_tracker (tracker)
  {
  }

  DISABLE_COPY_AND_ASSIGN (parser_state);

  /* Begin counting arguments for a function call,
     saving the data about any containing call.  */

  void start_arglist ()
  {
    m_funcall_chain.push_back (arglist_len);
    arglist_len = 0;
  }

  /* Return the number of arguments in a function call just terminated,
     and restore the data for the containing function call.  */

  int end_arglist ()
  {
    int val = arglist_len;
    arglist_len = m_funcall_chain.back ();
    m_funcall_chain.pop_back ();
    return val;
  }

  /* Mark the current index as the starting location of a structure
     expression.  This is used when completing on field names.  */

  void mark_struct_expression ();

  /* Indicate that the current parser invocation is completing a tag.
     TAG is the type code of the tag, and PTR and LENGTH represent the
     start of the tag name.  */

  void mark_completion_tag (enum type_code tag, const char *ptr, int length);


  /* If this is nonzero, this block is used as the lexical context for
     symbol names.  */

  const struct block * const expression_context_block;

  /* If expression_context_block is non-zero, then this is the PC
     within the block that we want to evaluate expressions at.  When
     debugging C or C++ code, we use this to find the exact line we're
     at, and then look up the macro definitions active at that
     point.  */
  const CORE_ADDR expression_context_pc;

  /* Nonzero means stop parsing on first comma (if not within parentheses).  */

  int comma_terminates;

  /* During parsing of a C expression, the pointer to the next character
     is in this variable.  */

  const char *lexptr;

  /* After a token has been recognized, this variable points to it.
     Currently used only for error reporting.  */
  const char *prev_lexptr = nullptr;

  /* Number of arguments seen so far in innermost function call.  */

  int arglist_len = 0;

  /* True if parsing an expression to attempt completion.  */
  int parse_completion;

  /* Completion state is updated here.  */
  expr_completion_state m_completion_state;

  /* The innermost block tracker.  */
  innermost_block_tracker *block_tracker;

private:

  /* Data structure for saving values of arglist_len for function calls whose
     arguments contain other function calls.  */

  std::vector<int> m_funcall_chain;
};

/* When parsing expressions we track the innermost block that was
   referenced.  */

class innermost_block_tracker
{
public:
  innermost_block_tracker (innermost_block_tracker_types types
			   = INNERMOST_BLOCK_FOR_SYMBOLS)
    : m_types (types),
      m_innermost_block (NULL)
  { /* Nothing.  */ }

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

/* Reverse an expression from suffix form (in which it is constructed)
   to prefix form (in which we can conveniently print or execute it).
   Ordinarily this always returns -1.  However, if LAST_STRUCT
   is not -1 (i.e., we are trying to complete a field name), it will
   return the index of the subexpression which is the left-hand-side
   of the struct operation at LAST_STRUCT.  */

extern int prefixify_expression (struct expression *expr,
				 int last_struct = -1);

extern void write_exp_elt_opcode (struct expr_builder *, enum exp_opcode);

extern void write_exp_elt_sym (struct expr_builder *, struct symbol *);

extern void write_exp_elt_longcst (struct expr_builder *, LONGEST);

extern void write_exp_elt_floatcst (struct expr_builder *, const gdb_byte *);

extern void write_exp_elt_type (struct expr_builder *, struct type *);

extern void write_exp_elt_intern (struct expr_builder *, struct internalvar *);

extern void write_exp_string (struct expr_builder *, struct stoken);

void write_exp_string_vector (struct expr_builder *, int type,
			      struct stoken_vector *vec);

extern void write_exp_bitstring (struct expr_builder *, struct stoken);

extern void write_exp_elt_block (struct expr_builder *, const struct block *);

extern void write_exp_elt_objfile (struct expr_builder *,
				   struct objfile *objfile);

extern void write_exp_msymbol (struct expr_builder *,
			       struct bound_minimal_symbol);

extern void write_dollar_variable (struct parser_state *, struct stoken str);

extern const char *find_template_name_end (const char *);

extern std::string copy_name (struct stoken);

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

extern bool parse_float (const char *p, int len,
			 const struct type *type, gdb_byte *data);

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

#endif /* PARSER_DEFS_H */

