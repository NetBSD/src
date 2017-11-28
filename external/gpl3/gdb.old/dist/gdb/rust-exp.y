/* Bison parser for Rust expressions, for GDB.
   Copyright (C) 2016 Free Software Foundation, Inc.

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

/* Removing the last conflict seems difficult.  */
%expect 1

%{

#include "defs.h"

#include "block.h"
#include "charset.h"
#include "cp-support.h"
#include "gdb_obstack.h"
#include "gdb_regex.h"
#include "rust-lang.h"
#include "parser-defs.h"
#include "selftest.h"
#include "value.h"
#include "vec.h"

#define GDB_YY_REMAP_PREFIX rust
#include "yy-remap.h"

#define RUSTSTYPE YYSTYPE

extern initialize_file_ftype _initialize_rust_exp;

struct rust_op;
typedef const struct rust_op *rust_op_ptr;
DEF_VEC_P (rust_op_ptr);

/* A typed integer constant.  */

struct typed_val_int
{
  LONGEST val;
  struct type *type;
};

/* A typed floating point constant.  */

struct typed_val_float
{
  DOUBLEST dval;
  struct type *type;
};

/* An identifier and an expression.  This is used to represent one
   element of a struct initializer.  */

struct set_field
{
  struct stoken name;
  const struct rust_op *init;
};

typedef struct set_field set_field;

DEF_VEC_O (set_field);


static int rustyylex (void);
static void rust_push_back (char c);
static const char *rust_copy_name (const char *, int);
static struct stoken rust_concat3 (const char *, const char *, const char *);
static struct stoken make_stoken (const char *);
static struct block_symbol rust_lookup_symbol (const char *name,
					       const struct block *block,
					       const domain_enum domain);
static struct type *rust_lookup_type (const char *name,
				      const struct block *block);
static struct type *rust_type (const char *name);

static const struct rust_op *crate_name (const struct rust_op *name);
static const struct rust_op *super_name (const struct rust_op *name,
					 unsigned int n_supers);

static const struct rust_op *ast_operation (enum exp_opcode opcode,
					    const struct rust_op *left,
					    const struct rust_op *right);
static const struct rust_op *ast_compound_assignment
  (enum exp_opcode opcode, const struct rust_op *left,
   const struct rust_op *rust_op);
static const struct rust_op *ast_literal (struct typed_val_int val);
static const struct rust_op *ast_dliteral (struct typed_val_float val);
static const struct rust_op *ast_structop (const struct rust_op *left,
					   const char *name,
					   int completing);
static const struct rust_op *ast_structop_anonymous
  (const struct rust_op *left, struct typed_val_int number);
static const struct rust_op *ast_unary (enum exp_opcode opcode,
					const struct rust_op *expr);
static const struct rust_op *ast_cast (const struct rust_op *expr,
				       const struct rust_op *type);
static const struct rust_op *ast_call_ish (enum exp_opcode opcode,
					   const struct rust_op *expr,
					   VEC (rust_op_ptr) **params);
static const struct rust_op *ast_path (struct stoken name,
				       VEC (rust_op_ptr) **params);
static const struct rust_op *ast_string (struct stoken str);
static const struct rust_op *ast_struct (const struct rust_op *name,
					 VEC (set_field) **fields);
static const struct rust_op *ast_range (const struct rust_op *lhs,
					const struct rust_op *rhs);
static const struct rust_op *ast_array_type (const struct rust_op *lhs,
					     struct typed_val_int val);
static const struct rust_op *ast_slice_type (const struct rust_op *type);
static const struct rust_op *ast_reference_type (const struct rust_op *type);
static const struct rust_op *ast_pointer_type (const struct rust_op *type,
					       int is_mut);
static const struct rust_op *ast_function_type (const struct rust_op *result,
						VEC (rust_op_ptr) **params);
static const struct rust_op *ast_tuple_type (VEC (rust_op_ptr) **params);

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* A regular expression for matching Rust numbers.  This is split up
   since it is very long and this gives us a way to comment the
   sections.  */

static const char *number_regex_text =
  /* subexpression 1: allows use of alternation, otherwise uninteresting */
  "^("
  /* First comes floating point.  */
  /* Recognize number after the decimal point, with optional
     exponent and optional type suffix.
     subexpression 2: allows "?", otherwise uninteresting
     subexpression 3: if present, type suffix
  */
  "[0-9][0-9_]*\\.[0-9][0-9_]*([eE][-+]?[0-9][0-9_]*)?(f32|f64)?"
#define FLOAT_TYPE1 3
  "|"
  /* Recognize exponent without decimal point, with optional type
     suffix.
     subexpression 4: if present, type suffix
  */
#define FLOAT_TYPE2 4
  "[0-9][0-9_]*[eE][-+]?[0-9][0-9_]*(f32|f64)?"
  "|"
  /* "23." is a valid floating point number, but "23.e5" and
     "23.f32" are not.  So, handle the trailing-. case
     separately.  */
  "[0-9][0-9_]*\\."
  "|"
  /* Finally come integers.
     subexpression 5: text of integer
     subexpression 6: if present, type suffix
     subexpression 7: allows use of alternation, otherwise uninteresting
  */
#define INT_TEXT 5
#define INT_TYPE 6
  "(0x[a-fA-F0-9_]+|0o[0-7_]+|0b[01_]+|[0-9][0-9_]*)"
  "([iu](size|8|16|32|64))?"
  ")";
/* The number of subexpressions to allocate space for, including the
   "0th" whole match subexpression.  */
#define NUM_SUBEXPRESSIONS 8

/* The compiled number-matching regex.  */

static regex_t number_regex;

/* True if we're running unit tests.  */

static int unit_testing;

/* Obstack for data temporarily allocated during parsing.  */

static struct obstack work_obstack;

/* Result of parsing.  Points into work_obstack.  */

static const struct rust_op *rust_ast;

%}

%union
{
  /* A typed integer constant.  */
  struct typed_val_int typed_val_int;

  /* A typed floating point constant.  */
  struct typed_val_float typed_val_float;

  /* An identifier or string.  */
  struct stoken sval;

  /* A token representing an opcode, like "==".  */
  enum exp_opcode opcode;

  /* A list of expressions; for example, the arguments to a function
     call.  */
  VEC (rust_op_ptr) **params;

  /* A list of field initializers.  */
  VEC (set_field) **field_inits;

  /* A single field initializer.  */
  struct set_field one_field_init;

  /* An expression.  */
  const struct rust_op *op;

  /* A plain integer, for example used to count the number of
     "super::" prefixes on a path.  */
  unsigned int depth;
}

%{

  /* Rust AST operations.  We build a tree of these; then lower them
     to gdb expressions when parsing has completed.  */

struct rust_op
{
  /* The opcode.  */
  enum exp_opcode opcode;
  /* If OPCODE is OP_TYPE, then this holds information about what type
     is described by this node.  */
  enum type_code typecode;
  /* Indicates whether OPCODE actually represents a compound
     assignment.  For example, if OPCODE is GTGT and this is false,
     then this rust_op represents an ordinary ">>"; but if this is
     true, then this rust_op represents ">>=".  Unused in other
     cases.  */
  unsigned int compound_assignment : 1;
  /* Only used by a field expression; if set, indicates that the field
     name occurred at the end of the expression and is eligible for
     completion.  */
  unsigned int completing : 1;
  /* Operands of expression.  Which one is used and how depends on the
     particular opcode.  */
  RUSTSTYPE left;
  RUSTSTYPE right;
};

%}

%token <sval> GDBVAR
%token <sval> IDENT
%token <sval> COMPLETE
%token <typed_val_int> INTEGER
%token <typed_val_int> DECIMAL_INTEGER
%token <sval> STRING
%token <sval> BYTESTRING
%token <typed_val_float> FLOAT
%token <opcode> COMPOUND_ASSIGN

/* Keyword tokens.  */
%token <voidval> KW_AS
%token <voidval> KW_IF
%token <voidval> KW_TRUE
%token <voidval> KW_FALSE
%token <voidval> KW_SUPER
%token <voidval> KW_SELF
%token <voidval> KW_MUT
%token <voidval> KW_EXTERN
%token <voidval> KW_CONST
%token <voidval> KW_FN

/* Operator tokens.  */
%token <voidval> DOTDOT
%token <voidval> OROR
%token <voidval> ANDAND
%token <voidval> EQEQ
%token <voidval> NOTEQ
%token <voidval> LTEQ
%token <voidval> GTEQ
%token <voidval> LSH RSH
%token <voidval> COLONCOLON
%token <voidval> ARROW

%type <op> type
%type <op> path_for_expr
%type <op> identifier_path_for_expr
%type <op> path_for_type
%type <op> identifier_path_for_type
%type <op> just_identifiers_for_type

%type <params> maybe_type_list
%type <params> type_list

%type <depth> super_path

%type <op> literal
%type <op> expr
%type <op> field_expr
%type <op> idx_expr
%type <op> unop_expr
%type <op> binop_expr
%type <op> binop_expr_expr
%type <op> type_cast_expr
%type <op> assignment_expr
%type <op> compound_assignment_expr
%type <op> paren_expr
%type <op> call_expr
%type <op> path_expr
%type <op> tuple_expr
%type <op> unit_expr
%type <op> struct_expr
%type <op> array_expr
%type <op> range_expr

%type <params> expr_list
%type <params> maybe_expr_list
%type <params> paren_expr_list

%type <field_inits> struct_expr_list
%type <one_field_init> struct_expr_tail

/* Precedence.  */
%nonassoc DOTDOT
%right '=' COMPOUND_ASSIGN
%left OROR
%left ANDAND
%nonassoc EQEQ NOTEQ '<' '>' LTEQ GTEQ
%left '|'
%left '^'
%left '&'
%left LSH RSH
%left '@'
%left '+' '-'
%left '*' '/' '%'
/* These could be %precedence in Bison, but that isn't a yacc
   feature.  */
