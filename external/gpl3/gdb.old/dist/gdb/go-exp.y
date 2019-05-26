/* YACC parser for Go expressions, for GDB.

   Copyright (C) 2012-2017 Free Software Foundation, Inc.

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

/* This file is derived from c-exp.y, p-exp.y.  */

/* Parse a Go expression from text in a string,
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

/* Known bugs or limitations:

   - Unicode
   - &^
   - '_' (blank identifier)
   - automatic deref of pointers
   - method expressions
   - interfaces, channels, etc.

   And lots of other things.
   I'm sure there's some cleanup to do.
*/

%{

#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "c-lang.h"
#include "go-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "charset.h"
#include "block.h"

#define parse_type(ps) builtin_type (parse_gdbarch (ps))

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX go_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

int yyparse (void);

static int yylex (void);

void yyerror (const char *);

%}

/* Although the yacc "value" of an expression is not used,
   since the result is stored in the structure being created,
   other node types do have values.  */

%union
  {
    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val_int;
    struct {
      DOUBLEST dval;
      struct type *type;
    } typed_val_float;
    struct stoken sval;
    struct symtoken ssym;
    struct type *tval;
    struct typed_stoken tsval;
    struct ttype tsym;
    int voidval;
    enum exp_opcode opcode;
    struct internalvar *ivar;
    struct stoken_vector svec;
  }

%{
/* YYSTYPE gets defined by %union.  */
static int parse_number (struct parser_state *,
			 const char *, int, int, YYSTYPE *);
static int parse_go_float (struct gdbarch *gdbarch, const char *p, int len,
			   DOUBLEST *d, struct type **t);
%}

%type <voidval> exp exp1 type_exp start variable lcurly
%type <lval> rcurly
%type <tval> type

%token <typed_val_int> INT
%token <typed_val_float> FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a type
   or builtin type name (such as int or char)
   and a NAME is any other symbol.
   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <tsval> RAW_STRING
%token <tsval> STRING
%token <tsval> CHAR
%token <ssym> NAME
%token <tsym> TYPENAME /* Not TYPE_NAME cus already taken.  */
%token <voidval> COMPLETE
/*%type <sval> name*/
%type <svec> string_exp
%type <ssym> name_not_typename

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */
%token <ssym> NAME_OR_INT

%token <lval> TRUE_KEYWORD FALSE_KEYWORD
%token STRUCT_KEYWORD INTERFACE_KEYWORD TYPE_KEYWORD CHAN_KEYWORD
%token SIZEOF_KEYWORD
%token LEN_KEYWORD CAP_KEYWORD
%token NEW_KEYWORD
%token IOTA_KEYWORD NIL_KEYWORD
%token CONST_KEYWORD
%token DOTDOTDOT
%token ENTRY
%token ERROR

/* Special type cases.  */
%token BYTE_KEYWORD /* An alias of uint8.  */

%token <sval> DOLLAR_VARIABLE

%token <opcode> ASSIGN_MODIFY

%left ','
%left ABOVE_COMMA
%right '=' ASSIGN_MODIFY
%right '?'
%left OROR
%left ANDAND
%left '|'
%left '^'
%left '&'
%left ANDNOT
%left EQUAL NOTEQUAL
%left '<' '>' LEQ GEQ
%left LSH RSH
%left '@'
%left '+' '-'
%left '*' '/' '%'
%right UNARY INCREMENT DECREMENT
%right LEFT_ARROW '.' '[' '('


%%

start   :	exp1
	|	type_exp
	;

type_exp:	type
			{ write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate, $1);
			  write_exp_elt_opcode (pstate, OP_TYPE); }
	;

/* Expressions, including the comma operator.  */
exp1	:	exp
	|	exp1 ',' exp
			{ write_exp_elt_opcode (pstate, BINOP_COMMA); }
	;

/* Expressions, not including the comma operator.  */
exp	:	'*' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_IND); }
	;

exp	:	'&' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_ADDR); }
	;

exp	:	'-' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_NEG); }
	;

exp	:	'+' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_PLUS); }
	;

exp	:	'!' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
	;

exp	:	'^' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
	;

exp	:	exp INCREMENT    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_POSTINCREMENT); }
	;

exp	:	exp DECREMENT    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_POSTDECREMENT); }
	;

/* foo->bar is not in Go.  May want as a gdb extension.  Later.  */

