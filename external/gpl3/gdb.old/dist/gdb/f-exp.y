
/* YACC parser for Fortran expressions, for GDB.
   Copyright (C) 1986-2020 Free Software Foundation, Inc.

   Contributed by Motorola.  Adapted from the C parser by Farooq Butt
   (fmbutt@engage.sps.mot.com).

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

/* This was blantantly ripped off the C expression parser, please 
   be aware of that as you look at its basic structure -FMB */ 

/* Parse a F77 expression from text in a string,
   and return the result as a  struct expression  pointer.
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
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "f-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "block.h"
#include <ctype.h>
#include <algorithm>
#include "type-stack.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())
#define parse_f_type(ps) builtin_f_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX f_
#include "yy-remap.h"

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* Depth of parentheses.  */
static int paren_depth;

/* The current type stack.  */
static struct type_stack *type_stack;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static void growbuf_by_size (int);

static int match_string_literal (void);

static void push_kind_type (LONGEST val, struct type *type);

static struct type *convert_to_kind_type (struct type *basetype, int kind);

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
    } typed_val;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    struct ttype tsym;
    struct symtoken ssym;
    int voidval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
  }

%{
/* YYSTYPE gets defined by %union */
static int parse_number (struct parser_state *, const char *, int,
			 int, YYSTYPE *);
%}

%type <voidval> exp  type_exp start variable 
%type <tval> type typebase
%type <tvec> nonempty_typelist
/* %type <bval> block */

/* Fancy type parsing.  */
%type <voidval> func_mod direct_abs_decl abs_decl
%type <tval> ptype

%token <typed_val> INT
%token <typed_val_float> FLOAT

/* Both NAME and TYPENAME tokens represent symbols in the input,
   and both convey their data as strings.
   But a TYPENAME is a string that happens to be defined as a typedef
   or builtin type name (such as int or char)
   and a NAME is any other symbol.
   Contexts where this distinction is not important can use the
   nonterminal "name", which matches either NAME or TYPENAME.  */

%token <sval> STRING_LITERAL
%token <lval> BOOLEAN_LITERAL
%token <ssym> NAME 
%token <tsym> TYPENAME
%type <sval> name
%type <ssym> name_not_typename

/* A NAME_OR_INT is a symbol which is not known in the symbol table,
   but which would parse as a valid number in the current input radix.
   E.g. "c" when input_radix==16.  Depending on the parse, it will be
   turned into a name or into a number.  */

%token <ssym> NAME_OR_INT 

%token SIZEOF KIND
%token ERROR

/* Special type cases, put in to allow the parser to distinguish different
   legal basetypes.  */
%token INT_KEYWORD INT_S2_KEYWORD LOGICAL_S1_KEYWORD LOGICAL_S2_KEYWORD 
%token LOGICAL_S8_KEYWORD
%token LOGICAL_KEYWORD REAL_KEYWORD REAL_S8_KEYWORD REAL_S16_KEYWORD 
%token COMPLEX_KEYWORD
%token COMPLEX_S8_KEYWORD COMPLEX_S16_KEYWORD COMPLEX_S32_KEYWORD 
%token BOOL_AND BOOL_OR BOOL_NOT   
%token SINGLE DOUBLE PRECISION
%token <lval> CHARACTER 

%token <voidval> DOLLAR_VARIABLE

%token <opcode> ASSIGN_MODIFY
%token <opcode> UNOP_INTRINSIC BINOP_INTRINSIC

%left ','
%left ABOVE_COMMA
%right '=' ASSIGN_MODIFY
%right '?'
%left BOOL_OR
%right BOOL_NOT
%left BOOL_AND
%left '|'
%left '^'
%left '&'
%left EQUAL NOTEQUAL
%left LESSTHAN GREATERTHAN LEQ GEQ
%left LSH RSH
%left '@'
%left '+' '-'
%left '*' '/'
%right STARSTAR
%right '%'
%right UNARY 
%right '('


%%

start   :	exp
	|	type_exp
	;

type_exp:	type
			{ write_exp_elt_opcode (pstate, OP_TYPE);
			  write_exp_elt_type (pstate, $1);
			  write_exp_elt_opcode (pstate, OP_TYPE); }
	;

exp     :       '(' exp ')'
        		{ }
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

exp	:	BOOL_NOT exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_LOGICAL_NOT); }
	;

exp	:	'~' exp    %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_COMPLEMENT); }
	;

exp	:	SIZEOF exp       %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_SIZEOF); }
	;

exp	:	KIND '(' exp ')'       %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_FORTRAN_KIND); }
	;

