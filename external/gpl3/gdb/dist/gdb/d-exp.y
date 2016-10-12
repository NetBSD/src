/* YACC parser for D expressions, for GDB.

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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

/* This file is derived from c-exp.y, jv-exp.y.  */

/* Parse a D expression from text in a string,
   and return the result as a struct expression pointer.
   That structure contains arithmetic operations in reverse polish,
   with constants represented by operations that are followed by special data.
   See expression.h for the details of the format.
   What is important here is that it can be built up sequentially
   during the process of parsing; the lower levels of the tree always
   come first in the result.

   Note that malloc's and realloc's in this file are transformed to
   xmalloc and xrealloc respectively by the same sed command in the
   makefile that remaps any other malloc/realloc inserted by the parser
   generator.  Doing this with #defines and trying to control the interaction
   with include files (<malloc.h> and <stdlib.h> for example) just became
   too messy, particularly when such includes can be inserted at random
   times by the parser generator.  */

%{

#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "d-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))
#define parse_d_type(ps) builtin_d_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX d_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (char *);

static int type_aggregate_p (struct type *);

%}

/* Although the yacc "value" of an expression is not used,
   since the result is stored in the structure being created,
   other node types do have values.  */

%union
  {
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct typed_stoken tsval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int ival;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct stoken_vector svec;
  }

%{
/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *,
			 int, int, YYSTYPE *);
%}

%token <sval> IDENTIFIER UNKNOWN_NAME
%token <tsym> TYPENAME
%token <voidval> COMPLETE

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */

%token <sval> NAME_OR_INT

%token <typed_val_int> INTEGER_LITERAL
%token <typed_val_float> FLOAT_LITERAL
%token <tsval> CHARACTER_LITERAL
%token <tsval> STRING_LITERAL

%type <svec> StringExp
%type <tval> BasicType TypeExp
%type <sval> IdentifierExp
%type <ival> ArrayLiteral

%token ENTRY
%token ERROR

/* Keywords that have a constant value.  */
%token TRUE_KEYWORD FALSE_KEYWORD NULL_KEYWORD
/* Class 'super' accessor.  */
%token SUPER_KEYWORD
/* Properties.  */
%token CAST_KEYWORD SIZEOF_KEYWORD
%token TYPEOF_KEYWORD TYPEID_KEYWORD
%token INIT_KEYWORD
/* Comparison keywords.  */
/* Type storage classes.  */
%token IMMUTABLE_KEYWORD CONST_KEYWORD SHARED_KEYWORD
/* Non-scalar type keywords.  */
%token STRUCT_KEYWORD UNION_KEYWORD
%token CLASS_KEYWORD INTERFACE_KEYWORD
%token ENUM_KEYWORD TEMPLATE_KEYWORD
%token DELEGATE_KEYWORD FUNCTION_KEYWORD

%token <sval> DOLLAR_VARIABLE

%token <opcode> ASSIGN_MODIFY

%left ','
%right '=' ASSIGN_MODIFY
%right '?'
%left OROR
%left ANDAND
%left '|'
%left '^'
%left '&'
%left EQUAL NOTEQUAL '<' '>' LEQ GEQ
%right LSH RSH
%left '+' '-'
%left '*' '/' '%'
%right HATHAT
%left IDENTITY NOTIDENTITY
%right INCREMENT DECREMENT
%right '.' '[' '('
%token DOTDOT


%%

start   :
	Expression
|	TypeExp
;

/* Expressions, including the comma operator.  */

Expression:
	CommaExpression
;

CommaExpression:
	AssignExpression
|	AssignExpression ',' CommaExpression
		{ write_exp_elt_opcode (pstate, BINOP_COMMA); }
;

AssignExpression:
	ConditionalExpression
|	ConditionalExpression '=' AssignExpression
		{ write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
|	ConditionalExpression ASSIGN_MODIFY AssignExpression
		{ write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
		  write_exp_elt_opcode (pstate, $2);
		  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY); }
;

ConditionalExpression:
	OrOrExpression
|	OrOrExpression '?' Expression ':' ConditionalExpression
		{ write_exp_elt_opcode (pstate, TERNOP_COND); }
;

OrOrExpression:
	AndAndExpression
|	OrOrExpression OROR AndAndExpression
		{ write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
;

AndAndExpression:
	OrExpression
|	AndAndExpression ANDAND OrExpression
		{ write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
;

OrExpression:
	XorExpression
|	OrExpression '|' XorExpression
		{ write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
;

XorExpression:
	AndExpression
|	XorExpression '^' AndExpression
		{ write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
;

AndExpression:
	CmpExpression
|	AndExpression '&' CmpExpression
		{ write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
;

CmpExpression:
	ShiftExpression
|	EqualExpression
|	IdentityExpression
|	RelExpression
;

EqualExpression:
	ShiftExpression EQUAL ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_EQUAL); }
|	ShiftExpression NOTEQUAL ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
;

IdentityExpression:
	ShiftExpression IDENTITY ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_EQUAL); }
|	ShiftExpression NOTIDENTITY ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
;

RelExpression:
	ShiftExpression '<' ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_LESS); }
|	ShiftExpression LEQ ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_LEQ); }
|	ShiftExpression '>' ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_GTR); }
|	ShiftExpression GEQ ShiftExpression
		{ write_exp_elt_opcode (pstate, BINOP_GEQ); }