exp	:	exp '.' name_not_typename
			{ write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, $3.stoken);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
	;

exp	:	exp '.' name_not_typename COMPLETE
			{ mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  write_exp_string (pstate, $3.stoken);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
	;

exp	:	exp '.' COMPLETE
			{ struct stoken s;
			  mark_struct_expression (pstate);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
			  s.ptr = "";
			  s.length = 0;
			  write_exp_string (pstate, s);
			  write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
	;

exp	:	exp '[' exp1 ']'
			{ write_exp_elt_opcode (pstate, BINOP_SUBSCRIPT); }
	;

exp	:	exp '('
			/* This is to save the value of arglist_len
			   being accumulated by an outer function call.  */
			{ start_arglist (); }
		arglist ')'	%prec LEFT_ARROW
			{ write_exp_elt_opcode (pstate, OP_FUNCALL);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) end_arglist ());
			  write_exp_elt_opcode (pstate, OP_FUNCALL); }
	;

lcurly	:	'{'
			{ start_arglist (); }
	;

arglist	:
	;

arglist	:	exp
			{ arglist_len = 1; }
	;

arglist	:	arglist ',' exp   %prec ABOVE_COMMA
			{ arglist_len++; }
	;

rcurly	:	'}'
			{ $$ = end_arglist () - 1; }
	;

exp	:	lcurly type rcurly exp  %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_MEMVAL);
			  write_exp_elt_type (pstate, $2);
			  write_exp_elt_opcode (pstate, UNOP_MEMVAL); }
	;

exp	:	type '(' exp ')'  %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_CAST);
			  write_exp_elt_type (pstate, $1);
			  write_exp_elt_opcode (pstate, UNOP_CAST); }
	;

exp	:	'(' exp1 ')'
			{ }
	;

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '@' exp
			{ write_exp_elt_opcode (pstate, BINOP_REPEAT); }
	;

exp	:	exp '*' exp
			{ write_exp_elt_opcode (pstate, BINOP_MUL); }
	;

exp	:	exp '/' exp
			{ write_exp_elt_opcode (pstate, BINOP_DIV); }
	;

exp	:	exp '%' exp
			{ write_exp_elt_opcode (pstate, BINOP_REM); }
	;

exp	:	exp '+' exp
			{ write_exp_elt_opcode (pstate, BINOP_ADD); }
	;

exp	:	exp '-' exp
			{ write_exp_elt_opcode (pstate, BINOP_SUB); }
	;

exp	:	exp LSH exp
			{ write_exp_elt_opcode (pstate, BINOP_LSH); }
	;

exp	:	exp RSH exp
			{ write_exp_elt_opcode (pstate, BINOP_RSH); }
	;

exp	:	exp EQUAL exp
			{ write_exp_elt_opcode (pstate, BINOP_EQUAL); }
	;

exp	:	exp NOTEQUAL exp
			{ write_exp_elt_opcode (pstate, BINOP_NOTEQUAL); }
	;

exp	:	exp LEQ exp
			{ write_exp_elt_opcode (pstate, BINOP_LEQ); }
	;

exp	:	exp GEQ exp
			{ write_exp_elt_opcode (pstate, BINOP_GEQ); }
	;

exp	:	exp '<' exp
			{ write_exp_elt_opcode (pstate, BINOP_LESS); }
	;

exp	:	exp '>' exp
			{ write_exp_elt_opcode (pstate, BINOP_GTR); }
	;

exp	:	exp '&' exp
			{ write_exp_elt_opcode (pstate, BINOP_BITWISE_AND); }
	;

exp	:	exp '^' exp
			{ write_exp_elt_opcode (pstate, BINOP_BITWISE_XOR); }
	;

exp	:	exp '|' exp
			{ write_exp_elt_opcode (pstate, BINOP_BITWISE_IOR); }
	;