%left KW_AS
%left UNARY
%left '[' '.' '('

%%

start:
	expr
		{
		  /* If we are completing and see a valid parse,
		     rust_ast will already have been set.  */
		  if (rust_ast == NULL)
		    rust_ast = $1;
		}
;

/* Note that the Rust grammar includes a method_call_expr, but we
   handle this differently, to avoid a shift/reduce conflict with
   call_expr.  */
expr:
	literal
|	path_expr
|	tuple_expr
|	unit_expr
|	struct_expr
|	field_expr
|	array_expr
|	idx_expr
|	range_expr
|	unop_expr
|	binop_expr
|	paren_expr
|	call_expr
;

tuple_expr:
	'(' expr ',' maybe_expr_list ')'
		{
		  VEC_safe_insert (rust_op_ptr, *$4, 0, $2);
		  error (_("Tuple expressions not supported yet"));
		}
;

unit_expr:
	'(' ')'
		{
		  struct typed_val_int val;

		  val.type
		    = language_lookup_primitive_type (parse_language (pstate),
						      parse_gdbarch (pstate),
						      "()");
		  val.val = 0;
		  $$ = ast_literal (val);
		}
;

/* To avoid a shift/reduce conflict with call_expr, we don't handle
   tuple struct expressions here, but instead when examining the
   AST.  */
struct_expr:
	path_for_expr '{' struct_expr_list '}'
		{ $$ = ast_struct ($1, $3); }
;

struct_expr_tail:
	DOTDOT expr
		{
		  struct set_field sf;

		  sf.name.ptr = NULL;
		  sf.name.length = 0;
		  sf.init = $2;

		  $$ = sf;
		}
|	IDENT ':' expr
		{
		  struct set_field sf;

		  sf.name = $1;
		  sf.init = $3;
		  $$ = sf;
		}
;

struct_expr_list:
	/* %empty */
		{
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  $$ = result;
		}
|	struct_expr_tail
		{
		  VEC (set_field) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);

		  make_cleanup (VEC_cleanup (set_field), result);
		  VEC_safe_push (set_field, *result, &$1);

		  $$ = result;
		}
|	IDENT ':' expr ',' struct_expr_list
		{
		  struct set_field sf;

		  sf.name = $1;
		  sf.init = $3;
		  VEC_safe_push (set_field, *$5, &sf);
		  $$ = $5;
		}
;

array_expr:
	'[' KW_MUT expr_list ']'
		{ $$ = ast_call_ish (OP_ARRAY, NULL, $3); }
|	'[' expr_list ']'
		{ $$ = ast_call_ish (OP_ARRAY, NULL, $2); }
|	'[' KW_MUT expr ';' expr ']'
		{ $$ = ast_operation (OP_RUST_ARRAY, $3, $5); }
|	'[' expr ';' expr ']'
		{ $$ = ast_operation (OP_RUST_ARRAY, $2, $4); }
;

range_expr:
	expr DOTDOT
		{ $$ = ast_range ($1, NULL); }
|	expr DOTDOT expr
		{ $$ = ast_range ($1, $3); }
|	DOTDOT expr
		{ $$ = ast_range (NULL, $2); }
|	DOTDOT
		{ $$ = ast_range (NULL, NULL); }
;

literal:
	INTEGER
		{ $$ = ast_literal ($1); }
|	DECIMAL_INTEGER
		{ $$ = ast_literal ($1); }
|	FLOAT
		{ $$ = ast_dliteral ($1); }
|	STRING
		{
		  const struct rust_op *str = ast_string ($1);
		  VEC (set_field) **fields;
		  struct set_field field;
		  struct typed_val_int val;
		  struct stoken token;

		  fields = OBSTACK_ZALLOC (&work_obstack, VEC (set_field) *);
		  make_cleanup (VEC_cleanup (set_field), fields);

		  /* Wrap the raw string in the &str struct.  */
		  field.name.ptr = "data_ptr";
		  field.name.length = strlen (field.name.ptr);
		  field.init = ast_unary (UNOP_ADDR, ast_string ($1));
		  VEC_safe_push (set_field, *fields, &field);

		  val.type = rust_type ("usize");
		  val.val = $1.length;

		  field.name.ptr = "length";
		  field.name.length = strlen (field.name.ptr);
		  field.init = ast_literal (val);
		  VEC_safe_push (set_field, *fields, &field);

		  token.ptr = "&str";
		  token.length = strlen (token.ptr);
		  $$ = ast_struct (ast_path (token, NULL), fields);
		}
|	BYTESTRING
		{ $$ = ast_string ($1); }
|	KW_TRUE
		{
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 1;
		  $$ = ast_literal (val);
		}
|	KW_FALSE
		{
		  struct typed_val_int val;

		  val.type = language_bool_type (parse_language (pstate),
						 parse_gdbarch (pstate));
		  val.val = 0;
		  $$ = ast_literal (val);
		}
;

field_expr:
	expr '.' IDENT
		{ $$ = ast_structop ($1, $3.ptr, 0); }
|	expr '.' COMPLETE
		{
		  $$ = ast_structop ($1, $3.ptr, 1);
		  rust_ast = $$;
		}
|	expr '.' DECIMAL_INTEGER
		{ $$ = ast_structop_anonymous ($1, $3); }
;

idx_expr:
	expr '[' expr ']'
		{ $$ = ast_operation (BINOP_SUBSCRIPT, $1, $3); }
;

unop_expr:
	'+' expr	%prec UNARY
		{ $$ = ast_unary (UNOP_PLUS, $2); }

|	'-' expr	%prec UNARY
		{ $$ = ast_unary (UNOP_NEG, $2); }

|	'!' expr	%prec UNARY
		{
		  /* Note that we provide a Rust-specific evaluator
		     override for UNOP_COMPLEMENT, so it can do the
		     right thing for both bool and integral
		     values.  */
		  $$ = ast_unary (UNOP_COMPLEMENT, $2);
		}

|	'*' expr	%prec UNARY
		{ $$ = ast_unary (UNOP_IND, $2); }

|	'&' expr	%prec UNARY
		{ $$ = ast_unary (UNOP_ADDR, $2); }

|	'&' KW_MUT expr	%prec UNARY
		{ $$ = ast_unary (UNOP_ADDR, $3); }

;

binop_expr:
	binop_expr_expr
|	type_cast_expr
|	assignment_expr
|	compound_assignment_expr
;

binop_expr_expr:
	expr '*' expr
		{ $$ = ast_operation (BINOP_MUL, $1, $3); }

|	expr '@' expr
		{ $$ = ast_operation (BINOP_REPEAT, $1, $3); }

|	expr '/' expr
		{ $$ = ast_operation (BINOP_DIV, $1, $3); }

|	expr '%' expr
		{ $$ = ast_operation (BINOP_REM, $1, $3); }

|	expr '<' expr
		{ $$ = ast_operation (BINOP_LESS, $1, $3); }

|	expr '>' expr
		{ $$ = ast_operation (BINOP_GTR, $1, $3); }

|	expr '&' expr
		{ $$ = ast_operation (BINOP_BITWISE_AND, $1, $3); }

|	expr '|' expr
		{ $$ = ast_operation (BINOP_BITWISE_IOR, $1, $3); }

|	expr '^' expr
		{ $$ = ast_operation (BINOP_BITWISE_XOR, $1, $3); }

|	expr '+' expr
		{ $$ = ast_operation (BINOP_ADD, $1, $3); }

|	expr '-' expr
		{ $$ = ast_operation (BINOP_SUB, $1, $3); }

|	expr OROR expr
		{ $$ = ast_operation (BINOP_LOGICAL_OR, $1, $3); }

|	expr ANDAND expr
		{ $$ = ast_operation (BINOP_LOGICAL_AND, $1, $3); }

|	expr EQEQ expr
		{ $$ = ast_operation (BINOP_EQUAL, $1, $3); }

|	expr NOTEQ expr
		{ $$ = ast_operation (BINOP_NOTEQUAL, $1, $3); }

|	expr LTEQ expr
		{ $$ = ast_operation (BINOP_LEQ, $1, $3); }

|	expr GTEQ expr
		{ $$ = ast_operation (BINOP_GEQ, $1, $3); }

|	expr LSH expr
		{ $$ = ast_operation (BINOP_LSH, $1, $3); }

|	expr RSH expr
		{ $$ = ast_operation (BINOP_RSH, $1, $3); }
;

type_cast_expr:
	expr KW_AS type
		{ $$ = ast_cast ($1, $3); }
;

assignment_expr:
	expr '=' expr
		{ $$ = ast_operation (BINOP_ASSIGN, $1, $3); }
;

compound_assignment_expr:
	expr COMPOUND_ASSIGN expr
		{ $$ = ast_compound_assignment ($2, $1, $3); }

;

paren_expr:
	'(' expr ')'
		{ $$ = $2; }
;

expr_list:
	expr
		{
		  $$ = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), $$);
		  VEC_safe_push (rust_op_ptr, *$$, $1);
		}
|	expr_list ',' expr
		{
		  VEC_safe_push (rust_op_ptr, *$1, $3);
		  $$ = $1;
		}
;

maybe_expr_list:
	/* %empty */
		{
		  /* The result can't be NULL.  */
		  $$ = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);
		  make_cleanup (VEC_cleanup (rust_op_ptr), $$);
		}
|	expr_list
		{ $$ = $1; }
;

paren_expr_list:
	'('
	maybe_expr_list
	')'
		{ $$ = $2; }
;