;

ShiftExpression:
	AddExpression
|	ShiftExpression LSH AddExpression
		{ write_exp_elt_opcode (pstate, BINOP_LSH); }
|	ShiftExpression RSH AddExpression
		{ write_exp_elt_opcode (pstate, BINOP_RSH); }
;

AddExpression:
	MulExpression
|	AddExpression '+' MulExpression
		{ write_exp_elt_opcode (pstate, BINOP_ADD); }
|	AddExpression '-' MulExpression
		{ write_exp_elt_opcode (pstate, BINOP_SUB); }
|	AddExpression '~' MulExpression
		{ write_exp_elt_opcode (pstate, BINOP_CONCAT); }
;

MulExpression:
	UnaryExpression
|	MulExpression '*' UnaryExpression
		{ write_exp_elt_opcode (pstate, BINOP_MUL); }
|	MulExpression '/' UnaryExpression
		{ write_exp_elt_opcode (pstate, BINOP_DIV); }
|	MulExpression '%' UnaryExpression
		{ write_exp_elt_opcode (pstate, BINOP_REM); }

UnaryExpression:
	'&' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_ADDR); }
|	INCREMENT UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_PREINCREMENT); }
|	DECREMENT UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_PREDECREMENT); }
|	'*' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_IND); }
|	'-' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_NEG); }
|	'+' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_PLUS); }
|	'!' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
|	'~' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
|	TypeExp '.' SIZEOF_KEYWORD
		{ write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
|	CastExpression
|	PowExpression
;

CastExpression:
	CAST_KEYWORD '(' TypeExp ')' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate, $3);
		  write_exp_elt_opcode (pstate, UNOP_CAST); }
	/* C style cast is illegal D, but is still recognised in
	   the grammar, so we keep this around for convenience.  */
|	'(' TypeExp ')' UnaryExpression
		{ write_exp_elt_opcode (pstate, UNOP_CAST);
		  write_exp_elt_type (pstate, $2);
		  write_exp_elt_opcode (pstate, UNOP_CAST); }
;

PowExpression:
	PostfixExpression
|	PostfixExpression HATHAT UnaryExpression
		{ write_exp_elt_opcode (pstate, BINOP_EXP); }
;

PostfixExpression:
	PrimaryExpression