exp	:	exp ANDAND exp
			{ write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
	;

exp	:	exp OROR exp
			{ write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
	;

exp	:	exp '?' exp ':' exp	%prec '?'
			{ write_exp_elt_opcode (pstate, TERNOP_COND); }
	;

exp	:	exp '=' exp
			{ write_exp_elt_opcode (pstate, BINOP_ASSIGN); }
	;

exp	:	exp ASSIGN_MODIFY exp
			{ write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode (pstate, $2);
			  write_exp_elt_opcode (pstate, BINOP_ASSIGN_MODIFY); }
	;

exp	:	INT
			{ write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, $1.type);
			  write_exp_elt_longcst (pstate, (LONGEST)($1.val));
			  write_exp_elt_opcode (pstate, OP_LONG); }
	;

exp	:	CHAR
			{
			  struct stoken_vector vec;
			  vec.len = 1;
			  vec.tokens = &$1;
			  write_exp_string_vector (pstate, $1.type, &vec);
			}
	;

exp	:	NAME_OR_INT
			{ YYSTYPE val;
			  parse_number (pstate, $1.stoken.ptr,
					$1.stoken.length, 0, &val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, val.typed_val_int.type);
			  write_exp_elt_longcst (pstate, (LONGEST)
						 val.typed_val_int.val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			}
	;


exp	:	FLOAT
			{ write_exp_elt_opcode (pstate, OP_DOUBLE);
			  write_exp_elt_type (pstate, $1.type);
			  write_exp_elt_dblcst (pstate, $1.dval);
			  write_exp_elt_opcode (pstate, OP_DOUBLE); }
	;

exp	:	variable
	;

exp	:	DOLLAR_VARIABLE
			{
			  write_dollar_variable (pstate, $1);
			}
	;

exp	:	SIZEOF_KEYWORD '(' type ')'  %prec UNARY
			{
			  /* TODO(dje): Go objects in structs.  */
			  write_exp_elt_opcode (pstate, OP_LONG);
			  /* TODO(dje): What's the right type here?  */
			  write_exp_elt_type
			    (pstate,
			     parse_type (pstate)->builtin_unsigned_int);
			  $3 = check_typedef ($3);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) TYPE_LENGTH ($3));
			  write_exp_elt_opcode (pstate, OP_LONG);
			}
	;

exp	:	SIZEOF_KEYWORD  '(' exp ')'  %prec UNARY
			{
			  /* TODO(dje): Go objects in structs.  */
			  write_exp_elt_opcode (pstate, UNOP_SIZEOF);
			}

string_exp:
		STRING
			{
			  /* We copy the string here, and not in the
			     lexer, to guarantee that we do not leak a
			     string.  */
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  struct typed_stoken *vec = XNEW (struct typed_stoken);
			  $$.len = 1;
			  $$.tokens = vec;

			  vec->type = $1.type;
			  vec->length = $1.length;
			  vec->ptr = (char *) malloc ($1.length + 1);
			  memcpy (vec->ptr, $1.ptr, $1.length + 1);
			}

	|	string_exp '+' STRING
			{
			  /* Note that we NUL-terminate here, but just
			     for convenience.  */
			  char *p;
			  ++$$.len;
			  $$.tokens = XRESIZEVEC (struct typed_stoken,
						  $$.tokens, $$.len);

			  p = (char *) malloc ($3.length + 1);
			  memcpy (p, $3.ptr, $3.length + 1);

			  $$.tokens[$$.len - 1].type = $3.type;
			  $$.tokens[$$.len - 1].length = $3.length;
			  $$.tokens[$$.len - 1].ptr = p;
			}
	;

exp	:	string_exp  %prec ABOVE_COMMA
			{
			  int i;

			  write_exp_string_vector (pstate, 0 /*always utf8*/,
						   &$1);
			  for (i = 0; i < $1.len; ++i)
			    free ($1.tokens[i].ptr);
			  free ($1.tokens);
			}
	;

exp	:	TRUE_KEYWORD
			{ write_exp_elt_opcode (pstate, OP_BOOL);
			  write_exp_elt_longcst (pstate, (LONGEST) $1);
			  write_exp_elt_opcode (pstate, OP_BOOL); }
	;

exp	:	FALSE_KEYWORD
			{ write_exp_elt_opcode (pstate, OP_BOOL);
			  write_exp_elt_longcst (pstate, (LONGEST) $1);
			  write_exp_elt_opcode (pstate, OP_BOOL); }
	;

variable:	name_not_typename ENTRY
			{ struct symbol *sym = $1.sym.symbol;

			  if (sym == NULL
			      || !SYMBOL_IS_ARGUMENT (sym)
			      || !symbol_read_needs_frame (sym))
			    error (_("@entry can be used only for function "
				     "parameters, not for \"%s\""),
				   copy_name ($1.stoken));

			  write_exp_elt_opcode (pstate, OP_VAR_ENTRY_VALUE);
			  write_exp_elt_sym (pstate, sym);
			  write_exp_elt_opcode (pstate, OP_VAR_ENTRY_VALUE);
			}
	;