call_expr:
	expr paren_expr_list
		{ $$ = ast_call_ish (OP_FUNCALL, $1, $2); }
;

maybe_self_path:
	/* %empty */
|	KW_SELF COLONCOLON
;

super_path:
	KW_SUPER COLONCOLON
		{ $$ = 1; }
|	super_path KW_SUPER COLONCOLON
		{ $$ = $1 + 1; }
;

path_expr:
	path_for_expr
		{ $$ = $1; }
|	GDBVAR
		{ $$ = ast_path ($1, NULL); }
|	KW_SELF
		{ $$ = ast_path (make_stoken ("self"), NULL); }
;

path_for_expr:
	identifier_path_for_expr
|	KW_SELF COLONCOLON identifier_path_for_expr
		{ $$ = super_name ($3, 0); }
|	maybe_self_path super_path identifier_path_for_expr
		{ $$ = super_name ($3, $2); }
|	COLONCOLON identifier_path_for_expr
		{ $$ = crate_name ($2); }
|	KW_EXTERN identifier_path_for_expr
		{
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  $$ = ast_path (rust_concat3 ("::", $2->left.sval.ptr, NULL),
				 $2->right.params);
		}
;

identifier_path_for_expr:
	IDENT
		{ $$ = ast_path ($1, NULL); }
|	identifier_path_for_expr COLONCOLON IDENT
		{
		  $$ = ast_path (rust_concat3 ($1->left.sval.ptr, "::",
					       $3.ptr),
				 NULL);
		}
|	identifier_path_for_expr COLONCOLON '<' type_list '>'
		{ $$ = ast_path ($1->left.sval, $4); }
|	identifier_path_for_expr COLONCOLON '<' type_list RSH
		{
		  $$ = ast_path ($1->left.sval, $4);
		  rust_push_back ('>');
		}
;

path_for_type:
	identifier_path_for_type
|	KW_SELF COLONCOLON identifier_path_for_type
		{ $$ = super_name ($3, 0); }
|	maybe_self_path super_path identifier_path_for_type
		{ $$ = super_name ($3, $2); }
|	COLONCOLON identifier_path_for_type
		{ $$ = crate_name ($2); }
|	KW_EXTERN identifier_path_for_type
		{
		  /* This is a gdb extension to make it possible to
		     refer to items in other crates.  It just bypasses
		     adding the current crate to the front of the
		     name.  */
		  $$ = ast_path (rust_concat3 ("::", $2->left.sval.ptr, NULL),
				 $2->right.params);
		}
;

just_identifiers_for_type:
	IDENT
	  	{ $$ = ast_path ($1, NULL); }
|	just_identifiers_for_type COLONCOLON IDENT
		{
		  $$ = ast_path (rust_concat3 ($1->left.sval.ptr, "::",
					       $3.ptr),
				 NULL);
		}
;

identifier_path_for_type:
	just_identifiers_for_type
|	just_identifiers_for_type '<' type_list '>'
		{ $$ = ast_path ($1->left.sval, $3); }
|	just_identifiers_for_type '<' type_list RSH
		{
		  $$ = ast_path ($1->left.sval, $3);
		  rust_push_back ('>');
		}
;

type:
	path_for_type
|	'[' type ';' INTEGER ']'
		{ $$ = ast_array_type ($2, $4); }
|	'[' type ';' DECIMAL_INTEGER ']'
		{ $$ = ast_array_type ($2, $4); }
|	'&' '[' type ']'
		{ $$ = ast_slice_type ($3); }
|	'&' type
		{ $$ = ast_reference_type ($2); }
|	'*' KW_MUT type
		{ $$ = ast_pointer_type ($3, 1); }
|	'*' KW_CONST type
		{ $$ = ast_pointer_type ($3, 0); }
|	KW_FN '(' maybe_type_list ')' ARROW type
		{ $$ = ast_function_type ($6, $3); }
|	'(' maybe_type_list ')'
		{ $$ = ast_tuple_type ($2); }
;

maybe_type_list:
	/* %empty */
		{ $$ = NULL; }
|	type_list
		{ $$ = $1; }
;

type_list:
	type
		{
		  VEC (rust_op_ptr) **result
		    = OBSTACK_ZALLOC (&work_obstack, VEC (rust_op_ptr) *);

		  make_cleanup (VEC_cleanup (rust_op_ptr), result);
		  VEC_safe_push (rust_op_ptr, *result, $1);
		  $$ = result;
		}
|	type_list ',' type
		{
		  VEC_safe_push (rust_op_ptr, *$1, $3);
		  $$ = $1;
		}
;

%%

/* A struct of this type is used to describe a token.  */

struct token_info
{
  const char *name;
  int value;
  enum exp_opcode opcode;
};

/* Identifier tokens.  */

static const struct token_info identifier_tokens[] =
{
  { "as", KW_AS, OP_NULL },
  { "false", KW_FALSE, OP_NULL },
  { "if", 0, OP_NULL },
  { "mut", KW_MUT, OP_NULL },
  { "const", KW_CONST, OP_NULL },
  { "self", KW_SELF, OP_NULL },
  { "super", KW_SUPER, OP_NULL },
  { "true", KW_TRUE, OP_NULL },
  { "extern", KW_EXTERN, OP_NULL },
  { "fn", KW_FN, OP_NULL },
};

/* Operator tokens, sorted longest first.  */

static const struct token_info operator_tokens[] =
{
  { ">>=", COMPOUND_ASSIGN, BINOP_RSH },
  { "<<=", COMPOUND_ASSIGN, BINOP_LSH },

  { "<<", LSH, OP_NULL },
  { ">>", RSH, OP_NULL },
  { "&&", ANDAND, OP_NULL },
  { "||", OROR, OP_NULL },
  { "==", EQEQ, OP_NULL },
  { "!=", NOTEQ, OP_NULL },
  { "<=", LTEQ, OP_NULL },
  { ">=", GTEQ, OP_NULL },
  { "+=", COMPOUND_ASSIGN, BINOP_ADD },
  { "-=", COMPOUND_ASSIGN, BINOP_SUB },
  { "*=", COMPOUND_ASSIGN, BINOP_MUL },
  { "/=", COMPOUND_ASSIGN, BINOP_DIV },
  { "%=", COMPOUND_ASSIGN, BINOP_REM },
  { "&=", COMPOUND_ASSIGN, BINOP_BITWISE_AND },
  { "|=", COMPOUND_ASSIGN, BINOP_BITWISE_IOR },
  { "^=", COMPOUND_ASSIGN, BINOP_BITWISE_XOR },

  { "::", COLONCOLON, OP_NULL },
  { "..", DOTDOT, OP_NULL },
  { "->", ARROW, OP_NULL }
};

/* Helper function to copy to the name obstack.  */

static const char *
rust_copy_name (const char *name, int len)
{
  return (const char *) obstack_copy0 (&work_obstack, name, len);
}

/* Helper function to make an stoken from a C string.  */

static struct stoken
make_stoken (const char *p)
{
  struct stoken result;

  result.ptr = p;
  result.length = strlen (result.ptr);
  return result;
}

/* Helper function to concatenate three strings on the name
   obstack.  */

static struct stoken
rust_concat3 (const char *s1, const char *s2, const char *s3)
{
  return make_stoken (obconcat (&work_obstack, s1, s2, s3, (char *) NULL));
}

/* Return an AST node referring to NAME, but relative to the crate's
   name.  */

static const struct rust_op *
crate_name (const struct rust_op *name)
{
  char *crate = rust_crate_for_block (expression_context_block);
  struct stoken result;

  gdb_assert (name->opcode == OP_VAR_VALUE);

  if (crate == NULL)
    error (_("Could not find crate for current location"));
  result = make_stoken (obconcat (&work_obstack, "::", crate, "::",
				  name->left.sval.ptr, (char *) NULL));
  xfree (crate);

  return ast_path (result, name->right.params);
}

/* Create an AST node referring to a "super::" qualified name.  IDENT
   is the base name and N_SUPERS is how many "super::"s were
   provided.  N_SUPERS can be zero.  */

static const struct rust_op *
super_name (const struct rust_op *ident, unsigned int n_supers)
{
  const char *scope = block_scope (expression_context_block);
  int offset;

  gdb_assert (ident->opcode == OP_VAR_VALUE);

  if (scope[0] == '\0')
    error (_("Couldn't find namespace scope for self::"));

  if (n_supers > 0)
    {
      int i;
      int len;
      VEC (int) *offsets = NULL;
      unsigned int current_len;
      struct cleanup *cleanup;

      cleanup = make_cleanup (VEC_cleanup (int), &offsets);
      current_len = cp_find_first_component (scope);
      while (scope[current_len] != '\0')
	{
	  VEC_safe_push (int, offsets, current_len);
	  gdb_assert (scope[current_len] == ':');
	  /* The "::".  */
	  current_len += 2;
	  current_len += cp_find_first_component (scope
						  + current_len);
	}

      len = VEC_length (int, offsets);
      if (n_supers >= len)
	error (_("Too many super:: uses from '%s'"), scope);

      offset = VEC_index (int, offsets, len - n_supers);

      do_cleanups (cleanup);
    }
  else
    offset = strlen (scope);

  obstack_grow (&work_obstack, "::", 2);
  obstack_grow (&work_obstack, scope, offset);
  obstack_grow (&work_obstack, "::", 2);
  obstack_grow0 (&work_obstack, ident->left.sval.ptr, ident->left.sval.length);

  return ast_path (make_stoken ((const char *) obstack_finish (&work_obstack)),
		   ident->right.params);
}

/* A helper that updates innermost_block as appropriate.  */

static void
update_innermost_block (struct block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol)
      && (innermost_block == NULL
	  || contained_in (sym.block, innermost_block)))
    innermost_block = sym.block;
}