|	PostfixExpression '.' COMPLETE
		{ struct stoken s;
		  mark_struct_expression (pstate);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  s.ptr = "";
		  s.length = 0;
		  write_exp_string (pstate, s);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
|	PostfixExpression '.' IDENTIFIER
		{ write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  write_exp_string (pstate, $3);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
|	PostfixExpression '.' IDENTIFIER COMPLETE
		{ mark_struct_expression (pstate);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
		  write_exp_string (pstate, $3);
		  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
|	PostfixExpression '.' SIZEOF_KEYWORD
		{ write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
|	PostfixExpression INCREMENT
		{ write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
|	PostfixExpression DECREMENT
		{ write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
|	CallExpression
|	IndexExpression
|	SliceExpression
;

ArgumentList:
	AssignExpression
		{ arglist_len = 1; }
|	ArgumentList ',' AssignExpression
		{ arglist_len++; }
;

ArgumentList_opt:
	/* EMPTY */
		{ arglist_len = 0; }
|	ArgumentList
;

CallExpression:
	PostfixExpression '('
		{ start_arglist (); }
	ArgumentList_opt ')'
		{ write_exp_elt_opcode (pstate, OP_FUNCALL);
		  write_exp_elt_longcst (pstate, (LONGEST) end_arglist ());
		  write_exp_elt_opcode (pstate, OP_FUNCALL); }
;

IndexExpression:
	PostfixExpression '[' ArgumentList ']'
		{ if (arglist_len > 0)
		    {
		      write_exp_elt_opcode (pstate, MULTI_SUBSCRIPT);
		      write_exp_elt_longcst (pstate, (LONGEST) arglist_len);
		      write_exp_elt_opcode (pstate, MULTI_SUBSCRIPT);
		    }
		  else
		    write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT);
		}
;

SliceExpression:
	PostfixExpression '[' ']'
		{ /* Do nothing.  */ }
|	PostfixExpression '[' AssignExpression DOTDOT AssignExpression ']'
		{ write_exp_elt_opcode (pstate, TERNOP_SLICE); }
;

PrimaryExpression:
	'(' Expression ')'
		{ /* Do nothing.  */ }
|	IdentifierExp
		{ struct bound_minimal_symbol msymbol;
		  char *copy = copy_name ($1);
		  struct field_of_this_result is_a_field_of_this;
		  struct block_symbol sym;

		  /* Handle VAR, which could be local or global.  */
		  sym = lookup_symbol (copy, expression_context_block, VAR_DOMAIN,
				       &is_a_field_of_this);
		  if (sym.symbol && SYMBOL_CLASS (sym.symbol) != LOC_TYPEDEF)
		    {
		      if (symbol_read_needs_frame (sym.symbol))
			{
			  if (innermost_block == 0
			      || contained_in (sym.block, innermost_block))
			    innermost_block = sym.block;
			}

		      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
		      write_exp_elt_block (pstate, sym.block);
		      write_exp_elt_sym (pstate, sym.symbol);
		      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
		    }
		  else if (is_a_field_of_this.type != NULL)
		     {
		      /* It hangs off of `this'.  Must not inadvertently convert from a
			 method call to data ref.  */
		      if (innermost_block == 0
			  || contained_in (sym.block, innermost_block))
			innermost_block = sym.block;
		      write_exp_elt_opcode (pstate, OP_THIS);
		      write_exp_elt_opcode (pstate, OP_THIS);
		      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
		      write_exp_string (pstate, $1);
		      write_exp_elt_opcode (pstate, STRUCTOP_PTR);
		    }
		  else
		    {
		      /* Lookup foreign name in global static symbols.  */
		      msymbol = lookup_bound_minimal_symbol (copy);
		      if (msymbol.minsym != NULL)
			write_exp_msymbol (pstate, msymbol);
		      else if (!have_full_symbols () && !have_partial_symbols ())
			error (_("No symbol table is loaded.  Use the \"file\" command"));
		      else
			error (_("No symbol \"%s\" in current context."), copy);
		    }
		  }
|	TypeExp '.' IdentifierExp
			{ struct type *type = check_typedef ($1);

			  /* Check if the qualified name is in the global
			     context.  However if the symbol has not already
			     been resolved, it's not likely to be found.  */
			  if (TYPE_CODE (type) == TYPE_CODE_MODULE)
			    {
			      struct bound_minimal_symbol msymbol;
			      struct block_symbol sym;
			      const char *type_name = TYPE_SAFE_NAME (type);
			      int type_name_len = strlen (type_name);
			      char *name;

			      name = xstrprintf ("%.*s.%.*s",
						 type_name_len, type_name,
						 $3.length, $3.ptr);
			      make_cleanup (xfree, name);

			      sym =
				lookup_symbol (name, (const struct block *) NULL,
					       VAR_DOMAIN, NULL);
			      if (sym.symbol)
				{
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				  write_exp_elt_block (pstate, sym.block);
				  write_exp_elt_sym (pstate, sym.symbol);
				  write_exp_elt_opcode (pstate, OP_VAR_VALUE);
				  break;
				}

			      msymbol = lookup_bound_minimal_symbol (name);
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (pstate, msymbol);
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error (_("No symbol table is loaded.  Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."), name);
			    }

			  /* Check if the qualified name resolves as a member
			     of an aggregate or an enum type.  */
			  if (!type_aggregate_p (type))
			    error (_("`%s' is not defined as an aggregate type."),
				   TYPE_SAFE_NAME (type));

			  write_exp_elt_opcode (pstate, OP_SCOPE);
			  write_exp_elt_type (pstate, type);
			  write_exp_string (pstate, $3);
			  write_exp_elt_opcode (pstate, OP_SCOPE);
			}
|	DOLLAR_VARIABLE
		{ write_dollar_variable (pstate, $1); }
|	NAME_OR_INT
		{ YYSTYPE val;
                  parse_number (pstate, $1.ptr, $1.length, 0, &val);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, val.typed_val_int.type);
		  write_exp_elt_longcst (pstate,
					 (LONGEST) val.typed_val_int.val);
		  write_exp_elt_opcode (pstate, OP_LONG); }
|	NULL_KEYWORD
		{ struct type *type = parse_d_type (pstate)->builtin_void;
		  type = lookup_pointer_type (type);
		  write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, type);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_opcode (pstate, OP_LONG); }
|	TRUE_KEYWORD
		{ write_exp_elt_opcode (pstate, OP_BOOL);
		  write_exp_elt_longcst (pstate, (LONGEST) 1);
		  write_exp_elt_opcode (pstate, OP_BOOL); }
|	FALSE_KEYWORD
		{ write_exp_elt_opcode (pstate, OP_BOOL);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_opcode (pstate, OP_BOOL); }
|	INTEGER_LITERAL
		{ write_exp_elt_opcode (pstate, OP_LONG);
		  write_exp_elt_type (pstate, $1.type);
		  write_exp_elt_longcst (pstate, (LONGEST)($1.val));
		  write_exp_elt_opcode (pstate, OP_LONG); }
|	FLOAT_LITERAL
		{ write_exp_elt_opcode (pstate, OP_DOUBLE);
		  write_exp_elt_type (pstate, $1.type);
		  write_exp_elt_dblcst (pstate, $1.dval);
		  write_exp_elt_opcode (pstate, OP_DOUBLE); }
|	CHARACTER_LITERAL
		{ struct stoken_vector vec;
		  vec.len = 1;
		  vec.tokens = &$1;
		  write_exp_string_vector (pstate, $1.type, &vec); }
|	StringExp
		{ int i;
		  write_exp_string_vector (pstate, 0, &$1);
		  for (i = 0; i < $1.len; ++i)
		    free ($1.tokens[i].ptr);
		  free ($1.tokens); }
|	ArrayLiteral
		{ write_exp_elt_opcode (pstate, OP_ARRAY);
		  write_exp_elt_longcst (pstate, (LONGEST) 0);
		  write_exp_elt_longcst (pstate, (LONGEST) $1 - 1);
		  write_exp_elt_opcode (pstate, OP_ARRAY); }
|	TYPEOF_KEYWORD '(' Expression ')'
		{ write_exp_elt_opcode (pstate, OP_TYPEOF); }
;

ArrayLiteral:
	'[' ArgumentList_opt ']'
		{ $$ = arglist_len; }
;

IdentifierExp:
	IDENTIFIER
;

StringExp:
	STRING_LITERAL
		{ /* We copy the string here, and not in the
		     lexer, to guarantee that we do not leak a
		     string.  Note that we follow the
		     NUL-termination convention of the
		     lexer.  */
		  struct typed_stoken *vec = XNEW (struct typed_stoken);
		  $$.len = 1;
		  $$.tokens = vec;

		  vec->type = $1.type;
		  vec->length = $1.length;
		  vec->ptr = (char *) malloc ($1.length + 1);
		  memcpy (vec->ptr, $1.ptr, $1.length + 1);
		}
|	StringExp STRING_LITERAL
		{ /* Note that we NUL-terminate here, but just
		     for convenience.  */
		  char *p;
		  ++$$.len;
		  $$.tokens
		    = XRESIZEVEC (struct typed_stoken, $$.tokens, $$.len);

		  p = (char *) malloc ($2.length + 1);
		  memcpy (p, $2.ptr, $2.length + 1);

		  $$.tokens[$$.len - 1].type = $2.type;
		  $$.tokens[$$.len - 1].length = $2.length;
		  $$.tokens[$$.len - 1].ptr = p;
		}
;

TypeExp:
	'(' TypeExp ')'
		{ /* Do nothing.  */ }
|	BasicType
		{ write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, $1);
		  write_exp_elt_opcode (pstate, OP_TYPE); }
|	BasicType BasicType2
		{ $$ = follow_types ($1);
		  write_exp_elt_opcode (pstate, OP_TYPE);
		  write_exp_elt_type (pstate, $$);
		  write_exp_elt_opcode (pstate, OP_TYPE);
		}
;

BasicType2:
	'*'
		{ push_type (tp_pointer); }
|	'*' BasicType2
		{ push_type (tp_pointer); }
|	'[' INTEGER_LITERAL ']'
		{ push_type_int ($2.val);
		  push_type (tp_array); }
|	'[' INTEGER_LITERAL ']' BasicType2
		{ push_type_int ($2.val);
		  push_type (tp_array); }
;

BasicType:
	TYPENAME
		{ $$ = $1.type; }
;

%%

/* Return true if the type is aggregate-like.  */

static int
type_aggregate_p (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
	  || TYPE_CODE (type) == TYPE_CODE_UNION
	  || (TYPE_CODE (type) == TYPE_CODE_ENUM
	      && TYPE_DECLARED_CLASS (type)));
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *ps, const char *p,
	      int len, int parsed_float, YYSTYPE *putithere)
{
  ULONGEST n = 0;
  ULONGEST prevn = 0;
  ULONGEST un;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      const char *suffix;
      int suffix_len;
      char *s, *sp;

      /* Strip out all embedded '_' before passing to parse_float.  */
      s = (char *) alloca (len + 1);
      sp = s;
      while (len-- > 0)
	{
	  if (*p != '_')
	    *sp++ = *p;
	  p++;
	}
      *sp = '\0';
      len = strlen (s);

      if (! parse_float (s, len, &putithere->typed_val_float.dval, &suffix))
	return ERROR;

      suffix_len = s + len - suffix;

      if (suffix_len == 0)
	{
	  putithere->typed_val_float.type
	    = parse_d_type (ps)->builtin_double;
	}
      else if (suffix_len == 1)
	{
	  /* Check suffix for `f', `l', or `i' (float, real, or idouble).  */
	  if (tolower (*suffix) == 'f')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_float;
	    }
	  else if (tolower (*suffix) == 'l')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_real;
	    }
	  else if (tolower (*suffix) == 'i')
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_idouble;
	    }
	  else
	    return ERROR;
	}
      else if (suffix_len == 2)
	{
	  /* Check suffix for `fi' or `li' (ifloat or ireal).  */
	  if (tolower (suffix[0]) == 'f' && tolower (suffix[1] == 'i'))
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ifloat;
	    }
	  else if (tolower (suffix[0]) == 'l' && tolower (suffix[1] == 'i'))
	    {
	      putithere->typed_val_float.type
		= parse_d_type (ps)->builtin_ireal;
	    }
	  else
	    return ERROR;
	}
      else
	return ERROR;

      return FLOAT_LITERAL;
    }

  /* Handle base-switching prefixes 0x, 0b, 0 */
  if (p[0] == '0')
    switch (p[1])
      {
      case 'x':
      case 'X':
	if (len >= 3)
	  {
	    p += 2;
	    base = 16;
	    len -= 2;
	  }
	break;

      case 'b':
      case 'B':
	if (len >= 3)
	  {
	    p += 2;
	    base = 2;
	    len -= 2;
	  }
	break;

      default:
	base = 8;
	break;
      }

  while (len-- > 0)
    {
      c = *p++;
      if (c == '_')
	continue;	/* Ignore embedded '_'.  */
      if (c >= 'A' && c <= 'Z')
	c += 'a' - 'A';
      if (c != 'l' && c != 'u')
	n *= base;
      if (c >= '0' && c <= '9')
	{
	  if (found_suffix)
	    return ERROR;
	  n += i = c - '0';
	}
      else
	{
	  if (base > 10 && c >= 'a' && c <= 'f')
	    {
	      if (found_suffix)
	        return ERROR;
	      n += i = c - 'a' + 10;
	    }
	  else if (c == 'l' && long_p == 0)
	    {
	      long_p = 1;
	      found_suffix = 1;
	    }
	  else if (c == 'u' && unsigned_p == 0)
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */
      /* Portably test for integer overflow.  */
      if (c != 'l' && c != 'u')
	{
	  ULONGEST n2 = prevn * base;
	  if ((n2 / base != prevn) || (n2 + i < prevn))
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int or a long.  An L suffix forces it to
     be long, and a U suffix forces it to be unsigned.  To figure out
     whether it fits, we shift it right and see whether anything remains.
     Note that we can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or
     more in one operation, because many compilers will warn about such a
     shift (which always produces a zero result).  To deal with the case
     where it is we just always shift the value more than once, with fewer
     bits each time.  */
  un = (ULONGEST) n >> 2;
  if (long_p == 0 && (un >> 30) == 0)
    {
      high_bit = ((ULONGEST) 1) << 31;
      signed_type = parse_d_type (ps)->builtin_int;
      /* For decimal notation, keep the sign of the worked out type.  */
      if (base == 10 && !unsigned_p)
	unsigned_type = parse_d_type (ps)->builtin_long;
      else
	unsigned_type = parse_d_type (ps)->builtin_uint;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT < 64)
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = 63;
      high_bit = (ULONGEST) 1 << shift;
      signed_type = parse_d_type (ps)->builtin_long;
      unsigned_type = parse_d_type (ps)->builtin_ulong;
    }

  putithere->typed_val_int.val = n;

  /* If the high bit of the worked out type is set then this number
     has to be unsigned_type.  */
  if (unsigned_p || (n & high_bit))
    putithere->typed_val_int.type = unsigned_type;
  else
    putithere->typed_val_int.type = signed_type;

  return INTEGER_LITERAL;
}

/* Temporary obstack used for holding strings.  */
static struct obstack tempbuf;
static int tempbuf_init;

/* Parse a string or character literal from TOKPTR.  The string or
   character may be wide or unicode.  *OUTPTR is set to just after the
   end of the literal in the input string.  The resulting token is
   stored in VALUE.  This returns a token value, either STRING or
   CHAR, depending on what was parsed.  *HOST_CHARS is set to the
   number of host characters in the literal.  */

static int
parse_string_or_char (const char *tokptr, const char **outptr,
		      struct typed_stoken *value, int *host_chars)
{
  int quote;

  /* Build the gdb internal form of the input string in tempbuf.  Note
     that the buffer is null byte terminated *only* for the
     convenience of debugging gdb itself and printing the buffer
     contents when the buffer contains no embedded nulls.  Gdb does
     not depend upon the buffer being null byte terminated, it uses
     the length string instead.  This allows gdb to handle C strings
     (as well as strings in other languages) with embedded null
     bytes */

  if (!tempbuf_init)
    tempbuf_init = 1;
  else
    obstack_free (&tempbuf, NULL);
  obstack_init (&tempbuf);

  /* Skip the quote.  */
  quote = *tokptr;
  ++tokptr;

  *host_chars = 0;

  while (*tokptr)
    {
      char c = *tokptr;
      if (c == '\\')
	{
	   ++tokptr;
	   *host_chars += c_parse_escape (&tokptr, &tempbuf);
	}
      else if (c == quote)
	break;
      else
	{
	  obstack_1grow (&tempbuf, c);
	  ++tokptr;
	  /* FIXME: this does the wrong thing with multi-byte host
	     characters.  We could use mbrlen here, but that would
	     make "set host-charset" a bit less useful.  */
	  ++*host_chars;
	}
    }

  if (*tokptr != quote)
    {
      if (quote == '"' || quote == '`')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  /* FIXME: should instead use own language string_type enum
     and handle D-specific string suffixes here. */
  if (quote == '\'')
    value->type = C_CHAR;
  else
    value->type = C_STRING;

  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHARACTER_LITERAL : STRING_LITERAL;
}

struct token
{
  char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {"^^=", ASSIGN_MODIFY, BINOP_EXP},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
  };

static const struct token tokentab2[] =
  {
    {"+=", ASSIGN_MODIFY, BINOP_ADD},
    {"-=", ASSIGN_MODIFY, BINOP_SUB},
    {"*=", ASSIGN_MODIFY, BINOP_MUL},
    {"/=", ASSIGN_MODIFY, BINOP_DIV},
    {"%=", ASSIGN_MODIFY, BINOP_REM},
    {"|=", ASSIGN_MODIFY, BINOP_BITWISE_IOR},
    {"&=", ASSIGN_MODIFY, BINOP_BITWISE_AND},
    {"^=", ASSIGN_MODIFY, BINOP_BITWISE_XOR},
    {"++", INCREMENT, BINOP_END},
    {"--", DECREMENT, BINOP_END},
    {"&&", ANDAND, BINOP_END},
    {"||", OROR, BINOP_END},
    {"^^", HATHAT, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END},
    {"..", DOTDOT, BINOP_END},
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"is", IDENTITY, BINOP_END},
    {"!is", NOTIDENTITY, BINOP_END},

    {"cast", CAST_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"immutable", IMMUTABLE_KEYWORD, OP_NULL},
    {"shared", SHARED_KEYWORD, OP_NULL},
    {"super", SUPER_KEYWORD, OP_NULL},

    {"null", NULL_KEYWORD, OP_NULL},
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},

    {"init", INIT_KEYWORD, OP_NULL},
    {"sizeof", SIZEOF_KEYWORD, OP_NULL},
    {"typeof", TYPEOF_KEYWORD, OP_NULL},
    {"typeid", TYPEID_KEYWORD, OP_NULL},

    {"delegate", DELEGATE_KEYWORD, OP_NULL},
    {"function", FUNCTION_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"union", UNION_KEYWORD, OP_NULL},
    {"class", CLASS_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"enum", ENUM_KEYWORD, OP_NULL},
    {"template", TEMPLATE_KEYWORD, OP_NULL},
  };

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure operator.
   This is used only when parsing to do field name completion.  */