/* No more explicit array operators, we treat everything in F77 as 
   a function call.  The disambiguation as to whether we are 
   doing a subscript operation or a function call is done 
   later in eval.c.  */

exp	:	exp '(' 
			{ pstate->start_arglist (); }
		arglist ')'	
			{ write_exp_elt_opcode (pstate,
						OP_F77_UNDETERMINED_ARGLIST);
			  write_exp_elt_longcst (pstate,
						 pstate->end_arglist ());
			  write_exp_elt_opcode (pstate,
					      OP_F77_UNDETERMINED_ARGLIST); }
	;

exp	:	UNOP_INTRINSIC '(' exp ')'
			{ write_exp_elt_opcode (pstate, $1); }
	;

exp	:	BINOP_INTRINSIC '(' exp ',' exp ')'
			{ write_exp_elt_opcode (pstate, $1); }
	;

arglist	:
	;

arglist	:	exp
			{ pstate->arglist_len = 1; }
	;

arglist :	subrange
			{ pstate->arglist_len = 1; }
	;
   
arglist	:	arglist ',' exp   %prec ABOVE_COMMA
			{ pstate->arglist_len++; }
	;

/* There are four sorts of subrange types in F90.  */

subrange:	exp ':' exp	%prec ABOVE_COMMA
			{ write_exp_elt_opcode (pstate, OP_RANGE); 
			  write_exp_elt_longcst (pstate, NONE_BOUND_DEFAULT);
			  write_exp_elt_opcode (pstate, OP_RANGE); }
	;

subrange:	exp ':'	%prec ABOVE_COMMA
			{ write_exp_elt_opcode (pstate, OP_RANGE);
			  write_exp_elt_longcst (pstate, HIGH_BOUND_DEFAULT);
			  write_exp_elt_opcode (pstate, OP_RANGE); }
	;

subrange:	':' exp	%prec ABOVE_COMMA
			{ write_exp_elt_opcode (pstate, OP_RANGE);
			  write_exp_elt_longcst (pstate, LOW_BOUND_DEFAULT);
			  write_exp_elt_opcode (pstate, OP_RANGE); }
	;

subrange:	':'	%prec ABOVE_COMMA
			{ write_exp_elt_opcode (pstate, OP_RANGE);
			  write_exp_elt_longcst (pstate, BOTH_BOUND_DEFAULT);
			  write_exp_elt_opcode (pstate, OP_RANGE); }
	;

complexnum:     exp ',' exp 
                	{ }                          
        ;

exp	:	'(' complexnum ')'
			{ write_exp_elt_opcode (pstate, OP_COMPLEX);
			  write_exp_elt_type (pstate,
					      parse_f_type (pstate)
					      ->builtin_complex_s16);
			  write_exp_elt_opcode (pstate, OP_COMPLEX); }
	;

exp	:	'(' type ')' exp  %prec UNARY
			{ write_exp_elt_opcode (pstate, UNOP_CAST);
			  write_exp_elt_type (pstate, $2);
			  write_exp_elt_opcode (pstate, UNOP_CAST); }
	;

exp     :       exp '%' name
                        { write_exp_elt_opcode (pstate, STRUCTOP_STRUCT);
                          write_exp_string (pstate, $3);
                          write_exp_elt_opcode (pstate, STRUCTOP_STRUCT); }
        ;

/* Binary operators in order of decreasing precedence.  */

exp	:	exp '@' exp
			{ write_exp_elt_opcode (pstate, BINOP_REPEAT); }
	;

exp	:	exp STARSTAR exp
			{ write_exp_elt_opcode (pstate, BINOP_EXP); }
	;

exp	:	exp '*' exp
			{ write_exp_elt_opcode (pstate, BINOP_MUL); }
	;

exp	:	exp '/' exp
			{ write_exp_elt_opcode (pstate, BINOP_DIV); }
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

exp	:	exp LESSTHAN exp
			{ write_exp_elt_opcode (pstate, BINOP_LESS); }
	;

exp	:	exp GREATERTHAN exp
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

exp     :       exp BOOL_AND exp
			{ write_exp_elt_opcode (pstate, BINOP_LOGICAL_AND); }
	;


exp	:	exp BOOL_OR exp
			{ write_exp_elt_opcode (pstate, BINOP_LOGICAL_OR); }
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
			  write_exp_elt_longcst (pstate, (LONGEST) ($1.val));
			  write_exp_elt_opcode (pstate, OP_LONG); }
	;