/* A helper to look up a Rust type, or fail.  This only works for
   types defined by rust_language_arch_info.  */

static struct type *
rust_type (const char *name)
{
  struct type *type;

  /* When unit testing, we don't bother checking the types, so avoid a
     possibly-failing lookup here.  */
  if (unit_testing)
    return NULL;

  type = language_lookup_primitive_type (parse_language (pstate),
					 parse_gdbarch (pstate),
					 name);
  if (type == NULL)
    error (_("Could not find Rust type %s"), name);
  return type;
}

/* Lex a hex number with at least MIN digits and at most MAX
   digits.  */

static uint32_t
lex_hex (int min, int max)
{
  uint32_t result = 0;
  int len = 0;
  /* We only want to stop at MAX if we're lexing a byte escape.  */
  int check_max = min == max;

  while ((check_max ? len <= max : 1)
	 && ((lexptr[0] >= 'a' && lexptr[0] <= 'f')
	     || (lexptr[0] >= 'A' && lexptr[0] <= 'F')
	     || (lexptr[0] >= '0' && lexptr[0] <= '9')))
    {
      result *= 16;
      if (lexptr[0] >= 'a' && lexptr[0] <= 'f')
	result = result + 10 + lexptr[0] - 'a';
      else if (lexptr[0] >= 'A' && lexptr[0] <= 'F')
	result = result + 10 + lexptr[0] - 'A';
      else
	result = result + lexptr[0] - '0';
      ++lexptr;
      ++len;
    }

  if (len < min)
    error (_("Not enough hex digits seen"));
  if (len > max)
    {
      gdb_assert (min != max);
      error (_("Overlong hex escape"));
    }

  return result;
}

/* Lex an escape.  IS_BYTE is true if we're lexing a byte escape;
   otherwise we're lexing a character escape.  */

static uint32_t
lex_escape (int is_byte)
{
  uint32_t result;

  gdb_assert (lexptr[0] == '\\');
  ++lexptr;
  switch (lexptr[0])
    {
    case 'x':
      ++lexptr;
      result = lex_hex (2, 2);
      break;

    case 'u':
      if (is_byte)
	error (_("Unicode escape in byte literal"));
      ++lexptr;
      if (lexptr[0] != '{')
	error (_("Missing '{' in Unicode escape"));
      ++lexptr;
      result = lex_hex (1, 6);
      /* Could do range checks here.  */
      if (lexptr[0] != '}')
	error (_("Missing '}' in Unicode escape"));
      ++lexptr;
      break;

    case 'n':
      result = '\n';
      ++lexptr;
      break;
    case 'r':
      result = '\r';
      ++lexptr;
      break;
    case 't':
      result = '\t';
      ++lexptr;
      break;
    case '\\':
      result = '\\';
      ++lexptr;
      break;
    case '0':
      result = '\0';
      ++lexptr;
      break;
    case '\'':
      result = '\'';
      ++lexptr;
      break;
    case '"':
      result = '"';
      ++lexptr;
      break;

    default:
      error (_("Invalid escape \\%c in literal"), lexptr[0]);
    }

  return result;
}

/* Lex a character constant.  */

static int
lex_character (void)
{
  int is_byte = 0;
  uint32_t value;

  if (lexptr[0] == 'b')
    {
      is_byte = 1;
      ++lexptr;
    }
  gdb_assert (lexptr[0] == '\'');
  ++lexptr;
  /* This should handle UTF-8 here.  */
  if (lexptr[0] == '\\')
    value = lex_escape (is_byte);
  else
    {
      value = lexptr[0] & 0xff;
      ++lexptr;
    }

  if (lexptr[0] != '\'')
    error (_("Unterminated character literal"));
  ++lexptr;

  rustyylval.typed_val_int.val = value;
  rustyylval.typed_val_int.type = rust_type (is_byte ? "u8" : "char");

  return INTEGER;
}

/* Return the offset of the double quote if STR looks like the start
   of a raw string, or 0 if STR does not start a raw string.  */

static int
starts_raw_string (const char *str)
{
  const char *save = str;

  if (str[0] != 'r')
    return 0;
  ++str;
  while (str[0] == '#')
    ++str;
  if (str[0] == '"')
    return str - save;
  return 0;
}

/* Return true if STR looks like the end of a raw string that had N
   hashes at the start.  */

static int
ends_raw_string (const char *str, int n)
{
  int i;

  gdb_assert (str[0] == '"');
  for (i = 0; i < n; ++i)
    if (str[i + 1] != '#')
      return 0;
  return 1;
}

/* Lex a string constant.  */

static int
lex_string (void)
{
  int is_byte = lexptr[0] == 'b';
  int raw_length;
  int len_in_chars = 0;

  if (is_byte)
    ++lexptr;
  raw_length = starts_raw_string (lexptr);
  lexptr += raw_length;
  gdb_assert (lexptr[0] == '"');
  ++lexptr;

  while (1)
    {
      uint32_t value;

      if (raw_length > 0)
	{
	  if (lexptr[0] == '"' && ends_raw_string (lexptr, raw_length - 1))
	    {
	      /* Exit with lexptr pointing after the final "#".  */
	      lexptr += raw_length;
	      break;
	    }
	  else if (lexptr[0] == '\0')
	    error (_("Unexpected EOF in string"));

	  value = lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in raw byte string"));
	  obstack_1grow (&work_obstack, value);

	  ++lexptr;
	}
      else if (lexptr[0] == '"')
	{
	  /* Make sure to skip the quote.  */
	  ++lexptr;
	  break;
	}
      else if (lexptr[0] == '\\')
	{
	  value = lex_escape (is_byte);

	  if (is_byte)
	    obstack_1grow (&work_obstack, value);
	  else
	    convert_between_encodings ("UTF-32", "UTF-8", (gdb_byte *) &value,
				       sizeof (value), sizeof (value),
				       &work_obstack, translit_none);
	}
      else if (lexptr[0] == '\0')
	error (_("Unexpected EOF in string"));
      else
	{
	  value = lexptr[0] & 0xff;
	  if (is_byte && value > 127)
	    error (_("Non-ASCII value in byte string"));
	  obstack_1grow (&work_obstack, value);
	  ++lexptr;
	}
    }

  rustyylval.sval.length = obstack_object_size (&work_obstack);
  rustyylval.sval.ptr = (const char *) obstack_finish (&work_obstack);
  return is_byte ? BYTESTRING : STRING;
}

/* Return true if STRING starts with whitespace followed by a digit.  */

static int
space_then_number (const char *string)
{
  const char *p = string;

  while (p[0] == ' ' || p[0] == '\t')
    ++p;
  if (p == string)
    return 0;

  return *p >= '0' && *p <= '9';
}

/* Return true if C can start an identifier.  */

static int
rust_identifier_start_p (char c)
{
  return ((c >= 'a' && c <= 'z')
	  || (c >= 'A' && c <= 'Z')
	  || c == '_'
	  || c == '$');
}

/* Lex an identifier.  */

static int
lex_identifier (void)
{
  const char *start = lexptr;
  unsigned int length;
  const struct token_info *token;
  int i;
  int is_gdb_var = lexptr[0] == '$';

  gdb_assert (rust_identifier_start_p (lexptr[0]));

  ++lexptr;

  /* For the time being this doesn't handle Unicode rules.  Non-ASCII
     identifiers are gated anyway.  */
  while ((lexptr[0] >= 'a' && lexptr[0] <= 'z')
	 || (lexptr[0] >= 'A' && lexptr[0] <= 'Z')
	 || lexptr[0] == '_'
	 || (is_gdb_var && lexptr[0] == '$')
	 || (lexptr[0] >= '0' && lexptr[0] <= '9'))
    ++lexptr;


  length = lexptr - start;
  token = NULL;
  for (i = 0; i < ARRAY_SIZE (identifier_tokens); ++i)
    {
      if (length == strlen (identifier_tokens[i].name)
	  && strncmp (identifier_tokens[i].name, start, length) == 0)
	{
	  token = &identifier_tokens[i];
	  break;
	}
    }

  if (token != NULL)
    {
      if (token->value == 0)
	{
	  /* Leave the terminating token alone.  */
	  lexptr = start;
	  return 0;
	}
    }
  else if (token == NULL
	   && (strncmp (start, "thread", length) == 0
	       || strncmp (start, "task", length) == 0)
	   && space_then_number (lexptr))
    {
      /* "task" or "thread" followed by a number terminates the
	 parse, per gdb rules.  */
      lexptr = start;
      return 0;
    }

  if (token == NULL || (parse_completion && lexptr[0] == '\0'))
    rustyylval.sval = make_stoken (rust_copy_name (start, length));

  if (parse_completion && lexptr[0] == '\0')
    {
      /* Prevent rustyylex from returning two COMPLETE tokens.  */
      prev_lexptr = lexptr;
      return COMPLETE;
    }

  if (token != NULL)
    return token->value;
  if (is_gdb_var)
    return GDBVAR;
  return IDENT;
}

/* Lex an operator.  */

static int
lex_operator (void)
{
  const struct token_info *token = NULL;
  int i;

  for (i = 0; i < ARRAY_SIZE (operator_tokens); ++i)
    {
      if (strncmp (operator_tokens[i].name, lexptr,
		   strlen (operator_tokens[i].name)) == 0)
	{
	  lexptr += strlen (operator_tokens[i].name);
	  token = &operator_tokens[i];
	  break;
	}
    }

  if (token != NULL)
    {
      rustyylval.opcode = token->opcode;
      return token->value;
    }

  return *lexptr++;
}

/* Lex a number.  */