variable:	name_not_typename
			{ struct block_symbol sym = $1.sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				{
				  if (innermost_block == 0
				      || contained_in (sym.block,
						       innermost_block))
				    innermost_block = sym.block;
				}

			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      write_exp_elt_block (pstate, sym.block);
			      write_exp_elt_sym (pstate, sym.symbol);
			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			    }
			  else if ($1.is_a_field_of_this)
			    {
			      /* TODO(dje): Can we get here?
				 E.g., via a mix of c++ and go?  */
			      gdb_assert_not_reached ("go with `this' field");
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      char *arg = copy_name ($1.stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg);
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (pstate, msymbol);
			      else if (!have_full_symbols ()
				       && !have_partial_symbols ())
				error (_("No symbol table is loaded.  "
				       "Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       copy_name ($1.stoken));
			    }
			}
	;

/* TODO
method_exp: PACKAGENAME '.' name '.' name
			{
			}
	;
*/

type  /* Implements (approximately): [*] type-specifier */
	:	'*' type
			{ $$ = lookup_pointer_type ($2); }
	|	TYPENAME
			{ $$ = $1.type; }
/*
	|	STRUCT_KEYWORD name
			{ $$ = lookup_struct (copy_name ($2),
					      expression_context_block); }
*/
	|	BYTE_KEYWORD
			{ $$ = builtin_go_type (parse_gdbarch (pstate))
			    ->builtin_uint8; }
	;

/* TODO
name	:	NAME { $$ = $1.stoken; }
	|	TYPENAME { $$ = $1.stoken; }
	|	NAME_OR_INT  { $$ = $1.stoken; }
	;
*/

name_not_typename
	:	NAME
/* These would be useful if name_not_typename was useful, but it is just
   a fake for "variable", so these cause reduce/reduce conflicts because
   the parser can't tell whether NAME_OR_INT is a name_not_typename (=variable,
   =exp) or just an exp.  If name_not_typename was ever used in an lvalue
   context where only a name could occur, this might be useful.
	|	NAME_OR_INT
*/
	;

%%

/* Wrapper on parse_c_float to get the type right for Go.  */

static int
parse_go_float (struct gdbarch *gdbarch, const char *p, int len,
		DOUBLEST *d, struct type **t)
{
  int result = parse_c_float (gdbarch, p, len, d, t);
  const struct builtin_type *builtin_types = builtin_type (gdbarch);
  const struct builtin_go_type *builtin_go_types = builtin_go_type (gdbarch);

  if (*t == builtin_types->builtin_float)
    *t = builtin_go_types->builtin_float32;
  else if (*t == builtin_types->builtin_double)
    *t = builtin_go_types->builtin_float64;

  return result;
}

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/* FIXME: Needs some error checking for the float case.  */
/* FIXME(dje): IWBN to use c-exp.y's parse_number if we could.
   That will require moving the guts into a function that we both call
   as our YYSTYPE is different than c-exp.y's  */

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  /* FIXME: Shouldn't these be unsigned?  We don't deal with negative values
     here, and we do kind of silly things like cast to unsigned.  */
  LONGEST n = 0;
  LONGEST prevn = 0;
  ULONGEST un;

  int i = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;

  /* Number of "L" suffixes encountered.  */
  int long_p = 0;

  /* We have found a "L" or "U" suffix.  */
  int found_suffix = 0;

  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      if (! parse_go_float (parse_gdbarch (par_state), p, len,
			    &putithere->typed_val_float.dval,
			    &putithere->typed_val_float.type))
	return ERROR;
      return FLOAT;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0.  */
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

      case 't':
      case 'T':
      case 'd':
      case 'D':
	if (len >= 3)
	  {
	    p += 2;
	    base = 10;
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
	  else if (c == 'l')
	    {
	      ++long_p;
	      found_suffix = 1;
	    }
	  else if (c == 'u')
	    {
	      unsigned_p = 1;
	      found_suffix = 1;
	    }
	  else
	    return ERROR;	/* Char not a digit */
	}
      if (i >= base)
	return ERROR;		/* Invalid digit in this base.  */

      /* Portably test for overflow (only works for nonzero values, so make
	 a second check for zero).  FIXME: Can't we just make n and prevn
	 unsigned and avoid this?  */
      if (c != 'l' && c != 'u' && (prevn >= n) && n != 0)
	unsigned_p = 1;		/* Try something unsigned.  */

      /* Portably test for unsigned overflow.
	 FIXME: This check is wrong; for example it doesn't find overflow
	 on 0x123456789 when LONGEST is 32 bits.  */
      if (c != 'l' && c != 'u' && n != 0)
	{
	  if ((unsigned_p && (ULONGEST) prevn >= (ULONGEST) n))
	    error (_("Numeric constant too large."));
	}
      prevn = n;
    }

  /* An integer constant is an int, a long, or a long long.  An L
     suffix forces it to be long; an LL suffix forces it to be long
     long.  If not forced to a larger size, it gets the first type of
     the above that it fits in.  To figure out whether it fits, we
     shift it right and see whether anything remains.  Note that we
     can't shift sizeof (LONGEST) * HOST_CHAR_BIT bits or more in one
     operation, because many compilers will warn about such a shift
     (which always produces a zero result).  Sometimes gdbarch_int_bit
     or gdbarch_long_bit will be that big, sometimes not.  To deal with
     the case where it is we just always shift the value more than
     once, with fewer bits each time.  */

  un = (ULONGEST)n >> 2;
  if (long_p == 0
      && (un >> (gdbarch_int_bit (parse_gdbarch (par_state)) - 2)) == 0)
    {
      high_bit
        = ((ULONGEST)1) << (gdbarch_int_bit (parse_gdbarch (par_state)) - 1);

      /* A large decimal (not hex or octal) constant (between INT_MAX
	 and UINT_MAX) is a long or unsigned long, according to ANSI,
	 never an unsigned int, but this code treats it as unsigned
	 int.  This probably should be fixed.  GCC gives a warning on
	 such constants.  */

      unsigned_type = parse_type (par_state)->builtin_unsigned_int;
      signed_type = parse_type (par_state)->builtin_int;
    }
  else if (long_p <= 1
	   && (un >> (gdbarch_long_bit (parse_gdbarch (par_state)) - 2)) == 0)
    {
      high_bit
	= ((ULONGEST)1) << (gdbarch_long_bit (parse_gdbarch (par_state)) - 1);
      unsigned_type = parse_type (par_state)->builtin_unsigned_long;
      signed_type = parse_type (par_state)->builtin_long;
    }
  else
    {
      int shift;
      if (sizeof (ULONGEST) * HOST_CHAR_BIT
	  < gdbarch_long_long_bit (parse_gdbarch (par_state)))
	/* A long long does not fit in a LONGEST.  */
	shift = (sizeof (ULONGEST) * HOST_CHAR_BIT - 1);
      else
	shift = (gdbarch_long_long_bit (parse_gdbarch (par_state)) - 1);
      high_bit = (ULONGEST) 1 << shift;
      unsigned_type = parse_type (par_state)->builtin_unsigned_long_long;
      signed_type = parse_type (par_state)->builtin_long_long;
    }

   putithere->typed_val_int.val = n;

   /* If the high bit of the worked out type is set then this number
      has to be unsigned.  */

   if (unsigned_p || (n & high_bit))
     {
       putithere->typed_val_int.type = unsigned_type;
     }
   else
     {
       putithere->typed_val_int.type = signed_type;
     }

   return INT;
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
      if (quote == '"')
	error (_("Unterminated string in expression."));
      else
	error (_("Unmatched single quote."));
    }
  ++tokptr;

  value->type = C_STRING | (quote == '\'' ? C_CHAR : 0); /*FIXME*/
  value->ptr = (char *) obstack_base (&tempbuf);
  value->length = obstack_object_size (&tempbuf);

  *outptr = tokptr;

  return quote == '\'' ? CHAR : STRING;
}