exp	:	NAME_OR_INT
			{ YYSTYPE val;
			  parse_number (pstate, $1.stoken.ptr,
					$1.stoken.length, 0, &val);
			  write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate, val.typed_val.type);
			  write_exp_elt_longcst (pstate,
						 (LONGEST)val.typed_val.val);
			  write_exp_elt_opcode (pstate, OP_LONG); }
	;

exp	:	FLOAT
			{ write_exp_elt_opcode (pstate, OP_FLOAT);
			  write_exp_elt_type (pstate, $1.type);
			  write_exp_elt_floatcst (pstate, $1.val);
			  write_exp_elt_opcode (pstate, OP_FLOAT); }
	;

exp	:	variable
	;

exp	:	DOLLAR_VARIABLE
	;

exp	:	SIZEOF '(' type ')'	%prec UNARY
			{ write_exp_elt_opcode (pstate, OP_LONG);
			  write_exp_elt_type (pstate,
					      parse_f_type (pstate)
					      ->builtin_integer);
			  $3 = check_typedef ($3);
			  write_exp_elt_longcst (pstate,
						 (LONGEST) TYPE_LENGTH ($3));
			  write_exp_elt_opcode (pstate, OP_LONG); }
	;

exp     :       BOOLEAN_LITERAL
			{ write_exp_elt_opcode (pstate, OP_BOOL);
			  write_exp_elt_longcst (pstate, (LONGEST) $1);
			  write_exp_elt_opcode (pstate, OP_BOOL);
			}
        ;

exp	:	STRING_LITERAL
			{
			  write_exp_elt_opcode (pstate, OP_STRING);
			  write_exp_string (pstate, $1);
			  write_exp_elt_opcode (pstate, OP_STRING);
			}
	;

variable:	name_not_typename
			{ struct block_symbol sym = $1.sym;

			  if (sym.symbol)
			    {
			      if (symbol_read_needs_frame (sym.symbol))
				pstate->block_tracker->update (sym);
			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      write_exp_elt_block (pstate, sym.block);
			      write_exp_elt_sym (pstate, sym.symbol);
			      write_exp_elt_opcode (pstate, OP_VAR_VALUE);
			      break;
			    }
			  else
			    {
			      struct bound_minimal_symbol msymbol;
			      std::string arg = copy_name ($1.stoken);

			      msymbol =
				lookup_bound_minimal_symbol (arg.c_str ());
			      if (msymbol.minsym != NULL)
				write_exp_msymbol (pstate, msymbol);
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error (_("No symbol table is loaded.  Use the \"file\" command."));
			      else
				error (_("No symbol \"%s\" in current context."),
				       arg.c_str ());
			    }
			}
	;


type    :       ptype
        ;

ptype	:	typebase
	|	typebase abs_decl
		{
		  /* This is where the interesting stuff happens.  */
		  int done = 0;
		  int array_size;
		  struct type *follow_type = $1;
		  struct type *range_type;
		  
		  while (!done)
		    switch (type_stack->pop ())
		      {
		      case tp_end:
			done = 1;
			break;
		      case tp_pointer:
			follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_reference:
			follow_type = lookup_lvalue_reference_type (follow_type);
			break;
		      case tp_array:
			array_size = type_stack->pop_int ();
			if (array_size != -1)
			  {
			    range_type =
			      create_static_range_type ((struct type *) NULL,
							parse_f_type (pstate)
							->builtin_integer,
							0, array_size - 1);
			    follow_type =
			      create_array_type ((struct type *) NULL,
						 follow_type, range_type);
			  }
			else
			  follow_type = lookup_pointer_type (follow_type);
			break;
		      case tp_function:
			follow_type = lookup_function_type (follow_type);
			break;
		      case tp_kind:
			{
			  int kind_val = type_stack->pop_int ();
			  follow_type
			    = convert_to_kind_type (follow_type, kind_val);
			}
			break;
		      }
		  $$ = follow_type;
		}
	;

abs_decl:	'*'
			{ type_stack->push (tp_pointer); $$ = 0; }
	|	'*' abs_decl
			{ type_stack->push (tp_pointer); $$ = $2; }
	|	'&'
			{ type_stack->push (tp_reference); $$ = 0; }
	|	'&' abs_decl
			{ type_stack->push (tp_reference); $$ = $2; }
	|	direct_abs_decl
	;

direct_abs_decl: '(' abs_decl ')'
			{ $$ = $2; }
	| 	'(' KIND '=' INT ')'
			{ push_kind_type ($4.val, $4.type); }
	|	'*' INT
			{ push_kind_type ($2.val, $2.type); }
	| 	direct_abs_decl func_mod
			{ type_stack->push (tp_function); }
	|	func_mod
			{ type_stack->push (tp_function); }
	;