static int
lex_number (void)
{
  regmatch_t subexps[NUM_SUBEXPRESSIONS];
  int match;
  int is_integer = 0;
  int could_be_decimal = 1;
  int implicit_i32 = 0;
  char *type_name = NULL;
  struct type *type;
  int end_index;
  int type_index = -1;
  int i, out;
  char *number;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);

  match = regexec (&number_regex, lexptr, ARRAY_SIZE (subexps), subexps, 0);
  /* Failure means the regexp is broken.  */
  gdb_assert (match == 0);

  if (subexps[INT_TEXT].rm_so != -1)
    {
      /* Integer part matched.  */
      is_integer = 1;
      end_index = subexps[INT_TEXT].rm_eo;
      if (subexps[INT_TYPE].rm_so == -1)
	{
	  type_name = "i32";
	  implicit_i32 = 1;
	}
      else
	{
	  type_index = INT_TYPE;
	  could_be_decimal = 0;
	}
    }
  else if (subexps[FLOAT_TYPE1].rm_so != -1)
    {
      /* Found floating point type suffix.  */
      end_index = subexps[FLOAT_TYPE1].rm_so;
      type_index = FLOAT_TYPE1;
    }
  else if (subexps[FLOAT_TYPE2].rm_so != -1)
    {
      /* Found floating point type suffix.  */
      end_index = subexps[FLOAT_TYPE2].rm_so;
      type_index = FLOAT_TYPE2;
    }
  else
    {
      /* Any other floating point match.  */
      end_index = subexps[0].rm_eo;
      type_name = "f64";
    }

  /* We need a special case if the final character is ".".  In this
     case we might need to parse an integer.  For example, "23.f()" is
     a request for a trait method call, not a syntax error involving
     the floating point number "23.".  */
  gdb_assert (subexps[0].rm_eo > 0);
  if (lexptr[subexps[0].rm_eo - 1] == '.')
    {
      const char *next = skip_spaces_const (&lexptr[subexps[0].rm_eo]);

      if (rust_identifier_start_p (*next) || *next == '.')
	{
	  --subexps[0].rm_eo;
	  is_integer = 1;
	  end_index = subexps[0].rm_eo;
	  type_name = "i32";
	  could_be_decimal = 1;
	  implicit_i32 = 1;
	}
    }

  /* Compute the type name if we haven't already.  */
  if (type_name == NULL)
    {
      gdb_assert (type_index != -1);
      type_name = xstrndup (lexptr + subexps[type_index].rm_so,
			   (subexps[type_index].rm_eo
			    - subexps[type_index].rm_so));
      make_cleanup (xfree, type_name);
    }

  /* Look up the type.  */
  type = rust_type (type_name);

  /* Copy the text of the number and remove the "_"s.  */
  number = xstrndup (lexptr, end_index);
  make_cleanup (xfree, number);
  for (i = out = 0; number[i]; ++i)
    {
      if (number[i] == '_')
	could_be_decimal = 0;
      else
	number[out++] = number[i];
    }
  number[out] = '\0';

  /* Advance past the match.  */
  lexptr += subexps[0].rm_eo;

  /* Parse the number.  */
  if (is_integer)
    {
      uint64_t value;
      int radix = 10;
      if (number[0] == '0')
	{
	  if (number[1] == 'x')
	    radix = 16;
	  else if (number[1] == 'o')
	    radix = 8;
	  else if (number[1] == 'b')
	    radix = 2;
	  if (radix != 10)
	    {
	      number += 2;
	      could_be_decimal = 0;
	    }
	}

      value = strtoul (number, NULL, radix);
      if (implicit_i32 && value >= ((uint64_t) 1) << 31)
	type = rust_type ("i64");

      rustyylval.typed_val_int.val = value;
      rustyylval.typed_val_int.type = type;
    }
  else
    {
      rustyylval.typed_val_float.dval = strtod (number, NULL);
      rustyylval.typed_val_float.type = type;
    }

  do_cleanups (cleanup);
  return is_integer ? (could_be_decimal ? DECIMAL_INTEGER : INTEGER) : FLOAT;
}

/* The lexer.  */

static int
rustyylex (void)
{
  /* Skip all leading whitespace.  */
  while (lexptr[0] == ' ' || lexptr[0] == '\t' || lexptr[0] == '\r'
	 || lexptr[0] == '\n')
    ++lexptr;

  /* If we hit EOF and we're completing, then return COMPLETE -- maybe
     we're completing an empty string at the end of a field_expr.
     But, we don't want to return two COMPLETE tokens in a row.  */
  if (lexptr[0] == '\0' && lexptr == prev_lexptr)
    return 0;
  prev_lexptr = lexptr;
  if (lexptr[0] == '\0')
    {
      if (parse_completion)
	{
	  rustyylval.sval = make_stoken ("");
	  return COMPLETE;
	}
      return 0;
    }

  if (lexptr[0] >= '0' && lexptr[0] <= '9')
    return lex_number ();
  else if (lexptr[0] == 'b' && lexptr[1] == '\'')
    return lex_character ();
  else if (lexptr[0] == 'b' && lexptr[1] == '"')
    return lex_string ();
  else if (lexptr[0] == 'b' && starts_raw_string (lexptr + 1))
    return lex_string ();
  else if (starts_raw_string (lexptr))
    return lex_string ();
  else if (rust_identifier_start_p (lexptr[0]))
    return lex_identifier ();
  else if (lexptr[0] == '"')
    return lex_string ();
  else if (lexptr[0] == '\'')
    return lex_character ();
  else if (lexptr[0] == '}' || lexptr[0] == ']')
    {
      /* Falls through to lex_operator.  */
      --paren_depth;
    }
  else if (lexptr[0] == '(' || lexptr[0] == '{')
    {
      /* Falls through to lex_operator.  */
      ++paren_depth;
    }
  else if (lexptr[0] == ',' && comma_terminates && paren_depth == 0)
    return 0;

  return lex_operator ();
}

/* Push back a single character to be re-lexed.  */

static void
rust_push_back (char c)
{
  /* Can't be called before any lexing.  */
  gdb_assert (prev_lexptr != NULL);

  --lexptr;
  gdb_assert (*lexptr == c);
}



/* Make an arbitrary operation and fill in the fields.  */

static const struct rust_op *
ast_operation (enum exp_opcode opcode, const struct rust_op *left,
		const struct rust_op *right)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = opcode;
  result->left.op = left;
  result->right.op = right;

  return result;
}

/* Make a compound assignment operation.  */

static const struct rust_op *
ast_compound_assignment (enum exp_opcode opcode, const struct rust_op *left,
			  const struct rust_op *right)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = opcode;
  result->compound_assignment = 1;
  result->left.op = left;
  result->right.op = right;

  return result;
}

/* Make a typed integer literal operation.  */

static const struct rust_op *
ast_literal (struct typed_val_int val)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_LONG;
  result->left.typed_val_int = val;

  return result;
}

/* Make a typed floating point literal operation.  */

static const struct rust_op *
ast_dliteral (struct typed_val_float val)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_DOUBLE;
  result->left.typed_val_float = val;

  return result;
}

/* Make a unary operation.  */

static const struct rust_op *
ast_unary (enum exp_opcode opcode, const struct rust_op *expr)
{
  return ast_operation (opcode, expr, NULL);
}

/* Make a cast operation.  */

static const struct rust_op *
ast_cast (const struct rust_op *expr, const struct rust_op *type)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = UNOP_CAST;
  result->left.op = expr;
  result->right.op = type;

  return result;
}

/* Make a call-like operation.  This is nominally a function call, but
   when lowering we may discover that it actually represents the
   creation of a tuple struct.  */

static const struct rust_op *
ast_call_ish (enum exp_opcode opcode, const struct rust_op *expr,
	       VEC (rust_op_ptr) **params)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = opcode;
  result->left.op = expr;
  result->right.params = params;

  return result;
}

/* Make a structure creation operation.  */

static const struct rust_op *
ast_struct (const struct rust_op *name, VEC (set_field) **fields)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_AGGREGATE;
  result->left.op = name;
  result->right.field_inits = fields;

  return result;
}

/* Make an identifier path.  */

static const struct rust_op *
ast_path (struct stoken path, VEC (rust_op_ptr) **params)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_VAR_VALUE;
  result->left.sval = path;
  result->right.params = params;

  return result;
}

/* Make a string constant operation.  */

static const struct rust_op *
ast_string (struct stoken str)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_STRING;
  result->left.sval = str;

  return result;
}

/* Make a field expression.  */

static const struct rust_op *
ast_structop (const struct rust_op *left, const char *name, int completing)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = STRUCTOP_STRUCT;
  result->completing = completing;
  result->left.op = left;
  result->right.sval = make_stoken (name);

  return result;
}

/* Make an anonymous struct operation, like 'x.0'.  */

static const struct rust_op *
ast_structop_anonymous (const struct rust_op *left,
			 struct typed_val_int number)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = STRUCTOP_ANONYMOUS;
  result->left.op = left;
  result->right.typed_val_int = number;

  return result;
}

/* Make a range operation.  */

static const struct rust_op *
ast_range (const struct rust_op *lhs, const struct rust_op *rhs)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_RANGE;
  result->left.op = lhs;
  result->right.op = rhs;

  return result;
}

/* A helper function to make a type-related AST node.  */

static struct rust_op *
ast_basic_type (enum type_code typecode)
{
  struct rust_op *result = OBSTACK_ZALLOC (&work_obstack, struct rust_op);

  result->opcode = OP_TYPE;
  result->typecode = typecode;
  return result;
}

/* Create an AST node describing an array type.  */

static const struct rust_op *
ast_array_type (const struct rust_op *lhs, struct typed_val_int val)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_ARRAY);

  result->left.op = lhs;
  result->right.typed_val_int = val;
  return result;
}

/* Create an AST node describing a reference type.  */