static int last_was_structop;

/* Read one token, getting characters through lexptr.  */

static int
lex_one_token (struct parser_state *par_state)
{
  int c;
  int namelen;
  unsigned int i;
  const char *tokstart;
  int saw_structop = last_was_structop;
  char *copy;

  last_was_structop = 0;

 retry:

  prev_lexptr = lexptr;

  tokstart = lexptr;
  /* See if it is a special token of length 3.  */
  for (i = 0; i < sizeof tokentab3 / sizeof tokentab3[0]; i++)
    if (strncmp (tokstart, tokentab3[i].oper, 3) == 0)
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof tokentab2 / sizeof tokentab2[0]; i++)
    if (strncmp (tokstart, tokentab2[i].oper, 2) == 0)
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
      /* If we're parsing for field name completion, and the previous
	 token allows such completion, return a COMPLETE token.
	 Otherwise, we were already scanning the original text, and
	 we're really done.  */
      if (saw_name_at_eof)
	{
	  saw_name_at_eof = 0;
	  return COMPLETE;
	}
      else if (saw_structop)
	return COMPLETE;
      else
        return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '[':
    case '(':
      paren_depth++;
      lexptr++;
      return c;

    case ']':
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      lexptr++;
      return c;

    case ',':
      if (comma_terminates && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	{
	  if (parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol.  */
	}
      /* FALL THRU into number case.  */

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	/* It's a number.  */
	int got_dot = 0, got_e = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;

	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }

	for (;; ++p)
	  {
	    /* Hex exponents start with 'p', because 'e' is a valid hex
	       digit and thus does not indicate a floating point number
	       when the radix is hex.  */
	    if ((!hex && !got_e && tolower (p[0]) == 'e')
		|| (hex && !got_e && tolower (p[0] == 'p')))
	      got_dot = got_e = 1;
	    /* A '.' always indicates a decimal floating point number
	       regardless of the radix.  If we have a '..' then its the
	       end of the number and the beginning of a slice.  */
	    else if (!got_dot && (p[0] == '.' && p[1] != '.'))
		got_dot = 1;
	    /* This is the sign of the exponent, not the end of the number.  */
	    else if (got_e && (tolower (p[-1]) == 'e' || tolower (p[-1]) == 'p')
		     && (*p == '-' || *p == '+'))
	      continue;
	    /* We will take any letters or digits, ignoring any embedded '_'.
	       parse_number will complain if past the radix, or if L or U are
	       not final.  */
	    else if ((*p < '0' || *p > '9') && (*p != '_')
		     && ((*p < 'a' || *p > 'z') && (*p < 'A' || *p > 'Z')))
	      break;
	  }

	toktype = parse_number (par_state, tokstart, p - tokstart,
				got_dot|got_e, &yylval);
	if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error (_("Invalid number \"%s\"."), err_copy);
	  }
	lexptr = p;
	return toktype;
      }

    case '@':
      {
	const char *p = &tokstart[1];
	size_t len = strlen ("entry");

	while (isspace (*p))
	  p++;
	if (strncmp (p, "entry", len) == 0 && !isalnum (p[len])
	    && p[len] != '_')
	  {
	    lexptr = &p[len];
	    return ENTRY;
	  }
      }
      /* FALLTHRU */
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '|':
    case '&':
    case '^':
    case '~':
    case '!':
    case '<':
    case '>':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      lexptr++;
      return c;

    case '\'':
    case '"':
    case '`':
      {
	int host_len;
	int result = parse_string_or_char (tokstart, &lexptr, &yylval.tsval,
					   &host_len);
	if (result == CHARACTER_LITERAL)
	  {
	    if (host_len == 0)
	      error (_("Empty character constant."));
	    else if (host_len > 2 && c == '\'')
	      {
		++tokstart;
		namelen = lexptr - tokstart - 1;
		goto tryname;
	      }
	    else if (host_len > 1)
	      error (_("Invalid character constant."));
	  }
	return result;
      }
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression"), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    c = tokstart[++namelen];

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.  "task" is similar.  Handle abbreviations of these,
     similarly to breakpoint.c:find_condition_and_thread.  */
  if (namelen >= 1
      && (strncmp (tokstart, "thread", namelen) == 0
	  || strncmp (tokstart, "task", namelen) == 0)
      && (tokstart[namelen] == ' ' || tokstart[namelen] == '\t'))
    {
      const char *p = tokstart + namelen + 1;

      while (*p == ' ' || *p == '\t')
        p++;
      if (*p >= '0' && *p <= '9')
        return 0;
    }

  lexptr += namelen;

 tryname:

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Catch specific keywords.  */
  copy = copy_name (yylval.sval);
  for (i = 0; i < sizeof ident_tokens / sizeof ident_tokens[0]; i++)
    if (strcmp (copy, ident_tokens[i].oper) == 0)
      {
	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = ident_tokens[i].opcode;
	return ident_tokens[i].token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  yylval.tsym.type
    = language_lookup_primitive_type (parse_language (par_state),
				      parse_gdbarch (par_state), copy);
  if (yylval.tsym.type != NULL)
    return TYPENAME;

  /* Input names that aren't symbols but ARE valid hex numbers,
     when the input radix permits them, can be names or numbers
     depending on the parse.  Note we support radixes > 16 here.  */
  if ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
      || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, tokstart, namelen, 0, &newlval);
      if (hextype == INTEGER_LITERAL)
	return NAME_OR_INT;
    }

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;

  return IDENTIFIER;
}