struct token
{
  const char *oper;
  int token;
  enum exp_opcode opcode;
};

static const struct token tokentab3[] =
  {
    {">>=", ASSIGN_MODIFY, BINOP_RSH},
    {"<<=", ASSIGN_MODIFY, BINOP_LSH},
    /*{"&^=", ASSIGN_MODIFY, BINOP_BITWISE_ANDNOT}, TODO */
    {"...", DOTDOTDOT, OP_NULL},
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
    /*{"->", RIGHT_ARROW, BINOP_END}, Doesn't exist in Go.  */
    {"<-", LEFT_ARROW, BINOP_END},
    {"&&", ANDAND, BINOP_END},
    {"||", OROR, BINOP_END},
    {"<<", LSH, BINOP_END},
    {">>", RSH, BINOP_END},
    {"==", EQUAL, BINOP_END},
    {"!=", NOTEQUAL, BINOP_END},
    {"<=", LEQ, BINOP_END},
    {">=", GEQ, BINOP_END},
    /*{"&^", ANDNOT, BINOP_END}, TODO */
  };

/* Identifier-like tokens.  */
static const struct token ident_tokens[] =
  {
    {"true", TRUE_KEYWORD, OP_NULL},
    {"false", FALSE_KEYWORD, OP_NULL},
    {"nil", NIL_KEYWORD, OP_NULL},
    {"const", CONST_KEYWORD, OP_NULL},
    {"struct", STRUCT_KEYWORD, OP_NULL},
    {"type", TYPE_KEYWORD, OP_NULL},
    {"interface", INTERFACE_KEYWORD, OP_NULL},
    {"chan", CHAN_KEYWORD, OP_NULL},
    {"byte", BYTE_KEYWORD, OP_NULL}, /* An alias of uint8.  */
    {"len", LEN_KEYWORD, OP_NULL},
    {"cap", CAP_KEYWORD, OP_NULL},
    {"new", NEW_KEYWORD, OP_NULL},
    {"iota", IOTA_KEYWORD, OP_NULL},
  };