static const struct rust_op *
ast_slice_type (const struct rust_op *type)
{
  /* Use TYPE_CODE_COMPLEX just because it is handy.  */
  struct rust_op *result = ast_basic_type (TYPE_CODE_COMPLEX);

  result->left.op = type;
  return result;
}

/* Create an AST node describing a reference type.  */

static const struct rust_op *
ast_reference_type (const struct rust_op *type)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_REF);

  result->left.op = type;
  return result;
}

/* Create an AST node describing a pointer type.  */

static const struct rust_op *
ast_pointer_type (const struct rust_op *type, int is_mut)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_PTR);

  result->left.op = type;
  /* For the time being we ignore is_mut.  */
  return result;
}

/* Create an AST node describing a function type.  */

static const struct rust_op *
ast_function_type (const struct rust_op *rtype, VEC (rust_op_ptr) **params)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_FUNC);

  result->left.op = rtype;
  result->right.params = params;
  return result;
}

/* Create an AST node describing a tuple type.  */

static const struct rust_op *
ast_tuple_type (VEC (rust_op_ptr) **params)
{
  struct rust_op *result = ast_basic_type (TYPE_CODE_STRUCT);

  result->left.params = params;
  return result;
}

/* A helper to appropriately munge NAME and BLOCK depending on the
   presence of a leading "::".  */

static void
munge_name_and_block (const char **name, const struct block **block)
{
  /* If it is a global reference, skip the current block in favor of
     the static block.  */
  if (strncmp (*name, "::", 2) == 0)
    {
      *name += 2;
      *block = block_static_block (*block);
    }
}

/* Like lookup_symbol, but handles Rust namespace conventions, and
   doesn't require field_of_this_result.  */

static struct block_symbol
rust_lookup_symbol (const char *name, const struct block *block,
		    const domain_enum domain)
{
  struct block_symbol result;

  munge_name_and_block (&name, &block);

  result = lookup_symbol (name, block, domain, NULL);
  if (result.symbol != NULL)
    update_innermost_block (result);
  return result;
}

/* Look up a type, following Rust namespace conventions.  */

static struct type *
rust_lookup_type (const char *name, const struct block *block)
{
  struct block_symbol result;
  struct type *type;

  munge_name_and_block (&name, &block);

  result = lookup_symbol (name, block, STRUCT_DOMAIN, NULL);
  if (result.symbol != NULL)
    {
      update_innermost_block (result);
      return SYMBOL_TYPE (result.symbol);
    }

  type = lookup_typename (parse_language (pstate), parse_gdbarch (pstate),
			  name, NULL, 1);
  if (type != NULL)
    return type;

  /* Last chance, try a built-in type.  */
  return language_lookup_primitive_type (parse_language (pstate),
					 parse_gdbarch (pstate),
					 name);
}

static struct type *convert_ast_to_type (struct parser_state *state,
					 const struct rust_op *operation);
static const char *convert_name (struct parser_state *state,
				 const struct rust_op *operation);

/* Convert a vector of rust_ops representing types to a vector of
   types.  */

static VEC (type_ptr) *
convert_params_to_types (struct parser_state *state, VEC (rust_op_ptr) *params)
{
  int i;
  const struct rust_op *op;
  VEC (type_ptr) *result = NULL;
  struct cleanup *cleanup = make_cleanup (VEC_cleanup (type_ptr), &result);

  for (i = 0; VEC_iterate (rust_op_ptr, params, i, op); ++i)
    VEC_safe_push (type_ptr, result, convert_ast_to_type (state, op));

  discard_cleanups (cleanup);
  return result;
}

/* Convert a rust_op representing a type to a struct type *.  */

static struct type *
convert_ast_to_type (struct parser_state *state,
		     const struct rust_op *operation)
{
  struct type *type, *result = NULL;

  if (operation->opcode == OP_VAR_VALUE)
    {
      const char *varname = convert_name (state, operation);

      result = rust_lookup_type (varname, expression_context_block);
      if (result == NULL)
	error (_("No typed name '%s' in current context"), varname);
      return result;
    }

  gdb_assert (operation->opcode == OP_TYPE);

  switch (operation->typecode)
    {
    case TYPE_CODE_ARRAY:
      type = convert_ast_to_type (state, operation->left.op);
      if (operation->right.typed_val_int.val < 0)
	error (_("Negative array length"));
      result = lookup_array_range_type (type, 0,
					operation->right.typed_val_int.val - 1);
      break;

    case TYPE_CODE_COMPLEX:
      {
	struct type *usize = rust_type ("usize");

	type = convert_ast_to_type (state, operation->left.op);
	result = rust_slice_type ("&[*gdb*]", type, usize);
      }
      break;

    case TYPE_CODE_REF:
    case TYPE_CODE_PTR:
      /* For now we treat &x and *x identically.  */
      type = convert_ast_to_type (state, operation->left.op);
      result = lookup_pointer_type (type);
      break;

    case TYPE_CODE_FUNC:
      {
	VEC (type_ptr) *args
	  = convert_params_to_types (state, *operation->right.params);
	struct cleanup *cleanup
	  = make_cleanup (VEC_cleanup (type_ptr), &args);
	struct type **argtypes = NULL;

	type = convert_ast_to_type (state, operation->left.op);
	if (!VEC_empty (type_ptr, args))
	  argtypes = VEC_address (type_ptr, args);

	result
	  = lookup_function_type_with_arguments (type,
						 VEC_length (type_ptr, args),
						 argtypes);
	result = lookup_pointer_type (result);

	do_cleanups (cleanup);
      }
      break;

    case TYPE_CODE_STRUCT:
      {
	VEC (type_ptr) *args
	  = convert_params_to_types (state, *operation->left.params);
	struct cleanup *cleanup
	  = make_cleanup (VEC_cleanup (type_ptr), &args);
	int i;
	struct type *type;
	const char *name;

	obstack_1grow (&work_obstack, '(');
	for (i = 0; VEC_iterate (type_ptr, args, i, type); ++i)
	  {
	    char *type_name = type_to_string (type);

	    if (i > 0)
	      obstack_1grow (&work_obstack, ',');
	    obstack_grow_str (&work_obstack, type_name);

	    xfree (type_name);
	  }

	obstack_grow_str0 (&work_obstack, ")");
	name = (const char *) obstack_finish (&work_obstack);

	/* We don't allow creating new tuple types (yet), but we do
	   allow looking up existing tuple types.  */
	result = rust_lookup_type (name, expression_context_block);
	if (result == NULL)
	  error (_("could not find tuple type '%s'"), name);

	do_cleanups (cleanup);
      }
      break;

    default:
      gdb_assert_not_reached ("unhandled opcode in convert_ast_to_type");
    }

  gdb_assert (result != NULL);
  return result;
}

/* A helper function to turn a rust_op representing a name into a full
   name.  This applies generic arguments as needed.  The returned name
   is allocated on the work obstack.  */

static const char *
convert_name (struct parser_state *state, const struct rust_op *operation)
{
  VEC (type_ptr) *types;
  struct cleanup *cleanup;
  int i;
  struct type *type;

  gdb_assert (operation->opcode == OP_VAR_VALUE);

  if (operation->right.params == NULL)
    return operation->left.sval.ptr;

  types = convert_params_to_types (state, *operation->right.params);
  cleanup = make_cleanup (VEC_cleanup (type_ptr), &types);

  obstack_grow_str (&work_obstack, operation->left.sval.ptr);
  obstack_1grow (&work_obstack, '<');
  for (i = 0; VEC_iterate (type_ptr, types, i, type); ++i)
    {
      char *type_name = type_to_string (type);

      if (i > 0)
	obstack_1grow (&work_obstack, ',');

      obstack_grow_str (&work_obstack, type_name);
      xfree (type_name);
    }
  obstack_grow_str0 (&work_obstack, ">");

  do_cleanups (cleanup);

  return (const char *) obstack_finish (&work_obstack);
}

static void convert_ast_to_expression (struct parser_state *state,
				       const struct rust_op *operation,
				       const struct rust_op *top);

/* A helper function that converts a vec of rust_ops to a gdb
   expression.  */

static void
convert_params_to_expression (struct parser_state *state,
			      VEC (rust_op_ptr) *params,
			      const struct rust_op *top)
{
  int i;
  rust_op_ptr elem;

  for (i = 0; VEC_iterate (rust_op_ptr, params, i, elem); ++i)
    convert_ast_to_expression (state, elem, top);
}

/* Lower a rust_op to a gdb expression.  STATE is the parser state.
   OPERATION is the operation to lower.  TOP is a pointer to the
   top-most operation; it is used to handle the special case where the
   top-most expression is an identifier and can be optionally lowered
   to OP_TYPE.  */