func_mod:	'(' ')'
			{ $$ = 0; }
	|	'(' nonempty_typelist ')'
			{ free ($2); $$ = 0; }
	;

typebase  /* Implements (approximately): (type-qualifier)* type-specifier */
	:	TYPENAME
			{ $$ = $1.type; }
	|	INT_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_integer; }
	|	INT_S2_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_integer_s2; }
	|	CHARACTER 
			{ $$ = parse_f_type (pstate)->builtin_character; }
	|	LOGICAL_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_logical_s8; }
	|	LOGICAL_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_logical; }
	|	LOGICAL_S2_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_logical_s2; }
	|	LOGICAL_S1_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_logical_s1; }
	|	REAL_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_real; }
	|       REAL_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_real_s8; }
	|	REAL_S16_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_real_s16; }
	|	COMPLEX_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex_s8; }
	|	COMPLEX_S8_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex_s8; }
	|	COMPLEX_S16_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_complex_s16; }
	|	COMPLEX_S32_KEYWORD 
			{ $$ = parse_f_type (pstate)->builtin_complex_s32; }
	|	SINGLE PRECISION
			{ $$ = parse_f_type (pstate)->builtin_real;}
	|	DOUBLE PRECISION
			{ $$ = parse_f_type (pstate)->builtin_real_s8;}
	|	SINGLE COMPLEX_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex_s8;}
	|	DOUBLE COMPLEX_KEYWORD
			{ $$ = parse_f_type (pstate)->builtin_complex_s16;}
	;

nonempty_typelist
	:	type
		{ $$ = (struct type **) malloc (sizeof (struct type *) * 2);
		  $<ivec>$[0] = 1;	/* Number of types in vector */
		  $$[1] = $1;
		}
	|	nonempty_typelist ',' type
		{ int len = sizeof (struct type *) * (++($<ivec>1[0]) + 1);
		  $$ = (struct type **) realloc ((char *) $1, len);
		  $$[$<ivec>$[0]] = $3;
		}
	;

name	:	NAME
		{  $$ = $1.stoken; }
	;

name_not_typename :	NAME
/* These would be useful if name_not_typename was useful, but it is just
   a fake for "variable", so these cause reduce/reduce conflicts because
   the parser can't tell whether NAME_OR_INT is a name_not_typename (=variable,
   =exp) or just an exp.  If name_not_typename was ever used in an lvalue
   context where only a name could occur, this might be useful.
  	|	NAME_OR_INT
   */
	;