/* An object of this type is pushed on a FIFO by the "outer" lexer.  */
typedef struct
{
  int token;
  YYSTYPE value;
} token_and_value;

DEF_VEC_O (token_and_value);

/* A FIFO of tokens that have been read but not yet returned to the
   parser.  */
static VEC (token_and_value) *token_fifo;

/* Non-zero if the lexer should return tokens from the FIFO.  */
static int popping;

/* Temporary storage for yylex; this holds symbol names as they are
   built up.  */
static struct obstack name_obstack;

/* Classify an IDENTIFIER token.  The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.  BLOCK is the block
   in which lookups start; this can be NULL to mean the global scope.  */

static int
classify_name (struct parser_state *par_state, const struct block *block)
{
  struct block_symbol sym;
  char *copy;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy, block, VAR_DOMAIN, &is_a_field_of_this);
  if (sym.symbol && SYMBOL_CLASS (sym.symbol) == LOC_TYPEDEF)
    {
      yylval.tsym.type = SYMBOL_TYPE (sym.symbol);
      return TYPENAME;
    }
  else if (sym.symbol == NULL)
    {
      /* Look-up first for a module name, then a type.  */
      sym = lookup_symbol (copy, block, MODULE_DOMAIN, NULL);
      if (sym.symbol == NULL)
	sym = lookup_symbol (copy, block, STRUCT_DOMAIN, NULL);

      if (sym.symbol != NULL)
	{
	  yylval.tsym.type = SYMBOL_TYPE (sym.symbol);
	  return TYPENAME;
	}

      return UNKNOWN_NAME;
    }

  return IDENTIFIER;
}