static void
convert_ast_to_expression (struct parser_state *state,
			   const struct rust_op *operation,
			   const struct rust_op *top)
{
  switch (operation->opcode)
    {
    case OP_LONG:
      write_exp_elt_opcode (state, OP_LONG);
      write_exp_elt_type (state, operation->left.typed_val_int.type);
      write_exp_elt_longcst (state, operation->left.typed_val_int.val);
      write_exp_elt_opcode (state, OP_LONG);
      break;

    case OP_DOUBLE:
      write_exp_elt_opcode (state, OP_DOUBLE);
      write_exp_elt_type (state, operation->left.typed_val_float.type);
      write_exp_elt_dblcst (state, operation->left.typed_val_float.dval);
      write_exp_elt_opcode (state, OP_DOUBLE);
      break;

    case STRUCTOP_STRUCT:
      {
	convert_ast_to_expression (state, operation->left.op, top);

	if (operation->completing)
	  mark_struct_expression (state);
	write_exp_elt_opcode (state, STRUCTOP_STRUCT);
	write_exp_string (state, operation->right.sval);
	write_exp_elt_opcode (state, STRUCTOP_STRUCT);
      }
      break;

    case STRUCTOP_ANONYMOUS:
      {
	convert_ast_to_expression (state, operation->left.op, top);

	write_exp_elt_opcode (state, STRUCTOP_ANONYMOUS);
	write_exp_elt_longcst (state, operation->right.typed_val_int.val);
	write_exp_elt_opcode (state, STRUCTOP_ANONYMOUS);
      }
      break;

    case UNOP_PLUS:
    case UNOP_NEG:
    case UNOP_COMPLEMENT:
    case UNOP_IND:
    case UNOP_ADDR:
      convert_ast_to_expression (state, operation->left.op, top);
      write_exp_elt_opcode (state, operation->opcode);
      break;

    case BINOP_SUBSCRIPT:
    case BINOP_MUL:
    case BINOP_REPEAT:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_LOGICAL_OR:
    case BINOP_LOGICAL_AND:
    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LEQ:
    case BINOP_GEQ:
    case BINOP_LSH:
    case BINOP_RSH:
    case BINOP_ASSIGN:
    case OP_RUST_ARRAY:
      convert_ast_to_expression (state, operation->left.op, top);
      convert_ast_to_expression (state, operation->right.op, top);
      if (operation->compound_assignment)
	{
	  write_exp_elt_opcode (state, BINOP_ASSIGN_MODIFY);
	  write_exp_elt_opcode (state, operation->opcode);
	  write_exp_elt_opcode (state, BINOP_ASSIGN_MODIFY);
	}
      else
	write_exp_elt_opcode (state, operation->opcode);

      if (operation->compound_assignment
	  || operation->opcode == BINOP_ASSIGN)
	{
	  struct type *type;

	  type = language_lookup_primitive_type (parse_language (state),
						 parse_gdbarch (state),
						 "()");

	  write_exp_elt_opcode (state, OP_LONG);
	  write_exp_elt_type (state, type);
	  write_exp_elt_longcst (state, 0);
	  write_exp_elt_opcode (state, OP_LONG);

	  write_exp_elt_opcode (state, BINOP_COMMA);
	}
      break;

    case UNOP_CAST:
      {
	struct type *type = convert_ast_to_type (state, operation->right.op);

	convert_ast_to_expression (state, operation->left.op, top);
	write_exp_elt_opcode (state, UNOP_CAST);
	write_exp_elt_type (state, type);
	write_exp_elt_opcode (state, UNOP_CAST);
      }
      break;

    case OP_FUNCALL:
      {
	if (operation->left.op->opcode == OP_VAR_VALUE)
	  {
	    struct type *type;
	    const char *varname = convert_name (state, operation->left.op);

	    type = rust_lookup_type (varname, expression_context_block);
	    if (type != NULL)
	      {
		/* This is actually a tuple struct expression, not a
		   call expression.  */
		rust_op_ptr elem;
		int i;
		VEC (rust_op_ptr) *params = *operation->right.params;

		if (TYPE_CODE (type) != TYPE_CODE_NAMESPACE)
		  {
		    if (!rust_tuple_struct_type_p (type))
		      error (_("Type %s is not a tuple struct"), varname);

		    for (i = 0;
			 VEC_iterate (rust_op_ptr, params, i, elem);
			 ++i)
		      {
			char *cell = get_print_cell ();

			xsnprintf (cell, PRINT_CELL_SIZE, "__%d", i);
			write_exp_elt_opcode (state, OP_NAME);
			write_exp_string (state, make_stoken (cell));
			write_exp_elt_opcode (state, OP_NAME);

			convert_ast_to_expression (state, elem, top);
		      }

		    write_exp_elt_opcode (state, OP_AGGREGATE);
		    write_exp_elt_type (state, type);
		    write_exp_elt_longcst (state,
					   2 * VEC_length (rust_op_ptr,
							   params));
		    write_exp_elt_opcode (state, OP_AGGREGATE);
		    break;
		  }
	      }
	  }
	convert_ast_to_expression (state, operation->left.op, top);
	convert_params_to_expression (state, *operation->right.params, top);
	write_exp_elt_opcode (state, OP_FUNCALL);
	write_exp_elt_longcst (state, VEC_length (rust_op_ptr,
						  *operation->right.params));
	write_exp_elt_longcst (state, OP_FUNCALL);
      }
      break;

    case OP_ARRAY:
      gdb_assert (operation->left.op == NULL);
      convert_params_to_expression (state, *operation->right.params, top);
      write_exp_elt_opcode (state, OP_ARRAY);
      write_exp_elt_longcst (state, 0);
      write_exp_elt_longcst (state, VEC_length (rust_op_ptr,
						*operation->right.params) - 1);
      write_exp_elt_longcst (state, OP_ARRAY);
      break;

    case OP_VAR_VALUE:
      {
	struct block_symbol sym;
	const char *varname;

	if (operation->left.sval.ptr[0] == '$')
	  {
	    write_dollar_variable (state, operation->left.sval);
	    break;
	  }

	varname = convert_name (state, operation);
	sym = rust_lookup_symbol (varname, expression_context_block,
				  VAR_DOMAIN);
	if (sym.symbol != NULL)
	  {
	    write_exp_elt_opcode (state, OP_VAR_VALUE);
	    write_exp_elt_block (state, sym.block);
	    write_exp_elt_sym (state, sym.symbol);
	    write_exp_elt_opcode (state, OP_VAR_VALUE);
	  }
	else
	  {
	    struct type *type;

	    type = rust_lookup_type (varname, expression_context_block);
	    if (type == NULL)
	      error (_("No symbol '%s' in current context"), varname);

	    if (TYPE_CODE (type) == TYPE_CODE_STRUCT
		&& TYPE_NFIELDS (type) == 0)
	      {
		/* A unit-like struct.  */
		write_exp_elt_opcode (state, OP_AGGREGATE);
		write_exp_elt_type (state, type);
		write_exp_elt_longcst (state, 0);
		write_exp_elt_opcode (state, OP_AGGREGATE);
	      }
	    else if (operation == top)
	      {
		write_exp_elt_opcode (state, OP_TYPE);
		write_exp_elt_type (state, type);
		write_exp_elt_opcode (state, OP_TYPE);
		break;
	      }
	  }
      }
      break;

    case OP_AGGREGATE:
      {
	int i;
	int length;
	struct set_field *init;
	VEC (set_field) *fields = *operation->right.field_inits;
	struct type *type;
	const char *name;

	length = 0;
	for (i = 0; VEC_iterate (set_field, fields, i, init); ++i)
	  {
	    if (init->name.ptr != NULL)
	      {
		write_exp_elt_opcode (state, OP_NAME);
		write_exp_string (state, init->name);
		write_exp_elt_opcode (state, OP_NAME);
		++length;
	      }

	    convert_ast_to_expression (state, init->init, top);
	    ++length;

	    if (init->name.ptr == NULL)
	      {
		/* This is handled differently from Ada in our
		   evaluator.  */
		write_exp_elt_opcode (state, OP_OTHERS);
	      }
	  }

	name = convert_name (state, operation->left.op);
	type = rust_lookup_type (name, expression_context_block);
	if (type == NULL)
	  error (_("Could not find type '%s'"), operation->left.sval.ptr);

	if (TYPE_CODE (type) != TYPE_CODE_STRUCT
	    || rust_tuple_type_p (type)
	    || rust_tuple_struct_type_p (type))
	  error (_("Struct expression applied to non-struct type"));

	write_exp_elt_opcode (state, OP_AGGREGATE);
	write_exp_elt_type (state, type);
	write_exp_elt_longcst (state, length);
	write_exp_elt_opcode (state, OP_AGGREGATE);
      }
      break;

    case OP_STRING:
      {
	write_exp_elt_opcode (state, OP_STRING);
	write_exp_string (state, operation->left.sval);
	write_exp_elt_opcode (state, OP_STRING);
      }
      break;

    case OP_RANGE:
      {
	enum range_type kind = BOTH_BOUND_DEFAULT;

	if (operation->left.op != NULL)
	  {
	    convert_ast_to_expression (state, operation->left.op, top);
	    kind = HIGH_BOUND_DEFAULT;
	  }
	if (operation->right.op != NULL)
	  {
	    convert_ast_to_expression (state, operation->right.op, top);
	    if (kind == BOTH_BOUND_DEFAULT)
	      kind = LOW_BOUND_DEFAULT;
	    else
	      {
		gdb_assert (kind == HIGH_BOUND_DEFAULT);
		kind = NONE_BOUND_DEFAULT;
	      }
	  }
	write_exp_elt_opcode (state, OP_RANGE);
	write_exp_elt_longcst (state, kind);
	write_exp_elt_opcode (state, OP_RANGE);
      }
      break;

    default:
      gdb_assert_not_reached ("unhandled opcode in convert_ast_to_expression");
    }
}



/* The parser as exposed to gdb.  */

int
rust_parse (struct parser_state *state)
{
  int result;
  struct cleanup *cleanup;

  obstack_init (&work_obstack);
  cleanup = make_cleanup_obstack_free (&work_obstack);
  rust_ast = NULL;

  pstate = state;
  result = rustyyparse ();

  if (!result || (parse_completion && rust_ast != NULL))
    {
      const struct rust_op *ast = rust_ast;

      rust_ast = NULL;
      gdb_assert (ast != NULL);
      convert_ast_to_expression (state, ast, ast);
    }

  do_cleanups (cleanup);
  return result;
}

/* The parser error handler.  */

void
rustyyerror (char *msg)
{
  const char *where = prev_lexptr ? prev_lexptr : lexptr;
  error (_("%s in expression, near `%s'."), (msg ? msg : "Error"), where);
}