%%

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (struct parser_state *par_state,
	      const char *p, int len, int parsed_float, YYSTYPE *putithere)
{
  LONGEST n = 0;
  LONGEST prevn = 0;
  int c;
  int base = input_radix;
  int unsigned_p = 0;
  int long_p = 0;
  ULONGEST high_bit;
  struct type *signed_type;
  struct type *unsigned_type;

  if (parsed_float)
    {
      /* It's a float since it contains a point or an exponent.  */
      /* [dD] is not understood as an exponent by parse_float,
	 change it to 'e'.  */
      char *tmp, *tmp2;

      tmp = xstrdup (p);
      for (tmp2 = tmp; *tmp2; ++tmp2)
	if (*tmp2 == 'd' || *tmp2 == 'D')
	  *tmp2 = 'e';

      /* FIXME: Should this use different types?  */
      putithere->typed_val_float.type = parse_f_type (pstate)->builtin_real_s8;
      bool parsed = parse_float (tmp, len,
				 putithere->typed_val_float.type,
				 putithere->typed_val_float.val);
      free (tmp);
      return parsed? FLOAT : ERROR;
    }

  /* Handle base-switching prefixes 0x, 0t, 0d, 0 */
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
      if (isupper (c))
	c = tolower (c);
      if (len == 0 && c == 'l')
	long_p = 1;
      else if (len == 0 && c == 'u')
	unsigned_p = 1;
      else
	{
	  int i;
	  if (c >= '0' && c <= '9')
	    i = c - '0';
	  else if (c >= 'a' && c <= 'f')
	    i = c - 'a' + 10;
	  else
	    return ERROR;	/* Char not a digit */
	  if (i >= base)
	    return ERROR;		/* Invalid digit in this base */
	  n *= base;
	  n += i;
	}
      /* Portably test for overflow (only works for nonzero values, so make
	 a second check for zero).  */
      if ((prevn >= n) && n != 0)
	unsigned_p=1;		/* Try something unsigned */
      /* If range checking enabled, portably test for unsigned overflow.  */
      if (RANGE_CHECK && n != 0)
	{
	  if ((unsigned_p && (unsigned)prevn >= (unsigned)n))
	    range_error (_("Overflow on numeric constant."));
	}
      prevn = n;
    }
  
  /* If the number is too big to be an int, or it's got an l suffix
     then it's a long.  Work out if this has to be a long by
     shifting right and seeing if anything remains, and the
     target int size is different to the target long size.
     
     In the expression below, we could have tested
     (n >> gdbarch_int_bit (parse_gdbarch))
     to see if it was zero,
     but too many compilers warn about that, when ints and longs
     are the same size.  So we shift it twice, with fewer bits
     each time, for the same result.  */
  
  if ((gdbarch_int_bit (par_state->gdbarch ())
       != gdbarch_long_bit (par_state->gdbarch ())
       && ((n >> 2)
	   >> (gdbarch_int_bit (par_state->gdbarch ())-2))) /* Avoid
							    shift warning */
      || long_p)
    {
      high_bit = ((ULONGEST)1)
      << (gdbarch_long_bit (par_state->gdbarch ())-1);
      unsigned_type = parse_type (par_state)->builtin_unsigned_long;
      signed_type = parse_type (par_state)->builtin_long;
    }
  else 
    {
      high_bit =
	((ULONGEST)1) << (gdbarch_int_bit (par_state->gdbarch ()) - 1);
      unsigned_type = parse_type (par_state)->builtin_unsigned_int;
      signed_type = parse_type (par_state)->builtin_int;
    }    
  
  putithere->typed_val.val = n;
  
  /* If the high bit of the worked out type is set then this number
     has to be unsigned.  */
  
  if (unsigned_p || (n & high_bit)) 
    putithere->typed_val.type = unsigned_type;
  else 
    putithere->typed_val.type = signed_type;
  
  return INT;
}

/* Called to setup the type stack when we encounter a '(kind=N)' type
   modifier, performs some bounds checking on 'N' and then pushes this to
   the type stack followed by the 'tp_kind' marker.  */
static void
push_kind_type (LONGEST val, struct type *type)
{
  int ival;

  if (TYPE_UNSIGNED (type))
    {
      ULONGEST uval = static_cast <ULONGEST> (val);
      if (uval > INT_MAX)
	error (_("kind value out of range"));
      ival = static_cast <int> (uval);
    }
  else
    {
      if (val > INT_MAX || val < 0)
	error (_("kind value out of range"));
      ival = static_cast <int> (val);
    }

  type_stack->push (ival);
  type_stack->push (tp_kind);
}

/* Called when a type has a '(kind=N)' modifier after it, for example
   'character(kind=1)'.  The BASETYPE is the type described by 'character'
   in our example, and KIND is the integer '1'.  This function returns a
   new type that represents the basetype of a specific kind.  */
static struct type *
convert_to_kind_type (struct type *basetype, int kind)
{
  if (basetype == parse_f_type (pstate)->builtin_character)
    {
      /* Character of kind 1 is a special case, this is the same as the
	 base character type.  */
      if (kind == 1)
	return parse_f_type (pstate)->builtin_character;
    }
  else if (basetype == parse_f_type (pstate)->builtin_complex_s8)
    {
      if (kind == 4)
	return parse_f_type (pstate)->builtin_complex_s8;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_complex_s16;
      else if (kind == 16)
	return parse_f_type (pstate)->builtin_complex_s32;
    }
  else if (basetype == parse_f_type (pstate)->builtin_real)
    {
      if (kind == 4)
	return parse_f_type (pstate)->builtin_real;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_real_s8;
      else if (kind == 16)
	return parse_f_type (pstate)->builtin_real_s16;
    }
  else if (basetype == parse_f_type (pstate)->builtin_logical)
    {
      if (kind == 1)
	return parse_f_type (pstate)->builtin_logical_s1;
      else if (kind == 2)
	return parse_f_type (pstate)->builtin_logical_s2;
      else if (kind == 4)
	return parse_f_type (pstate)->builtin_logical;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_logical_s8;
    }
  else if (basetype == parse_f_type (pstate)->builtin_integer)
    {
      if (kind == 2)
	return parse_f_type (pstate)->builtin_integer_s2;
      else if (kind == 4)
	return parse_f_type (pstate)->builtin_integer;
      else if (kind == 8)
	return parse_f_type (pstate)->builtin_integer_s8;
    }

  error (_("unsupported kind %d for type %s"),
	 kind, TYPE_SAFE_NAME (basetype));

  /* Should never get here.  */
  return nullptr;
}