/* Like classify_name, but used by the inner loop of the lexer, when a
   name might have already been seen.  CONTEXT is the context type, or
   NULL if this is the first component of a name.  */

static int
classify_inner_name (struct parser_state *par_state,
		     const struct block *block, struct type *context)
{
  struct type *type;
  char *copy;

  if (context == NULL)
    return classify_name (par_state, block);

  type = check_typedef (context);
  if (!type_aggregate_p (type))
    return ERROR;

  copy = copy_name (yylval.ssym.stoken);
  yylval.ssym.sym = d_lookup_nested_symbol (type, copy, block);

  if (yylval.ssym.sym.symbol == NULL)
    return ERROR;

  if (SYMBOL_CLASS (yylval.ssym.sym.symbol) == LOC_TYPEDEF)
    {
      yylval.tsym.type = SYMBOL_TYPE (yylval.ssym.sym.symbol);
      return TYPENAME;
    }

  return IDENTIFIER;
}

/* The outer level of a two-level lexer.  This calls the inner lexer
   to return tokens.  It then either returns these tokens, or
   aggregates them into a larger token.  This lets us work around a
   problem in our parsing approach, where the parser could not
   distinguish between qualified names and qualified types at the
   right point.  */

static int
yylex (void)
{
  token_and_value current;
  int last_was_dot;
  struct type *context_type = NULL;
  int last_to_examine, next_to_examine, checkpoint;
  const struct block *search_block;

  if (popping && !VEC_empty (token_and_value, token_fifo))
    goto do_pop;
  popping = 0;

  /* Read the first token and decide what to do.  */
  current.token = lex_one_token (pstate);
  if (current.token != IDENTIFIER && current.token != '.')
    return current.token;

  /* Read any sequence of alternating "." and identifier tokens into
     the token FIFO.  */
  current.value = yylval;
  VEC_safe_push (token_and_value, token_fifo, &current);
  last_was_dot = current.token == '.';

  while (1)
    {
      current.token = lex_one_token (pstate);
      current.value = yylval;
      VEC_safe_push (token_and_value, token_fifo, &current);

      if ((last_was_dot && current.token != IDENTIFIER)
	  || (!last_was_dot && current.token != '.'))
	break;

      last_was_dot = !last_was_dot;
    }
  popping = 1;

  /* We always read one extra token, so compute the number of tokens
     to examine accordingly.  */
  last_to_examine = VEC_length (token_and_value, token_fifo) - 2;
  next_to_examine = 0;

  current = *VEC_index (token_and_value, token_fifo, next_to_examine);
  ++next_to_examine;

  /* If we are not dealing with a typename, now is the time to find out.  */
  if (current.token == IDENTIFIER)
    {
      yylval = current.value;
      current.token = classify_name (pstate, expression_context_block);
      current.value = yylval;
    }

  /* If the IDENTIFIER is not known, it could be a package symbol,
     first try building up a name until we find the qualified module.  */
  if (current.token == UNKNOWN_NAME)
    {
      obstack_free (&name_obstack, obstack_base (&name_obstack));
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);

      last_was_dot = 0;

      while (next_to_examine <= last_to_examine)
	{
	  token_and_value *next;

	  next = VEC_index (token_and_value, token_fifo, next_to_examine);
	  ++next_to_examine;

	  if (next->token == IDENTIFIER && last_was_dot)
	    {
	      /* Update the partial name we are constructing.  */
              obstack_grow_str (&name_obstack, ".");
	      obstack_grow (&name_obstack, next->value.sval.ptr,
			    next->value.sval.length);

	      yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	      yylval.sval.length = obstack_object_size (&name_obstack);

	      current.token = classify_name (pstate, expression_context_block);
	      current.value = yylval;

	      /* We keep going until we find a TYPENAME.  */
	      if (current.token == TYPENAME)
		{
		  /* Install it as the first token in the FIFO.  */
		  VEC_replace (token_and_value, token_fifo, 0, &current);
		  VEC_block_remove (token_and_value, token_fifo, 1,
				    next_to_examine - 1);
		  break;
		}
	    }
	  else if (next->token == '.' && !last_was_dot)
	    last_was_dot = 1;
	  else
	    {
	      /* We've reached the end of the name.  */
	      break;
	    }
	}

      /* Reset our current token back to the start, if we found nothing
	 this means that we will just jump to do pop.  */
      current = *VEC_index (token_and_value, token_fifo, 0);
      next_to_examine = 1;
    }
  if (current.token != TYPENAME && current.token != '.')
    goto do_pop;

  obstack_free (&name_obstack, obstack_base (&name_obstack));
  checkpoint = 0;
  if (current.token == '.')
    search_block = NULL;
  else
    {
      gdb_assert (current.token == TYPENAME);
      search_block = expression_context_block;
      obstack_grow (&name_obstack, current.value.sval.ptr,
		    current.value.sval.length);
      context_type = current.value.tsym.type;
      checkpoint = 1;
    }

  last_was_dot = current.token == '.';

  while (next_to_examine <= last_to_examine)
    {
      token_and_value *next;

      next = VEC_index (token_and_value, token_fifo, next_to_examine);
      ++next_to_examine;

      if (next->token == IDENTIFIER && last_was_dot)
	{
	  int classification;

	  yylval = next->value;
	  classification = classify_inner_name (pstate, search_block,
						context_type);
	  /* We keep going until we either run out of names, or until
	     we have a qualified name which is not a type.  */
	  if (classification != TYPENAME && classification != IDENTIFIER)
	    break;

	  /* Accept up to this token.  */
	  checkpoint = next_to_examine;

	  /* Update the partial name we are constructing.  */
	  if (context_type != NULL)
	    {
	      /* We don't want to put a leading "." into the name.  */
              obstack_grow_str (&name_obstack, ".");
	    }
	  obstack_grow (&name_obstack, next->value.sval.ptr,
			next->value.sval.length);

	  yylval.sval.ptr = (char *) obstack_base (&name_obstack);
	  yylval.sval.length = obstack_object_size (&name_obstack);
	  current.value = yylval;
	  current.token = classification;

	  last_was_dot = 0;

	  if (classification == IDENTIFIER)
	    break;

	  context_type = yylval.tsym.type;
	}
      else if (next->token == '.' && !last_was_dot)
	last_was_dot = 1;
      else
	{
	  /* We've reached the end of the name.  */
	  break;
	}
    }

  /* If we have a replacement token, install it as the first token in
     the FIFO, and delete the other constituent tokens.  */
  if (checkpoint > 0)
    {
      VEC_replace (token_and_value, token_fifo, 0, &current);
      if (checkpoint > 1)
	VEC_block_remove (token_and_value, token_fifo, 1, checkpoint - 1);
    }

 do_pop:
  current = *VEC_index (token_and_value, token_fifo, 0);
  VEC_ordered_remove (token_and_value, token_fifo, 0);
  yylval = current.value;
  return current.token;
}

int
d_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *back_to;

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  back_to = make_cleanup (null_cleanup, NULL);

  make_cleanup_restore_integer (&yydebug);
  make_cleanup_clear_parser_state (&pstate);
  yydebug = parser_debug;

  /* Initialize some state used by the lexer.  */
  last_was_structop = 0;
  saw_name_at_eof = 0;

  VEC_free (token_and_value, token_fifo);
  popping = 0;
  obstack_init (&name_obstack);
  make_cleanup_obstack_free (&name_obstack);

  result = yyparse ();
  do_cleanups (back_to);
  return result;
}

void
yyerror (char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}