/* This is set if a NAME token appeared at the very end of the input
   string, with no whitespace separating the name from the EOF.  This
   is used only when parsing to do field name completion.  */
static int saw_name_at_eof;

/* This is set if the previously-returned token was a structure
   operator -- either '.' or ARROW.  This is used only when parsing to
   do field name completion.  */
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
  for (i = 0; i < sizeof (tokentab3) / sizeof (tokentab3[0]); i++)
    if (strncmp (tokstart, tokentab3[i].oper, 3) == 0)
      {
	lexptr += 3;
	yylval.opcode = tokentab3[i].opcode;
	return tokentab3[i].token;
      }

  /* See if it is a special token of length 2.  */
  for (i = 0; i < sizeof (tokentab2) / sizeof (tokentab2[0]); i++)
    if (strncmp (tokstart, tokentab2[i].oper, 2) == 0)
      {
	lexptr += 2;
	yylval.opcode = tokentab2[i].opcode;
	/* NOTE: -> doesn't exist in Go, so we don't need to watch for
	   setting last_was_structop here.  */
	return tokentab2[i].token;
      }

  switch (c = *tokstart)
    {
    case 0:
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
      if (comma_terminates
          && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] < '0' || lexptr[1] > '9')
	{
	  if (parse_completion)
	    last_was_structop = 1;
	  goto symbol;		/* Nope, must be a symbol. */
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
	    /* This test includes !hex because 'e' is a valid hex digit
	       and thus does not indicate a floating point number when
	       the radix is hex.  */
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    /* This test does not include !hex, because a '.' always indicates
	       a decimal floating point number regardless of the radix.  */
	    else if (!got_dot && *p == '.')
	      got_dot = 1;
	    else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		     && (*p == '-' || *p == '+'))
	      /* This is the sign of the exponent, not the end of the
		 number.  */
	      continue;
	    /* We will take any letters or digits.  parse_number will
	       complain if past the radix, or if L or U are not final.  */
	    else if ((*p < '0' || *p > '9')
		     && ((*p < 'a' || *p > 'z')
				  && (*p < 'A' || *p > 'Z')))
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
	if (result == CHAR)
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
    error (_("Invalid character '%c' in expression."), c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));)
    {
      c = tokstart[++namelen];
    }

  /* The token "if" terminates the expression and is NOT removed from
     the input stream.  It doesn't count if it appears in the
     expansion of a macro.  */
  if (namelen == 2
      && tokstart[0] == 'i'
      && tokstart[1] == 'f')
    {
      return 0;
    }

  /* For the same reason (breakpoint conditions), "thread N"
     terminates the expression.  "thread" could be an identifier, but
     an identifier is never followed by a number without intervening
     punctuation.
     Handle abbreviations of these, similarly to
     breakpoint.c:find_condition_and_thread.
     TODO: Watch for "goroutine" here?  */
  if (namelen >= 1
      && strncmp (tokstart, "thread", namelen) == 0
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
  for (i = 0; i < sizeof (ident_tokens) / sizeof (ident_tokens[0]); i++)
    if (strcmp (copy, ident_tokens[i].oper) == 0)
      {
	/* It is ok to always set this, even though we don't always
	   strictly need to.  */
	yylval.opcode = ident_tokens[i].opcode;
	return ident_tokens[i].token;
      }

  if (*tokstart == '$')
    return DOLLAR_VARIABLE;

  if (parse_completion && *lexptr == '\0')
    saw_name_at_eof = 1;
  return NAME;
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

/* Build "package.name" in name_obstack.
   For convenience of the caller, the name is NUL-terminated,
   but the NUL is not included in the recorded length.  */

static struct stoken
build_packaged_name (const char *package, int package_len,
		     const char *name, int name_len)
{
  struct stoken result;

  obstack_free (&name_obstack, obstack_base (&name_obstack));
  obstack_grow (&name_obstack, package, package_len);
  obstack_grow_str (&name_obstack, ".");
  obstack_grow (&name_obstack, name, name_len);
  obstack_grow (&name_obstack, "", 1);
  result.ptr = (char *) obstack_base (&name_obstack);
  result.length = obstack_object_size (&name_obstack) - 1;

  return result;
}

/* Return non-zero if NAME is a package name.
   BLOCK is the scope in which to interpret NAME; this can be NULL
   to mean the global scope.  */

static int
package_name_p (const char *name, const struct block *block)
{
  struct symbol *sym;
  struct field_of_this_result is_a_field_of_this;

  sym = lookup_symbol (name, block, STRUCT_DOMAIN, &is_a_field_of_this).symbol;

  if (sym
      && SYMBOL_CLASS (sym) == LOC_TYPEDEF
      && TYPE_CODE (SYMBOL_TYPE (sym)) == TYPE_CODE_MODULE)
    return 1;

  return 0;
}

/* Classify a (potential) function in the "unsafe" package.
   We fold these into "keywords" to keep things simple, at least until
   something more complex is warranted.  */

static int
classify_unsafe_function (struct stoken function_name)
{
  char *copy = copy_name (function_name);

  if (strcmp (copy, "Sizeof") == 0)
    {
      yylval.sval = function_name;
      return SIZEOF_KEYWORD;
    }

  error (_("Unknown function in `unsafe' package: %s"), copy);
}

/* Classify token(s) "name1.name2" where name1 is known to be a package.
   The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.

   The result is one of NAME, NAME_OR_INT, or TYPENAME.  */

static int
classify_packaged_name (const struct block *block)
{
  char *copy;
  struct block_symbol sym;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  sym = lookup_symbol (copy, block, VAR_DOMAIN, &is_a_field_of_this);

  if (sym.symbol)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
    }

  return NAME;
}