struct token
{
  /* The string to match against.  */
  const char *oper;

  /* The lexer token to return.  */
  int token;

  /* The expression opcode to embed within the token.  */
  enum exp_opcode opcode;

  /* When this is true the string in OPER is matched exactly including
     case, when this is false OPER is matched case insensitively.  */
  bool case_sensitive;
};

static const struct token dot_ops[] =
{
  { ".and.", BOOL_AND, BINOP_END, false },
  { ".or.", BOOL_OR, BINOP_END, false },
  { ".not.", BOOL_NOT, BINOP_END, false },
  { ".eq.", EQUAL, BINOP_END, false },
  { ".eqv.", EQUAL, BINOP_END, false },
  { ".neqv.", NOTEQUAL, BINOP_END, false },
  { ".ne.", NOTEQUAL, BINOP_END, false },
  { ".le.", LEQ, BINOP_END, false },
  { ".ge.", GEQ, BINOP_END, false },
  { ".gt.", GREATERTHAN, BINOP_END, false },
  { ".lt.", LESSTHAN, BINOP_END, false },
};

/* Holds the Fortran representation of a boolean, and the integer value we
   substitute in when one of the matching strings is parsed.  */
struct f77_boolean_val
{
  /* The string representing a Fortran boolean.  */
  const char *name;

  /* The integer value to replace it with.  */
  int value;
};

/* The set of Fortran booleans.  These are matched case insensitively.  */
static const struct f77_boolean_val boolean_values[]  =
{
  { ".true.", 1 },
  { ".false.", 0 }
};

static const struct token f77_keywords[] =
{
  /* Historically these have always been lowercase only in GDB.  */
  { "complex_16", COMPLEX_S16_KEYWORD, BINOP_END, true },
  { "complex_32", COMPLEX_S32_KEYWORD, BINOP_END, true },
  { "character", CHARACTER, BINOP_END, true },
  { "integer_2", INT_S2_KEYWORD, BINOP_END, true },
  { "logical_1", LOGICAL_S1_KEYWORD, BINOP_END, true },
  { "logical_2", LOGICAL_S2_KEYWORD, BINOP_END, true },
  { "logical_8", LOGICAL_S8_KEYWORD, BINOP_END, true },
  { "complex_8", COMPLEX_S8_KEYWORD, BINOP_END, true },
  { "integer", INT_KEYWORD, BINOP_END, true },
  { "logical", LOGICAL_KEYWORD, BINOP_END, true },
  { "real_16", REAL_S16_KEYWORD, BINOP_END, true },
  { "complex", COMPLEX_KEYWORD, BINOP_END, true },
  { "sizeof", SIZEOF, BINOP_END, true },
  { "real_8", REAL_S8_KEYWORD, BINOP_END, true },
  { "real", REAL_KEYWORD, BINOP_END, true },
  { "single", SINGLE, BINOP_END, true },
  { "double", DOUBLE, BINOP_END, true },
  { "precision", PRECISION, BINOP_END, true },
  /* The following correspond to actual functions in Fortran and are case
     insensitive.  */
  { "kind", KIND, BINOP_END, false },
  { "abs", UNOP_INTRINSIC, UNOP_ABS, false },
  { "mod", BINOP_INTRINSIC, BINOP_MOD, false },
  { "floor", UNOP_INTRINSIC, UNOP_FORTRAN_FLOOR, false },
  { "ceiling", UNOP_INTRINSIC, UNOP_FORTRAN_CEILING, false },
  { "modulo", BINOP_INTRINSIC, BINOP_FORTRAN_MODULO, false },
  { "cmplx", BINOP_INTRINSIC, BINOP_FORTRAN_CMPLX, false },
};

/* Implementation of a dynamically expandable buffer for processing input
   characters acquired through lexptr and building a value to return in
   yylval.  Ripped off from ch-exp.y */ 

static char *tempbuf;		/* Current buffer contents */
static int tempbufsize;		/* Size of allocated buffer */
static int tempbufindex;	/* Current index into buffer */

#define GROWBY_MIN_SIZE 64	/* Minimum amount to grow buffer by */

#define CHECKBUF(size) \
  do { \
    if (tempbufindex + (size) >= tempbufsize) \
      { \
	growbuf_by_size (size); \
      } \
  } while (0);


/* Grow the static temp buffer if necessary, including allocating the
   first one on demand.  */