#if GDB_SELF_TEST

/* Initialize the lexer for testing.  */

static void
rust_lex_test_init (const char *input)
{
  prev_lexptr = NULL;
  lexptr = input;
  paren_depth = 0;
}

/* A test helper that lexes a string, expecting a single token.  It
   returns the lexer data for this token.  */

static RUSTSTYPE
rust_lex_test_one (const char *input, int expected)
{
  int token;
  RUSTSTYPE result;

  rust_lex_test_init (input);

  token = rustyylex ();
  SELF_CHECK (token == expected);
  result = rustyylval;

  if (token)
    {
      token = rustyylex ();
      SELF_CHECK (token == 0);
    }

  return result;
}

/* Test that INPUT lexes as the integer VALUE.  */

static void
rust_lex_int_test (const char *input, int value, int kind)
{
  RUSTSTYPE result = rust_lex_test_one (input, kind);
  SELF_CHECK (result.typed_val_int.val == value);
}

/* Test that INPUT throws an exception with text ERR.  */

static void
rust_lex_exception_test (const char *input, const char *err)
{
  TRY
    {
      /* The "kind" doesn't matter.  */
      rust_lex_test_one (input, DECIMAL_INTEGER);
      SELF_CHECK (0);
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      SELF_CHECK (strcmp (except.message, err) == 0);
    }
  END_CATCH
}

/* Test that INPUT lexes as the identifier, string, or byte-string
   VALUE.  KIND holds the expected token kind.  */

static void
rust_lex_stringish_test (const char *input, const char *value, int kind)
{
  RUSTSTYPE result = rust_lex_test_one (input, kind);
  SELF_CHECK (result.sval.length == strlen (value));
  SELF_CHECK (strncmp (result.sval.ptr, value, result.sval.length) == 0);
}

/* Helper to test that a string parses as a given token sequence.  */

static void
rust_lex_test_sequence (const char *input, int len, const int expected[])
{
  int i;

  lexptr = input;
  paren_depth = 0;

  for (i = 0; i < len; ++i)
    {
      int token = rustyylex ();

      SELF_CHECK (token == expected[i]);
    }
}

/* Tests for an integer-parsing corner case.  */

static void
rust_lex_test_trailing_dot (void)
{
  const int expected1[] = { DECIMAL_INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected2[] = { INTEGER, '.', IDENT, '(', ')', 0 };
  const int expected3[] = { FLOAT, EQEQ, '(', ')', 0 };
  const int expected4[] = { DECIMAL_INTEGER, DOTDOT, DECIMAL_INTEGER, 0 };

  rust_lex_test_sequence ("23.g()", ARRAY_SIZE (expected1), expected1);
  rust_lex_test_sequence ("23_0.g()", ARRAY_SIZE (expected2), expected2);
  rust_lex_test_sequence ("23.==()", ARRAY_SIZE (expected3), expected3);
  rust_lex_test_sequence ("23..25", ARRAY_SIZE (expected4), expected4);
}

/* Tests of completion.  */

static void
rust_lex_test_completion (void)
{
  const int expected[] = { IDENT, '.', COMPLETE, 0 };

  parse_completion = 1;

  rust_lex_test_sequence ("something.wha", ARRAY_SIZE (expected), expected);
  rust_lex_test_sequence ("something.", ARRAY_SIZE (expected), expected);

  parse_completion = 0;
}

/* Test pushback.  */

static void
rust_lex_test_push_back (void)
{
  int token;

  rust_lex_test_init (">>=");

  token = rustyylex ();
  SELF_CHECK (token == COMPOUND_ASSIGN);
  SELF_CHECK (rustyylval.opcode == BINOP_RSH);

  rust_push_back ('=');

  token = rustyylex ();
  SELF_CHECK (token == '=');

  token = rustyylex ();
  SELF_CHECK (token == 0);
}

/* Unit test the lexer.  */

static void
rust_lex_tests (void)
{
  int i;

  obstack_init (&work_obstack);
  unit_testing = 1;

  rust_lex_test_one ("", 0);
  rust_lex_test_one ("    \t  \n \r  ", 0);
  rust_lex_test_one ("thread 23", 0);
  rust_lex_test_one ("task 23", 0);
  rust_lex_test_one ("th 104", 0);
  rust_lex_test_one ("ta 97", 0);

  rust_lex_int_test ("'z'", 'z', INTEGER);
  rust_lex_int_test ("'\\xff'", 0xff, INTEGER);
  rust_lex_int_test ("'\\u{1016f}'", 0x1016f, INTEGER);
  rust_lex_int_test ("b'z'", 'z', INTEGER);
  rust_lex_int_test ("b'\\xfe'", 0xfe, INTEGER);
  rust_lex_int_test ("b'\\xFE'", 0xfe, INTEGER);
  rust_lex_int_test ("b'\\xfE'", 0xfe, INTEGER);

  /* Test all escapes in both modes.  */
  rust_lex_int_test ("'\\n'", '\n', INTEGER);
  rust_lex_int_test ("'\\r'", '\r', INTEGER);
  rust_lex_int_test ("'\\t'", '\t', INTEGER);
  rust_lex_int_test ("'\\\\'", '\\', INTEGER);
  rust_lex_int_test ("'\\0'", '\0', INTEGER);
  rust_lex_int_test ("'\\''", '\'', INTEGER);
  rust_lex_int_test ("'\\\"'", '"', INTEGER);

  rust_lex_int_test ("b'\\n'", '\n', INTEGER);
  rust_lex_int_test ("b'\\r'", '\r', INTEGER);
  rust_lex_int_test ("b'\\t'", '\t', INTEGER);
  rust_lex_int_test ("b'\\\\'", '\\', INTEGER);
  rust_lex_int_test ("b'\\0'", '\0', INTEGER);
  rust_lex_int_test ("b'\\''", '\'', INTEGER);
  rust_lex_int_test ("b'\\\"'", '"', INTEGER);

  rust_lex_exception_test ("'z", "Unterminated character literal");
  rust_lex_exception_test ("b'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test ("b'\\u{0}'", "Unicode escape in byte literal");
  rust_lex_exception_test ("'\\x0'", "Not enough hex digits seen");
  rust_lex_exception_test ("'\\u0'", "Missing '{' in Unicode escape");
  rust_lex_exception_test ("'\\u{0", "Missing '}' in Unicode escape");
  rust_lex_exception_test ("'\\u{0000007}", "Overlong hex escape");
  rust_lex_exception_test ("'\\u{}", "Not enough hex digits seen");
  rust_lex_exception_test ("'\\Q'", "Invalid escape \\Q in literal");
  rust_lex_exception_test ("b'\\Q'", "Invalid escape \\Q in literal");

  rust_lex_int_test ("23", 23, DECIMAL_INTEGER);
  rust_lex_int_test ("2_344__29", 234429, INTEGER);
  rust_lex_int_test ("0x1f", 0x1f, INTEGER);
  rust_lex_int_test ("23usize", 23, INTEGER);
  rust_lex_int_test ("23i32", 23, INTEGER);
  rust_lex_int_test ("0x1_f", 0x1f, INTEGER);
  rust_lex_int_test ("0b1_101011__", 0x6b, INTEGER);
  rust_lex_int_test ("0o001177i64", 639, INTEGER);

  rust_lex_test_trailing_dot ();

  rust_lex_test_one ("23.", FLOAT);
  rust_lex_test_one ("23.99f32", FLOAT);
  rust_lex_test_one ("23e7", FLOAT);
  rust_lex_test_one ("23E-7", FLOAT);
  rust_lex_test_one ("23e+7", FLOAT);
  rust_lex_test_one ("23.99e+7f64", FLOAT);
  rust_lex_test_one ("23.82f32", FLOAT);

  rust_lex_stringish_test ("hibob", "hibob", IDENT);
  rust_lex_stringish_test ("hibob__93", "hibob__93", IDENT);
  rust_lex_stringish_test ("thread", "thread", IDENT);

  rust_lex_stringish_test ("\"string\"", "string", STRING);
  rust_lex_stringish_test ("\"str\\ting\"", "str\ting", STRING);
  rust_lex_stringish_test ("\"str\\\"ing\"", "str\"ing", STRING);
  rust_lex_stringish_test ("r\"str\\ing\"", "str\\ing", STRING);
  rust_lex_stringish_test ("r#\"str\\ting\"#", "str\\ting", STRING);
  rust_lex_stringish_test ("r###\"str\\\"ing\"###", "str\\\"ing", STRING);

  rust_lex_stringish_test ("b\"string\"", "string", BYTESTRING);
  rust_lex_stringish_test ("b\"\x73tring\"", "string", BYTESTRING);
  rust_lex_stringish_test ("b\"str\\\"ing\"", "str\"ing", BYTESTRING);
  rust_lex_stringish_test ("br####\"\\x73tring\"####", "\\x73tring",
			   BYTESTRING);

  for (i = 0; i < ARRAY_SIZE (identifier_tokens); ++i)
    rust_lex_test_one (identifier_tokens[i].name, identifier_tokens[i].value);

  for (i = 0; i < ARRAY_SIZE (operator_tokens); ++i)
    rust_lex_test_one (operator_tokens[i].name, operator_tokens[i].value);

  rust_lex_test_completion ();
  rust_lex_test_push_back ();

  obstack_free (&work_obstack, NULL);
  unit_testing = 0;
}

#endif /* GDB_SELF_TEST */

void
_initialize_rust_exp (void)
{
  int code = regcomp (&number_regex, number_regex_text, REG_EXTENDED);
  /* If the regular expression was incorrect, it was a programming
     error.  */
  gdb_assert (code == 0);

#if GDB_SELF_TEST
  register_self_test (rust_lex_tests);
#endif
}