/* Classify a NAME token.
   The contents of the token are in `yylval'.
   Updates yylval and returns the new token type.
   BLOCK is the block in which lookups start; this can be NULL
   to mean the global scope.

   The result is one of NAME, NAME_OR_INT, or TYPENAME.  */

static int
classify_name (struct parser_state *par_state, const struct block *block)
{
  struct type *type;
  struct block_symbol sym;
  char *copy;
  struct field_of_this_result is_a_field_of_this;

  copy = copy_name (yylval.sval);

  /* Try primitive types first so they win over bad/weird debug info.  */
  type = language_lookup_primitive_type (parse_language (par_state),
					 parse_gdbarch (par_state),
					 copy);
  if (type != NULL)
    {
      /* NOTE: We take advantage of the fact that yylval coming in was a
	 NAME, and that struct ttype is a compatible extension of struct
	 stoken, so yylval.tsym.stoken is already filled in.  */
      yylval.tsym.type = type;
      return TYPENAME;
    }

  /* TODO: What about other types?  */

  sym = lookup_symbol (copy, block, VAR_DOMAIN, &is_a_field_of_this);

  if (sym.symbol)
    {
      yylval.ssym.sym = sym;
      yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
      return NAME;
    }

  /* If we didn't find a symbol, look again in the current package.
     This is to, e.g., make "p global_var" work without having to specify
     the package name.  We intentionally only looks for objects in the
     current package.  */

  {
    char *current_package_name = go_block_package_name (block);

    if (current_package_name != NULL)
      {
	struct stoken sval =
	  build_packaged_name (current_package_name,
			       strlen (current_package_name),
			       copy, strlen (copy));

	xfree (current_package_name);
	sym = lookup_symbol (sval.ptr, block, VAR_DOMAIN,
			     &is_a_field_of_this);
	if (sym.symbol)
	  {
	    yylval.ssym.stoken = sval;
	    yylval.ssym.sym = sym;
	    yylval.ssym.is_a_field_of_this = is_a_field_of_this.type != NULL;
	    return NAME;
	  }
      }
  }

  /* Input names that aren't symbols but ARE valid hex numbers, when
     the input radix permits them, can be names or numbers depending
     on the parse.  Note we support radixes > 16 here.  */
  if ((copy[0] >= 'a' && copy[0] < 'a' + input_radix - 10)
      || (copy[0] >= 'A' && copy[0] < 'A' + input_radix - 10))
    {
      YYSTYPE newlval;	/* Its value is ignored.  */
      int hextype = parse_number (par_state, copy, yylval.sval.length,
				  0, &newlval);
      if (hextype == INT)
	{
	  yylval.ssym.sym.symbol = NULL;
	  yylval.ssym.sym.block = NULL;
	  yylval.ssym.is_a_field_of_this = 0;
	  return NAME_OR_INT;
	}
    }

  yylval.ssym.sym.symbol = NULL;
  yylval.ssym.sym.block = NULL;
  yylval.ssym.is_a_field_of_this = 0;
  return NAME;
}