static void
growbuf_by_size (int count)
{
  int growby;

  growby = std::max (count, GROWBY_MIN_SIZE);
  tempbufsize += growby;
  if (tempbuf == NULL)
    tempbuf = (char *) malloc (tempbufsize);
  else
    tempbuf = (char *) realloc (tempbuf, tempbufsize);
}

/* Blatantly ripped off from ch-exp.y. This routine recognizes F77 
   string-literals.
   
   Recognize a string literal.  A string literal is a nonzero sequence
   of characters enclosed in matching single quotes, except that
   a single character inside single quotes is a character literal, which
   we reject as a string literal.  To embed the terminator character inside
   a string, it is simply doubled (I.E. 'this''is''one''string') */

static int
match_string_literal (void)
{
  const char *tokptr = pstate->lexptr;

  for (tempbufindex = 0, tokptr++; *tokptr != '\0'; tokptr++)
    {
      CHECKBUF (1);
      if (*tokptr == *pstate->lexptr)
	{
	  if (*(tokptr + 1) == *pstate->lexptr)
	    tokptr++;
	  else
	    break;
	}
      tempbuf[tempbufindex++] = *tokptr;
    }
  if (*tokptr == '\0'					/* no terminator */
      || tempbufindex == 0)				/* no string */
    return 0;
  else
    {
      tempbuf[tempbufindex] = '\0';
      yylval.sval.ptr = tempbuf;
      yylval.sval.length = tempbufindex;
      pstate->lexptr = ++tokptr;
      return STRING_LITERAL;
    }
}

/* Read one token, getting characters through lexptr.  */

static int
yylex (void)
{
  int c;
  int namelen;
  unsigned int token;
  const char *tokstart;
  
 retry:
 
  pstate->prev_lexptr = pstate->lexptr;
 
  tokstart = pstate->lexptr;

  /* First of all, let us make sure we are not dealing with the
     special tokens .true. and .false. which evaluate to 1 and 0.  */

  if (*pstate->lexptr == '.')
    {
      for (int i = 0; i < ARRAY_SIZE (boolean_values); i++)
	{
	  if (strncasecmp (tokstart, boolean_values[i].name,
			   strlen (boolean_values[i].name)) == 0)
	    {
	      pstate->lexptr += strlen (boolean_values[i].name);
	      yylval.lval = boolean_values[i].value;
	      return BOOLEAN_LITERAL;
	    }
	}
    }

  /* See if it is a special .foo. operator.  */
  for (int i = 0; i < ARRAY_SIZE (dot_ops); i++)
    if (strncasecmp (tokstart, dot_ops[i].oper,
		     strlen (dot_ops[i].oper)) == 0)
      {
	gdb_assert (!dot_ops[i].case_sensitive);
	pstate->lexptr += strlen (dot_ops[i].oper);
	yylval.opcode = dot_ops[i].opcode;
	return dot_ops[i].token;
      }

  /* See if it is an exponentiation operator.  */

  if (strncmp (tokstart, "**", 2) == 0)
    {
      pstate->lexptr += 2;
      yylval.opcode = BINOP_EXP;
      return STARSTAR;
    }

  switch (c = *tokstart)
    {
    case 0:
      return 0;
      
    case ' ':
    case '\t':
    case '\n':
      pstate->lexptr++;
      goto retry;
      
    case '\'':
      token = match_string_literal ();
      if (token != 0)
	return (token);
      break;
      
    case '(':
      paren_depth++;
      pstate->lexptr++;
      return c;
      
    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      pstate->lexptr++;
      return c;
      
    case ',':
      if (pstate->comma_terminates && paren_depth == 0)
	return 0;
      pstate->lexptr++;
      return c;
      
    case '.':
      /* Might be a floating point number.  */
      if (pstate->lexptr[1] < '0' || pstate->lexptr[1] > '9')
	goto symbol;		/* Nope, must be a symbol.  */
      /* FALL THRU.  */
      
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
	int got_dot = 0, got_e = 0, got_d = 0, toktype;
	const char *p = tokstart;
	int hex = input_radix > 10;
	
	if (c == '0' && (p[1] == 'x' || p[1] == 'X'))
	  {
	    p += 2;
	    hex = 1;
	  }
	else if (c == '0' && (p[1]=='t' || p[1]=='T'
			      || p[1]=='d' || p[1]=='D'))
	  {
	    p += 2;
	    hex = 0;
	  }
	
	for (;; ++p)
	  {
	    if (!hex && !got_e && (*p == 'e' || *p == 'E'))
	      got_dot = got_e = 1;
	    else if (!hex && !got_d && (*p == 'd' || *p == 'D'))
	      got_dot = got_d = 1;
	    else if (!hex && !got_dot && *p == '.')
	      got_dot = 1;
	    else if (((got_e && (p[-1] == 'e' || p[-1] == 'E'))
		     || (got_d && (p[-1] == 'd' || p[-1] == 'D')))
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
	toktype = parse_number (pstate, tokstart, p - tokstart,
				got_dot|got_e|got_d,
				&yylval);
        if (toktype == ERROR)
          {
	    char *err_copy = (char *) alloca (p - tokstart + 1);
	    
	    memcpy (err_copy, tokstart, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error (_("Invalid number \"%s\"."), err_copy);
	  }
	pstate->lexptr = p;
	return toktype;
      }
      
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
    case '@':
    case '<':
    case '>':
    case '[':
    case ']':
    case '?':
    case ':':
    case '=':
    case '{':
    case '}':
    symbol:
      pstate->lexptr++;
      return c;
    }
  
  if (!(c == '_' || c == '$' || c ==':'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error (_("Invalid character '%c' in expression."), c);
  
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || c == ':' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')); 
       c = tokstart[++namelen]);
  
  /* The token "if" terminates the expression and is NOT 
     removed from the input stream.  */
  
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    return 0;
  
  pstate->lexptr += namelen;
  
  /* Catch specific keywords.  */

  for (int i = 0; i < ARRAY_SIZE (f77_keywords); i++)
    if (strlen (f77_keywords[i].oper) == namelen
	&& ((!f77_keywords[i].case_sensitive
	     && strncasecmp (tokstart, f77_keywords[i].oper, namelen) == 0)
	    || (f77_keywords[i].case_sensitive
		&& strncmp (tokstart, f77_keywords[i].oper, namelen) == 0)))
      {
	yylval.opcode = f77_keywords[i].opcode;
	return f77_keywords[i].token;
      }

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;
  
  if (*tokstart == '$')
    {
      write_dollar_variable (pstate, yylval.sval);
      return DOLLAR_VARIABLE;
    }
  
  /* Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
  {
    std::string tmp = copy_name (yylval.sval);
    struct block_symbol result;
    enum domain_enum_tag lookup_domains[] =
    {
      STRUCT_DOMAIN,
      VAR_DOMAIN,
      MODULE_DOMAIN
    };
    int hextype;

    for (int i = 0; i < ARRAY_SIZE (lookup_domains); ++i)
      {
	result = lookup_symbol (tmp.c_str (), pstate->expression_context_block,
				lookup_domains[i], NULL);
	if (result.symbol && SYMBOL_CLASS (result.symbol) == LOC_TYPEDEF)
	  {
	    yylval.tsym.type = SYMBOL_TYPE (result.symbol);
	    return TYPENAME;
	  }

	if (result.symbol)
	  break;
      }

    yylval.tsym.type
      = language_lookup_primitive_type (pstate->language (),
					pstate->gdbarch (), tmp.c_str ());
    if (yylval.tsym.type != NULL)
      return TYPENAME;
    
    /* Input names that aren't symbols but ARE valid hex numbers,
       when the input radix permits them, can be names or numbers
       depending on the parse.  Note we support radixes > 16 here.  */
    if (!result.symbol
	&& ((tokstart[0] >= 'a' && tokstart[0] < 'a' + input_radix - 10)
	    || (tokstart[0] >= 'A' && tokstart[0] < 'A' + input_radix - 10)))
      {
 	YYSTYPE newlval;	/* Its value is ignored.  */
	hextype = parse_number (pstate, tokstart, namelen, 0, &newlval);
	if (hextype == INT)
	  {
	    yylval.ssym.sym = result;
	    yylval.ssym.is_a_field_of_this = false;
	    return NAME_OR_INT;
	  }
      }
    
    /* Any other kind of symbol */
    yylval.ssym.sym = result;
    yylval.ssym.is_a_field_of_this = false;
    return NAME;
  }
}

int
f_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  scoped_restore restore_yydebug = make_scoped_restore (&yydebug,
							parser_debug);
  gdb_assert (par_state != NULL);
  pstate = par_state;
  paren_depth = 0;

  struct type_stack stack;
  scoped_restore restore_type_stack = make_scoped_restore (&type_stack,
							   &stack);

  return yyparse ();
}

static void
yyerror (const char *msg)
{
  if (pstate->prev_lexptr)
    pstate->lexptr = pstate->prev_lexptr;

  error (_("A %s in expression, near `%s'."), msg, pstate->lexptr);
}