/* This is taken from c-exp.y mostly to get something working.
   The basic structure has been kept because we may yet need some of it.  */

static int
yylex (void)
{
  token_and_value current, next;

  if (popping && !VEC_empty (token_and_value, token_fifo))
    {
      token_and_value tv = *VEC_index (token_and_value, token_fifo, 0);
      VEC_ordered_remove (token_and_value, token_fifo, 0);
      yylval = tv.value;
      /* There's no need to fall through to handle package.name
	 as that can never happen here.  In theory.  */
      return tv.token;
    }
  popping = 0;

  current.token = lex_one_token (pstate);

  /* TODO: Need a way to force specifying name1 as a package.
     .name1.name2 ?  */

  if (current.token != NAME)
    return current.token;

  /* See if we have "name1 . name2".  */

  current.value = yylval;
  next.token = lex_one_token (pstate);
  next.value = yylval;

  if (next.token == '.')
    {
      token_and_value name2;

      name2.token = lex_one_token (pstate);
      name2.value = yylval;

      if (name2.token == NAME)
	{
	  /* Ok, we have "name1 . name2".  */
	  char *copy;

	  copy = copy_name (current.value.sval);

	  if (strcmp (copy, "unsafe") == 0)
	    {
	      popping = 1;
	      return classify_unsafe_function (name2.value.sval);
	    }

	  if (package_name_p (copy, expression_context_block))
	    {
	      popping = 1;
	      yylval.sval = build_packaged_name (current.value.sval.ptr,
						 current.value.sval.length,
						 name2.value.sval.ptr,
						 name2.value.sval.length);
	      return classify_packaged_name (expression_context_block);
	    }
	}

      VEC_safe_push (token_and_value, token_fifo, &next);
      VEC_safe_push (token_and_value, token_fifo, &name2);
    }
  else
    {
      VEC_safe_push (token_and_value, token_fifo, &next);
    }

  /* If we arrive here we don't have a package-qualified name.  */

  popping = 1;
  yylval = current.value;
  return classify_name (pstate, expression_context_block);
}

int
go_parse (struct parser_state *par_state)
{
  int result;
  struct cleanup *back_to;

  /* Setting up the parser state.  */
  gdb_assert (par_state != NULL);
  pstate = par_state;

  back_to = make_cleanup (null_cleanup, NULL);

  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);
  make_cleanup_clear_parser_state (&pstate);

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
yyerror (const char *msg)
{
  if (prev_lexptr)
    lexptr = prev_lexptr;

  error (_("A %s in expression, near `%s'."), (msg ? msg : "error"), lexptr);
}
