// OBSOLETE /* Parser for GNU CHILL (CCITT High-Level Language)  -*- C -*-
// OBSOLETE    Copyright 1992, 1993, 1995, 1996, 1997, 1999, 2000, 2001
// OBSOLETE    Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE    This file is part of GDB.
// OBSOLETE 
// OBSOLETE    This program is free software; you can redistribute it and/or modify
// OBSOLETE    it under the terms of the GNU General Public License as published by
// OBSOLETE    the Free Software Foundation; either version 2 of the License, or
// OBSOLETE    (at your option) any later version.
// OBSOLETE 
// OBSOLETE    This program is distributed in the hope that it will be useful,
// OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE    GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE    You should have received a copy of the GNU General Public License
// OBSOLETE    along with this program; if not, write to the Free Software
// OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330,
// OBSOLETE    Boston, MA 02111-1307, USA.  */
// OBSOLETE 
// OBSOLETE /* Parse a Chill expression from text in a string,
// OBSOLETE    and return the result as a  struct expression  pointer.
// OBSOLETE    That structure contains arithmetic operations in reverse polish,
// OBSOLETE    with constants represented by operations that are followed by special data.
// OBSOLETE    See expression.h for the details of the format.
// OBSOLETE    What is important here is that it can be built up sequentially
// OBSOLETE    during the process of parsing; the lower levels of the tree always
// OBSOLETE    come first in the result.
// OBSOLETE 
// OBSOLETE    Note that the language accepted by this parser is more liberal
// OBSOLETE    than the one accepted by an actual Chill compiler.  For example, the
// OBSOLETE    language rule that a simple name string can not be one of the reserved
// OBSOLETE    simple name strings is not enforced (e.g "case" is not treated as a
// OBSOLETE    reserved name).  Another example is that Chill is a strongly typed
// OBSOLETE    language, and certain expressions that violate the type constraints
// OBSOLETE    may still be evaluated if gdb can do so in a meaningful manner, while
// OBSOLETE    such expressions would be rejected by the compiler.  The reason for
// OBSOLETE    this more liberal behavior is the philosophy that the debugger
// OBSOLETE    is intended to be a tool that is used by the programmer when things
// OBSOLETE    go wrong, and as such, it should provide as few artificial barriers
// OBSOLETE    to it's use as possible.  If it can do something meaningful, even
// OBSOLETE    something that violates language contraints that are enforced by the
// OBSOLETE    compiler, it should do so without complaint.
// OBSOLETE 
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE #include "defs.h"
// OBSOLETE #include "gdb_string.h"
// OBSOLETE #include <ctype.h>
// OBSOLETE #include "expression.h"
// OBSOLETE #include "language.h"
// OBSOLETE #include "value.h"
// OBSOLETE #include "parser-defs.h"
// OBSOLETE #include "ch-lang.h"
// OBSOLETE #include "bfd.h"		/* Required by objfiles.h.  */
// OBSOLETE #include "symfile.h"		/* Required by objfiles.h.  */
// OBSOLETE #include "objfiles.h"		/* For have_full_symbols and have_partial_symbols */
// OBSOLETE 
// OBSOLETE #ifdef __GNUC__
// OBSOLETE #define INLINE __inline__
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE typedef union
// OBSOLETE 
// OBSOLETE   {
// OBSOLETE     LONGEST lval;
// OBSOLETE     ULONGEST ulval;
// OBSOLETE     struct
// OBSOLETE       {
// OBSOLETE 	LONGEST val;
// OBSOLETE 	struct type *type;
// OBSOLETE       }
// OBSOLETE     typed_val;
// OBSOLETE     double dval;
// OBSOLETE     struct symbol *sym;
// OBSOLETE     struct type *tval;
// OBSOLETE     struct stoken sval;
// OBSOLETE     struct ttype tsym;
// OBSOLETE     struct symtoken ssym;
// OBSOLETE   }
// OBSOLETE YYSTYPE;
// OBSOLETE 
// OBSOLETE enum ch_terminal
// OBSOLETE   {
// OBSOLETE     END_TOKEN = 0,
// OBSOLETE     /* '\001' ... '\xff' come first. */
// OBSOLETE     OPEN_PAREN = '(',
// OBSOLETE     TOKEN_NOT_READ = 999,
// OBSOLETE     INTEGER_LITERAL,
// OBSOLETE     BOOLEAN_LITERAL,
// OBSOLETE     CHARACTER_LITERAL,
// OBSOLETE     FLOAT_LITERAL,
// OBSOLETE     GENERAL_PROCEDURE_NAME,
// OBSOLETE     LOCATION_NAME,
// OBSOLETE     EMPTINESS_LITERAL,
// OBSOLETE     CHARACTER_STRING_LITERAL,
// OBSOLETE     BIT_STRING_LITERAL,
// OBSOLETE     TYPENAME,
// OBSOLETE     DOT_FIELD_NAME,		/* '.' followed by <field name> */
// OBSOLETE     CASE,
// OBSOLETE     OF,
// OBSOLETE     ESAC,
// OBSOLETE     LOGIOR,
// OBSOLETE     ORIF,
// OBSOLETE     LOGXOR,
// OBSOLETE     LOGAND,
// OBSOLETE     ANDIF,
// OBSOLETE     NOTEQUAL,
// OBSOLETE     GEQ,
// OBSOLETE     LEQ,
// OBSOLETE     IN,
// OBSOLETE     SLASH_SLASH,
// OBSOLETE     MOD,
// OBSOLETE     REM,
// OBSOLETE     NOT,
// OBSOLETE     POINTER,
// OBSOLETE     RECEIVE,
// OBSOLETE     UP,
// OBSOLETE     IF,
// OBSOLETE     THEN,
// OBSOLETE     ELSE,
// OBSOLETE     FI,
// OBSOLETE     ELSIF,
// OBSOLETE     ILLEGAL_TOKEN,
// OBSOLETE     NUM,
// OBSOLETE     PRED,
// OBSOLETE     SUCC,
// OBSOLETE     ABS,
// OBSOLETE     CARD,
// OBSOLETE     MAX_TOKEN,
// OBSOLETE     MIN_TOKEN,
// OBSOLETE     ADDR_TOKEN,
// OBSOLETE     SIZE,
// OBSOLETE     UPPER,
// OBSOLETE     LOWER,
// OBSOLETE     LENGTH,
// OBSOLETE     ARRAY,
// OBSOLETE     GDB_VARIABLE,
// OBSOLETE     GDB_ASSIGNMENT
// OBSOLETE   };
// OBSOLETE 
// OBSOLETE /* Forward declarations. */
// OBSOLETE 
// OBSOLETE static void write_lower_upper_value (enum exp_opcode, struct type *);
// OBSOLETE static enum ch_terminal match_bitstring_literal (void);
// OBSOLETE static enum ch_terminal match_integer_literal (void);
// OBSOLETE static enum ch_terminal match_character_literal (void);
// OBSOLETE static enum ch_terminal match_string_literal (void);
// OBSOLETE static enum ch_terminal match_float_literal (void);
// OBSOLETE static int decode_integer_literal (LONGEST *, char **);
// OBSOLETE static int decode_integer_value (int, char **, LONGEST *);
// OBSOLETE static char *match_simple_name_string (void);
// OBSOLETE static void growbuf_by_size (int);
// OBSOLETE static void parse_case_label (void);
// OBSOLETE static void parse_untyped_expr (void);
// OBSOLETE static void parse_if_expression (void);
// OBSOLETE static void parse_if_expression_body (void);
// OBSOLETE static void parse_else_alternative (void);
// OBSOLETE static void parse_then_alternative (void);
// OBSOLETE static void parse_expr (void);
// OBSOLETE static void parse_operand0 (void);
// OBSOLETE static void parse_operand1 (void);
// OBSOLETE static void parse_operand2 (void);
// OBSOLETE static void parse_operand3 (void);
// OBSOLETE static void parse_operand4 (void);
// OBSOLETE static void parse_operand5 (void);
// OBSOLETE static void parse_operand6 (void);
// OBSOLETE static void parse_primval (void);
// OBSOLETE static void parse_tuple (struct type *);
// OBSOLETE static void parse_opt_element_list (struct type *);
// OBSOLETE static void parse_tuple_element (struct type *);
// OBSOLETE static void parse_named_record_element (void);
// OBSOLETE static void parse_call (void);
// OBSOLETE static struct type *parse_mode_or_normal_call (void);
// OBSOLETE #if 0
// OBSOLETE static struct type *parse_mode_call (void);
// OBSOLETE #endif
// OBSOLETE static void parse_unary_call (void);
// OBSOLETE static int parse_opt_untyped_expr (void);
// OBSOLETE static int expect (enum ch_terminal, char *);
// OBSOLETE static enum ch_terminal ch_lex (void);
// OBSOLETE INLINE static enum ch_terminal PEEK_TOKEN (void);
// OBSOLETE static enum ch_terminal peek_token_ (int);
// OBSOLETE static void forward_token_ (void);
// OBSOLETE static void require (enum ch_terminal);
// OBSOLETE static int check_token (enum ch_terminal);
// OBSOLETE 
// OBSOLETE #define MAX_LOOK_AHEAD 2
// OBSOLETE static enum ch_terminal terminal_buffer[MAX_LOOK_AHEAD + 1] =
// OBSOLETE {
// OBSOLETE   TOKEN_NOT_READ, TOKEN_NOT_READ, TOKEN_NOT_READ};
// OBSOLETE static YYSTYPE yylval;
// OBSOLETE static YYSTYPE val_buffer[MAX_LOOK_AHEAD + 1];
// OBSOLETE 
// OBSOLETE /*int current_token, lookahead_token; */
// OBSOLETE 
// OBSOLETE INLINE static enum ch_terminal
// OBSOLETE PEEK_TOKEN (void)
// OBSOLETE {
// OBSOLETE   if (terminal_buffer[0] == TOKEN_NOT_READ)
// OBSOLETE     {
// OBSOLETE       terminal_buffer[0] = ch_lex ();
// OBSOLETE       val_buffer[0] = yylval;
// OBSOLETE     }
// OBSOLETE   return terminal_buffer[0];
// OBSOLETE }
// OBSOLETE #define PEEK_LVAL() val_buffer[0]
// OBSOLETE #define PEEK_TOKEN1() peek_token_(1)
// OBSOLETE #define PEEK_TOKEN2() peek_token_(2)
// OBSOLETE static enum ch_terminal
// OBSOLETE peek_token_ (int i)
// OBSOLETE {
// OBSOLETE   if (i > MAX_LOOK_AHEAD)
// OBSOLETE     internal_error (__FILE__, __LINE__,
// OBSOLETE 		    "too much lookahead");
// OBSOLETE   if (terminal_buffer[i] == TOKEN_NOT_READ)
// OBSOLETE     {
// OBSOLETE       terminal_buffer[i] = ch_lex ();
// OBSOLETE       val_buffer[i] = yylval;
// OBSOLETE     }
// OBSOLETE   return terminal_buffer[i];
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE pushback_token (enum ch_terminal code, YYSTYPE node)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   if (terminal_buffer[MAX_LOOK_AHEAD] != TOKEN_NOT_READ)
// OBSOLETE     internal_error (__FILE__, __LINE__,
// OBSOLETE 		    "cannot pushback token");
// OBSOLETE   for (i = MAX_LOOK_AHEAD; i > 0; i--)
// OBSOLETE     {
// OBSOLETE       terminal_buffer[i] = terminal_buffer[i - 1];
// OBSOLETE       val_buffer[i] = val_buffer[i - 1];
// OBSOLETE     }
// OBSOLETE   terminal_buffer[0] = code;
// OBSOLETE   val_buffer[0] = node;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE forward_token_ (void)
// OBSOLETE {
// OBSOLETE   int i;
// OBSOLETE   for (i = 0; i < MAX_LOOK_AHEAD; i++)
// OBSOLETE     {
// OBSOLETE       terminal_buffer[i] = terminal_buffer[i + 1];
// OBSOLETE       val_buffer[i] = val_buffer[i + 1];
// OBSOLETE     }
// OBSOLETE   terminal_buffer[MAX_LOOK_AHEAD] = TOKEN_NOT_READ;
// OBSOLETE }
// OBSOLETE #define FORWARD_TOKEN() forward_token_()
// OBSOLETE 
// OBSOLETE /* Skip the next token.
// OBSOLETE    if it isn't TOKEN, the parser is broken. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE require (enum ch_terminal token)
// OBSOLETE {
// OBSOLETE   if (PEEK_TOKEN () != token)
// OBSOLETE     {
// OBSOLETE       internal_error (__FILE__, __LINE__,
// OBSOLETE 		      "expected token %d", (int) token);
// OBSOLETE     }
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE check_token (enum ch_terminal token)
// OBSOLETE {
// OBSOLETE   if (PEEK_TOKEN () != token)
// OBSOLETE     return 0;
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* return 0 if expected token was not found,
// OBSOLETE    else return 1.
// OBSOLETE  */
// OBSOLETE static int
// OBSOLETE expect (enum ch_terminal token, char *message)
// OBSOLETE {
// OBSOLETE   if (PEEK_TOKEN () != token)
// OBSOLETE     {
// OBSOLETE       if (message)
// OBSOLETE 	error (message);
// OBSOLETE       else if (token < 256)
// OBSOLETE 	error ("syntax error - expected a '%c' here \"%s\"", token, lexptr);
// OBSOLETE       else
// OBSOLETE 	error ("syntax error");
// OBSOLETE       return 0;
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     FORWARD_TOKEN ();
// OBSOLETE   return 1;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE /* Parse a name string.  If ALLOW_ALL is 1, ALL is allowed as a postfix. */
// OBSOLETE 
// OBSOLETE static tree
// OBSOLETE parse_opt_name_string (int allow_all)
// OBSOLETE {
// OBSOLETE   int token = PEEK_TOKEN ();
// OBSOLETE   tree name;
// OBSOLETE   if (token != NAME)
// OBSOLETE     {
// OBSOLETE       if (token == ALL && allow_all)
// OBSOLETE 	{
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  return ALL_POSTFIX;
// OBSOLETE 	}
// OBSOLETE       return NULL_TREE;
// OBSOLETE     }
// OBSOLETE   name = PEEK_LVAL ();
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       token = PEEK_TOKEN ();
// OBSOLETE       if (token != '!')
// OBSOLETE 	return name;
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       token = PEEK_TOKEN ();
// OBSOLETE       if (token == ALL && allow_all)
// OBSOLETE 	return get_identifier3 (IDENTIFIER_POINTER (name), "!", "*");
// OBSOLETE       if (token != NAME)
// OBSOLETE 	{
// OBSOLETE 	  if (pass == 1)
// OBSOLETE 	    error ("'%s!' is not followed by an identifier",
// OBSOLETE 		   IDENTIFIER_POINTER (name));
// OBSOLETE 	  return name;
// OBSOLETE 	}
// OBSOLETE       name = get_identifier3 (IDENTIFIER_POINTER (name),
// OBSOLETE 			      "!", IDENTIFIER_POINTER (PEEK_LVAL ()));
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static tree
// OBSOLETE parse_simple_name_string (void)
// OBSOLETE {
// OBSOLETE   int token = PEEK_TOKEN ();
// OBSOLETE   tree name;
// OBSOLETE   if (token != NAME)
// OBSOLETE     {
// OBSOLETE       error ("expected a name here");
// OBSOLETE       return error_mark_node;
// OBSOLETE     }
// OBSOLETE   name = PEEK_LVAL ();
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE   return name;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static tree
// OBSOLETE parse_name_string (void)
// OBSOLETE {
// OBSOLETE   tree name = parse_opt_name_string (0);
// OBSOLETE   if (name)
// OBSOLETE     return name;
// OBSOLETE   if (pass == 1)
// OBSOLETE     error ("expected a name string here");
// OBSOLETE   return error_mark_node;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Matches: <name_string>
// OBSOLETE    Returns if pass 1: the identifier.
// OBSOLETE    Returns if pass 2: a decl or value for identifier. */
// OBSOLETE 
// OBSOLETE static tree
// OBSOLETE parse_name (void)
// OBSOLETE {
// OBSOLETE   tree name = parse_name_string ();
// OBSOLETE   if (pass == 1 || ignoring)
// OBSOLETE     return name;
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       tree decl = lookup_name (name);
// OBSOLETE       if (decl == NULL_TREE)
// OBSOLETE 	{
// OBSOLETE 	  error ("`%s' undeclared", IDENTIFIER_POINTER (name));
// OBSOLETE 	  return error_mark_node;
// OBSOLETE 	}
// OBSOLETE       else if (TREE_CODE (TREE_TYPE (decl)) == ERROR_MARK)
// OBSOLETE 	return error_mark_node;
// OBSOLETE       else if (TREE_CODE (decl) == CONST_DECL)
// OBSOLETE 	return DECL_INITIAL (decl);
// OBSOLETE       else if (TREE_CODE (TREE_TYPE (decl)) == REFERENCE_TYPE)
// OBSOLETE 	return convert_from_reference (decl);
// OBSOLETE       else
// OBSOLETE 	return decl;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE static void
// OBSOLETE pushback_paren_expr (tree expr)
// OBSOLETE {
// OBSOLETE   if (pass == 1 && !ignoring)
// OBSOLETE     expr = build1 (PAREN_EXPR, NULL_TREE, expr);
// OBSOLETE   pushback_token (EXPR, expr);
// OBSOLETE }
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE /* Matches: <case label> */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_case_label (void)
// OBSOLETE {
// OBSOLETE   if (check_token (ELSE))
// OBSOLETE     error ("ELSE in tuples labels not implemented");
// OBSOLETE   /* Does not handle the case of a mode name.  FIXME */
// OBSOLETE   parse_expr ();
// OBSOLETE   if (check_token (':'))
// OBSOLETE     {
// OBSOLETE       parse_expr ();
// OBSOLETE       write_exp_elt_opcode (BINOP_RANGE);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE parse_opt_untyped_expr (void)
// OBSOLETE {
// OBSOLETE   switch (PEEK_TOKEN ())
// OBSOLETE     {
// OBSOLETE     case ',':
// OBSOLETE     case ':':
// OBSOLETE     case ')':
// OBSOLETE       return 0;
// OBSOLETE     default:
// OBSOLETE       parse_untyped_expr ();
// OBSOLETE       return 1;
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_unary_call (void)
// OBSOLETE {
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE   expect ('(', NULL);
// OBSOLETE   parse_expr ();
// OBSOLETE   expect (')', NULL);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Parse NAME '(' MODENAME ')'. */
// OBSOLETE 
// OBSOLETE #if 0
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE parse_mode_call (void)
// OBSOLETE {
// OBSOLETE   struct type *type;
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE   expect ('(', NULL);
// OBSOLETE   if (PEEK_TOKEN () != TYPENAME)
// OBSOLETE     error ("expect MODENAME here `%s'", lexptr);
// OBSOLETE   type = PEEK_LVAL ().tsym.type;
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE   expect (')', NULL);
// OBSOLETE   return type;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE static struct type *
// OBSOLETE parse_mode_or_normal_call (void)
// OBSOLETE {
// OBSOLETE   struct type *type;
// OBSOLETE   FORWARD_TOKEN ();
// OBSOLETE   expect ('(', NULL);
// OBSOLETE   if (PEEK_TOKEN () == TYPENAME)
// OBSOLETE     {
// OBSOLETE       type = PEEK_LVAL ().tsym.type;
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       parse_expr ();
// OBSOLETE       type = NULL;
// OBSOLETE     }
// OBSOLETE   expect (')', NULL);
// OBSOLETE   return type;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Parse something that looks like a function call.
// OBSOLETE    Assume we have parsed the function, and are at the '('. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_call (void)
// OBSOLETE {
// OBSOLETE   int arg_count;
// OBSOLETE   require ('(');
// OBSOLETE   /* This is to save the value of arglist_len
// OBSOLETE      being accumulated for each dimension. */
// OBSOLETE   start_arglist ();
// OBSOLETE   if (parse_opt_untyped_expr ())
// OBSOLETE     {
// OBSOLETE       int tok = PEEK_TOKEN ();
// OBSOLETE       arglist_len = 1;
// OBSOLETE       if (tok == UP || tok == ':')
// OBSOLETE 	{
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  parse_expr ();
// OBSOLETE 	  expect (')', "expected ')' to terminate slice");
// OBSOLETE 	  end_arglist ();
// OBSOLETE 	  write_exp_elt_opcode (tok == UP ? TERNOP_SLICE_COUNT
// OBSOLETE 				: TERNOP_SLICE);
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       while (check_token (','))
// OBSOLETE 	{
// OBSOLETE 	  parse_untyped_expr ();
// OBSOLETE 	  arglist_len++;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     arglist_len = 0;
// OBSOLETE   expect (')', NULL);
// OBSOLETE   arg_count = end_arglist ();
// OBSOLETE   write_exp_elt_opcode (MULTI_SUBSCRIPT);
// OBSOLETE   write_exp_elt_longcst (arg_count);
// OBSOLETE   write_exp_elt_opcode (MULTI_SUBSCRIPT);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_named_record_element (void)
// OBSOLETE {
// OBSOLETE   struct stoken label;
// OBSOLETE   char buf[256];
// OBSOLETE 
// OBSOLETE   label = PEEK_LVAL ().sval;
// OBSOLETE   sprintf (buf, "expected a field name here `%s'", lexptr);
// OBSOLETE   expect (DOT_FIELD_NAME, buf);
// OBSOLETE   if (check_token (','))
// OBSOLETE     parse_named_record_element ();
// OBSOLETE   else if (check_token (':'))
// OBSOLETE     parse_expr ();
// OBSOLETE   else
// OBSOLETE     error ("syntax error near `%s' in named record tuple element", lexptr);
// OBSOLETE   write_exp_elt_opcode (OP_LABELED);
// OBSOLETE   write_exp_string (label);
// OBSOLETE   write_exp_elt_opcode (OP_LABELED);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Returns one or more TREE_LIST nodes, in reverse order. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_tuple_element (struct type *type)
// OBSOLETE {
// OBSOLETE   if (PEEK_TOKEN () == DOT_FIELD_NAME)
// OBSOLETE     {
// OBSOLETE       /* Parse a labelled structure tuple. */
// OBSOLETE       parse_named_record_element ();
// OBSOLETE       return;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   if (check_token ('('))
// OBSOLETE     {
// OBSOLETE       if (check_token ('*'))
// OBSOLETE 	{
// OBSOLETE 	  expect (')', "missing ')' after '*' case label list");
// OBSOLETE 	  if (type)
// OBSOLETE 	    {
// OBSOLETE 	      if (TYPE_CODE (type) == TYPE_CODE_ARRAY)
// OBSOLETE 		{
// OBSOLETE 		  /* do this as a range from low to high */
// OBSOLETE 		  struct type *range_type = TYPE_FIELD_TYPE (type, 0);
// OBSOLETE 		  LONGEST low_bound, high_bound;
// OBSOLETE 		  if (get_discrete_bounds (range_type, &low_bound, &high_bound) < 0)
// OBSOLETE 		    error ("cannot determine bounds for (*)");
// OBSOLETE 		  /* lower bound */
// OBSOLETE 		  write_exp_elt_opcode (OP_LONG);
// OBSOLETE 		  write_exp_elt_type (range_type);
// OBSOLETE 		  write_exp_elt_longcst (low_bound);
// OBSOLETE 		  write_exp_elt_opcode (OP_LONG);
// OBSOLETE 		  /* upper bound */
// OBSOLETE 		  write_exp_elt_opcode (OP_LONG);
// OBSOLETE 		  write_exp_elt_type (range_type);
// OBSOLETE 		  write_exp_elt_longcst (high_bound);
// OBSOLETE 		  write_exp_elt_opcode (OP_LONG);
// OBSOLETE 		  write_exp_elt_opcode (BINOP_RANGE);
// OBSOLETE 		}
// OBSOLETE 	      else
// OBSOLETE 		error ("(*) in invalid context");
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    error ("(*) only possible with modename in front of tuple (mode[..])");
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  parse_case_label ();
// OBSOLETE 	  while (check_token (','))
// OBSOLETE 	    {
// OBSOLETE 	      parse_case_label ();
// OBSOLETE 	      write_exp_elt_opcode (BINOP_COMMA);
// OBSOLETE 	    }
// OBSOLETE 	  expect (')', NULL);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     parse_untyped_expr ();
// OBSOLETE   if (check_token (':'))
// OBSOLETE     {
// OBSOLETE       /* A powerset range or a labeled Array. */
// OBSOLETE       parse_untyped_expr ();
// OBSOLETE       write_exp_elt_opcode (BINOP_RANGE);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Matches:  a COMMA-separated list of tuple elements.
// OBSOLETE    Returns a list (of TREE_LIST nodes). */
// OBSOLETE static void
// OBSOLETE parse_opt_element_list (struct type *type)
// OBSOLETE {
// OBSOLETE   arglist_len = 0;
// OBSOLETE   if (PEEK_TOKEN () == ']')
// OBSOLETE     return;
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       parse_tuple_element (type);
// OBSOLETE       arglist_len++;
// OBSOLETE       if (PEEK_TOKEN () == ']')
// OBSOLETE 	break;
// OBSOLETE       if (!check_token (','))
// OBSOLETE 	error ("bad syntax in tuple");
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Parses: '[' elements ']'
// OBSOLETE    If modename is non-NULL it prefixed the tuple.  */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_tuple (struct type *mode)
// OBSOLETE {
// OBSOLETE   struct type *type;
// OBSOLETE   if (mode)
// OBSOLETE     type = check_typedef (mode);
// OBSOLETE   else
// OBSOLETE     type = 0;
// OBSOLETE   require ('[');
// OBSOLETE   start_arglist ();
// OBSOLETE   parse_opt_element_list (type);
// OBSOLETE   expect (']', "missing ']' after tuple");
// OBSOLETE   write_exp_elt_opcode (OP_ARRAY);
// OBSOLETE   write_exp_elt_longcst ((LONGEST) 0);
// OBSOLETE   write_exp_elt_longcst ((LONGEST) end_arglist () - 1);
// OBSOLETE   write_exp_elt_opcode (OP_ARRAY);
// OBSOLETE   if (type)
// OBSOLETE     {
// OBSOLETE       if (TYPE_CODE (type) != TYPE_CODE_ARRAY
// OBSOLETE 	  && TYPE_CODE (type) != TYPE_CODE_STRUCT
// OBSOLETE 	  && TYPE_CODE (type) != TYPE_CODE_SET)
// OBSOLETE 	error ("invalid tuple mode");
// OBSOLETE       write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE       write_exp_elt_type (mode);
// OBSOLETE       write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_primval (void)
// OBSOLETE {
// OBSOLETE   struct type *type;
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   char *op_name;
// OBSOLETE   switch (PEEK_TOKEN ())
// OBSOLETE     {
// OBSOLETE     case INTEGER_LITERAL:
// OBSOLETE     case CHARACTER_LITERAL:
// OBSOLETE       write_exp_elt_opcode (OP_LONG);
// OBSOLETE       write_exp_elt_type (PEEK_LVAL ().typed_val.type);
// OBSOLETE       write_exp_elt_longcst (PEEK_LVAL ().typed_val.val);
// OBSOLETE       write_exp_elt_opcode (OP_LONG);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case BOOLEAN_LITERAL:
// OBSOLETE       write_exp_elt_opcode (OP_BOOL);
// OBSOLETE       write_exp_elt_longcst ((LONGEST) PEEK_LVAL ().ulval);
// OBSOLETE       write_exp_elt_opcode (OP_BOOL);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case FLOAT_LITERAL:
// OBSOLETE       write_exp_elt_opcode (OP_DOUBLE);
// OBSOLETE       write_exp_elt_type (builtin_type_double);
// OBSOLETE       write_exp_elt_dblcst (PEEK_LVAL ().dval);
// OBSOLETE       write_exp_elt_opcode (OP_DOUBLE);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case EMPTINESS_LITERAL:
// OBSOLETE       write_exp_elt_opcode (OP_LONG);
// OBSOLETE       write_exp_elt_type (lookup_pointer_type (builtin_type_void));
// OBSOLETE       write_exp_elt_longcst (0);
// OBSOLETE       write_exp_elt_opcode (OP_LONG);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case CHARACTER_STRING_LITERAL:
// OBSOLETE       write_exp_elt_opcode (OP_STRING);
// OBSOLETE       write_exp_string (PEEK_LVAL ().sval);
// OBSOLETE       write_exp_elt_opcode (OP_STRING);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case BIT_STRING_LITERAL:
// OBSOLETE       write_exp_elt_opcode (OP_BITSTRING);
// OBSOLETE       write_exp_bitstring (PEEK_LVAL ().sval);
// OBSOLETE       write_exp_elt_opcode (OP_BITSTRING);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case ARRAY:
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       /* This is pseudo-Chill, similar to C's '(TYPE[])EXPR'
// OBSOLETE          which casts to an artificial array. */
// OBSOLETE       expect ('(', NULL);
// OBSOLETE       expect (')', NULL);
// OBSOLETE       if (PEEK_TOKEN () != TYPENAME)
// OBSOLETE 	error ("missing MODENAME after ARRAY()");
// OBSOLETE       type = PEEK_LVAL ().tsym.type;
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       expect ('(', NULL);
// OBSOLETE       parse_expr ();
// OBSOLETE       expect (')', "missing right parenthesis");
// OBSOLETE       type = create_array_type ((struct type *) NULL, type,
// OBSOLETE 				create_range_type ((struct type *) NULL,
// OBSOLETE 						   builtin_type_int, 0, 0));
// OBSOLETE       TYPE_ARRAY_UPPER_BOUND_TYPE (type) = BOUND_CANNOT_BE_DETERMINED;
// OBSOLETE       write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE       write_exp_elt_type (type);
// OBSOLETE       write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE       break;
// OBSOLETE #if 0
// OBSOLETE     case CONST:
// OBSOLETE     case EXPR:
// OBSOLETE       val = PEEK_LVAL ();
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE #endif
// OBSOLETE     case '(':
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       parse_expr ();
// OBSOLETE       expect (')', "missing right parenthesis");
// OBSOLETE       break;
// OBSOLETE     case '[':
// OBSOLETE       parse_tuple (NULL);
// OBSOLETE       break;
// OBSOLETE     case GENERAL_PROCEDURE_NAME:
// OBSOLETE     case LOCATION_NAME:
// OBSOLETE       write_exp_elt_opcode (OP_VAR_VALUE);
// OBSOLETE       write_exp_elt_block (NULL);
// OBSOLETE       write_exp_elt_sym (PEEK_LVAL ().ssym.sym);
// OBSOLETE       write_exp_elt_opcode (OP_VAR_VALUE);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case GDB_VARIABLE:		/* gdb specific */
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       break;
// OBSOLETE     case NUM:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE       write_exp_elt_type (builtin_type_int);
// OBSOLETE       write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE       break;
// OBSOLETE     case CARD:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       write_exp_elt_opcode (UNOP_CARD);
// OBSOLETE       break;
// OBSOLETE     case MAX_TOKEN:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       write_exp_elt_opcode (UNOP_CHMAX);
// OBSOLETE       break;
// OBSOLETE     case MIN_TOKEN:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       write_exp_elt_opcode (UNOP_CHMIN);
// OBSOLETE       break;
// OBSOLETE     case PRED:
// OBSOLETE       op_name = "PRED";
// OBSOLETE       goto unimplemented_unary_builtin;
// OBSOLETE     case SUCC:
// OBSOLETE       op_name = "SUCC";
// OBSOLETE       goto unimplemented_unary_builtin;
// OBSOLETE     case ABS:
// OBSOLETE       op_name = "ABS";
// OBSOLETE       goto unimplemented_unary_builtin;
// OBSOLETE     unimplemented_unary_builtin:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       error ("not implemented:  %s builtin function", op_name);
// OBSOLETE       break;
// OBSOLETE     case ADDR_TOKEN:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       write_exp_elt_opcode (UNOP_ADDR);
// OBSOLETE       break;
// OBSOLETE     case SIZE:
// OBSOLETE       type = parse_mode_or_normal_call ();
// OBSOLETE       if (type)
// OBSOLETE 	{
// OBSOLETE 	  write_exp_elt_opcode (OP_LONG);
// OBSOLETE 	  write_exp_elt_type (builtin_type_int);
// OBSOLETE 	  CHECK_TYPEDEF (type);
// OBSOLETE 	  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH (type));
// OBSOLETE 	  write_exp_elt_opcode (OP_LONG);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	write_exp_elt_opcode (UNOP_SIZEOF);
// OBSOLETE       break;
// OBSOLETE     case LOWER:
// OBSOLETE       op = UNOP_LOWER;
// OBSOLETE       goto lower_upper;
// OBSOLETE     case UPPER:
// OBSOLETE       op = UNOP_UPPER;
// OBSOLETE       goto lower_upper;
// OBSOLETE     lower_upper:
// OBSOLETE       type = parse_mode_or_normal_call ();
// OBSOLETE       write_lower_upper_value (op, type);
// OBSOLETE       break;
// OBSOLETE     case LENGTH:
// OBSOLETE       parse_unary_call ();
// OBSOLETE       write_exp_elt_opcode (UNOP_LENGTH);
// OBSOLETE       break;
// OBSOLETE     case TYPENAME:
// OBSOLETE       type = PEEK_LVAL ().tsym.type;
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       switch (PEEK_TOKEN ())
// OBSOLETE 	{
// OBSOLETE 	case '[':
// OBSOLETE 	  parse_tuple (type);
// OBSOLETE 	  break;
// OBSOLETE 	case '(':
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  parse_expr ();
// OBSOLETE 	  expect (')', "missing right parenthesis");
// OBSOLETE 	  write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE 	  write_exp_elt_type (type);
// OBSOLETE 	  write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  error ("typename in invalid context");
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default:
// OBSOLETE       error ("invalid expression syntax at `%s'", lexptr);
// OBSOLETE     }
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       switch (PEEK_TOKEN ())
// OBSOLETE 	{
// OBSOLETE 	case DOT_FIELD_NAME:
// OBSOLETE 	  write_exp_elt_opcode (STRUCTOP_STRUCT);
// OBSOLETE 	  write_exp_string (PEEK_LVAL ().sval);
// OBSOLETE 	  write_exp_elt_opcode (STRUCTOP_STRUCT);
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  continue;
// OBSOLETE 	case POINTER:
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  if (PEEK_TOKEN () == TYPENAME)
// OBSOLETE 	    {
// OBSOLETE 	      type = PEEK_LVAL ().tsym.type;
// OBSOLETE 	      write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE 	      write_exp_elt_type (lookup_pointer_type (type));
// OBSOLETE 	      write_exp_elt_opcode (UNOP_CAST);
// OBSOLETE 	      FORWARD_TOKEN ();
// OBSOLETE 	    }
// OBSOLETE 	  write_exp_elt_opcode (UNOP_IND);
// OBSOLETE 	  continue;
// OBSOLETE 	case OPEN_PAREN:
// OBSOLETE 	  parse_call ();
// OBSOLETE 	  continue;
// OBSOLETE 	case CHARACTER_STRING_LITERAL:
// OBSOLETE 	case CHARACTER_LITERAL:
// OBSOLETE 	case BIT_STRING_LITERAL:
// OBSOLETE 	  /* Handle string repetition. (See comment in parse_operand5.) */
// OBSOLETE 	  parse_primval ();
// OBSOLETE 	  write_exp_elt_opcode (MULTI_SUBSCRIPT);
// OBSOLETE 	  write_exp_elt_longcst (1);
// OBSOLETE 	  write_exp_elt_opcode (MULTI_SUBSCRIPT);
// OBSOLETE 	  continue;
// OBSOLETE 	case END_TOKEN:
// OBSOLETE 	case TOKEN_NOT_READ:
// OBSOLETE 	case INTEGER_LITERAL:
// OBSOLETE 	case BOOLEAN_LITERAL:
// OBSOLETE 	case FLOAT_LITERAL:
// OBSOLETE 	case GENERAL_PROCEDURE_NAME:
// OBSOLETE 	case LOCATION_NAME:
// OBSOLETE 	case EMPTINESS_LITERAL:
// OBSOLETE 	case TYPENAME:
// OBSOLETE 	case CASE:
// OBSOLETE 	case OF:
// OBSOLETE 	case ESAC:
// OBSOLETE 	case LOGIOR:
// OBSOLETE 	case ORIF:
// OBSOLETE 	case LOGXOR:
// OBSOLETE 	case LOGAND:
// OBSOLETE 	case ANDIF:
// OBSOLETE 	case NOTEQUAL:
// OBSOLETE 	case GEQ:
// OBSOLETE 	case LEQ:
// OBSOLETE 	case IN:
// OBSOLETE 	case SLASH_SLASH:
// OBSOLETE 	case MOD:
// OBSOLETE 	case REM:
// OBSOLETE 	case NOT:
// OBSOLETE 	case RECEIVE:
// OBSOLETE 	case UP:
// OBSOLETE 	case IF:
// OBSOLETE 	case THEN:
// OBSOLETE 	case ELSE:
// OBSOLETE 	case FI:
// OBSOLETE 	case ELSIF:
// OBSOLETE 	case ILLEGAL_TOKEN:
// OBSOLETE 	case NUM:
// OBSOLETE 	case PRED:
// OBSOLETE 	case SUCC:
// OBSOLETE 	case ABS:
// OBSOLETE 	case CARD:
// OBSOLETE 	case MAX_TOKEN:
// OBSOLETE 	case MIN_TOKEN:
// OBSOLETE 	case ADDR_TOKEN:
// OBSOLETE 	case SIZE:
// OBSOLETE 	case UPPER:
// OBSOLETE 	case LOWER:
// OBSOLETE 	case LENGTH:
// OBSOLETE 	case ARRAY:
// OBSOLETE 	case GDB_VARIABLE:
// OBSOLETE 	case GDB_ASSIGNMENT:
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   return;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand6 (void)
// OBSOLETE {
// OBSOLETE   if (check_token (RECEIVE))
// OBSOLETE     {
// OBSOLETE       parse_primval ();
// OBSOLETE       error ("not implemented:  RECEIVE expression");
// OBSOLETE     }
// OBSOLETE   else if (check_token (POINTER))
// OBSOLETE     {
// OBSOLETE       parse_primval ();
// OBSOLETE       write_exp_elt_opcode (UNOP_ADDR);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     parse_primval ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand5 (void)
// OBSOLETE {
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   /* We are supposed to be looking for a <string repetition operator>,
// OBSOLETE      but in general we can't distinguish that from a parenthesized
// OBSOLETE      expression.  This is especially difficult if we allow the
// OBSOLETE      string operand to be a constant expression (as requested by
// OBSOLETE      some users), and not just a string literal.
// OBSOLETE      Consider:  LPRN expr RPRN LPRN expr RPRN
// OBSOLETE      Is that a function call or string repetition?
// OBSOLETE      Instead, we handle string repetition in parse_primval,
// OBSOLETE      and build_generalized_call. */
// OBSOLETE   switch (PEEK_TOKEN ())
// OBSOLETE     {
// OBSOLETE     case NOT:
// OBSOLETE       op = UNOP_LOGICAL_NOT;
// OBSOLETE       break;
// OBSOLETE     case '-':
// OBSOLETE       op = UNOP_NEG;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       op = OP_NULL;
// OBSOLETE     }
// OBSOLETE   if (op != OP_NULL)
// OBSOLETE     FORWARD_TOKEN ();
// OBSOLETE   parse_operand6 ();
// OBSOLETE   if (op != OP_NULL)
// OBSOLETE     write_exp_elt_opcode (op);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand4 (void)
// OBSOLETE {
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   parse_operand5 ();
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       switch (PEEK_TOKEN ())
// OBSOLETE 	{
// OBSOLETE 	case '*':
// OBSOLETE 	  op = BINOP_MUL;
// OBSOLETE 	  break;
// OBSOLETE 	case '/':
// OBSOLETE 	  op = BINOP_DIV;
// OBSOLETE 	  break;
// OBSOLETE 	case MOD:
// OBSOLETE 	  op = BINOP_MOD;
// OBSOLETE 	  break;
// OBSOLETE 	case REM:
// OBSOLETE 	  op = BINOP_REM;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       parse_operand5 ();
// OBSOLETE       write_exp_elt_opcode (op);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand3 (void)
// OBSOLETE {
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   parse_operand4 ();
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       switch (PEEK_TOKEN ())
// OBSOLETE 	{
// OBSOLETE 	case '+':
// OBSOLETE 	  op = BINOP_ADD;
// OBSOLETE 	  break;
// OBSOLETE 	case '-':
// OBSOLETE 	  op = BINOP_SUB;
// OBSOLETE 	  break;
// OBSOLETE 	case SLASH_SLASH:
// OBSOLETE 	  op = BINOP_CONCAT;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       parse_operand4 ();
// OBSOLETE       write_exp_elt_opcode (op);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand2 (void)
// OBSOLETE {
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   parse_operand3 ();
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       if (check_token (IN))
// OBSOLETE 	{
// OBSOLETE 	  parse_operand3 ();
// OBSOLETE 	  write_exp_elt_opcode (BINOP_IN);
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  switch (PEEK_TOKEN ())
// OBSOLETE 	    {
// OBSOLETE 	    case '>':
// OBSOLETE 	      op = BINOP_GTR;
// OBSOLETE 	      break;
// OBSOLETE 	    case GEQ:
// OBSOLETE 	      op = BINOP_GEQ;
// OBSOLETE 	      break;
// OBSOLETE 	    case '<':
// OBSOLETE 	      op = BINOP_LESS;
// OBSOLETE 	      break;
// OBSOLETE 	    case LEQ:
// OBSOLETE 	      op = BINOP_LEQ;
// OBSOLETE 	      break;
// OBSOLETE 	    case '=':
// OBSOLETE 	      op = BINOP_EQUAL;
// OBSOLETE 	      break;
// OBSOLETE 	    case NOTEQUAL:
// OBSOLETE 	      op = BINOP_NOTEQUAL;
// OBSOLETE 	      break;
// OBSOLETE 	    default:
// OBSOLETE 	      return;
// OBSOLETE 	    }
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  parse_operand3 ();
// OBSOLETE 	  write_exp_elt_opcode (op);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand1 (void)
// OBSOLETE {
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   parse_operand2 ();
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       switch (PEEK_TOKEN ())
// OBSOLETE 	{
// OBSOLETE 	case LOGAND:
// OBSOLETE 	  op = BINOP_BITWISE_AND;
// OBSOLETE 	  break;
// OBSOLETE 	case ANDIF:
// OBSOLETE 	  op = BINOP_LOGICAL_AND;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       parse_operand2 ();
// OBSOLETE       write_exp_elt_opcode (op);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_operand0 (void)
// OBSOLETE {
// OBSOLETE   enum exp_opcode op;
// OBSOLETE   parse_operand1 ();
// OBSOLETE   for (;;)
// OBSOLETE     {
// OBSOLETE       switch (PEEK_TOKEN ())
// OBSOLETE 	{
// OBSOLETE 	case LOGIOR:
// OBSOLETE 	  op = BINOP_BITWISE_IOR;
// OBSOLETE 	  break;
// OBSOLETE 	case LOGXOR:
// OBSOLETE 	  op = BINOP_BITWISE_XOR;
// OBSOLETE 	  break;
// OBSOLETE 	case ORIF:
// OBSOLETE 	  op = BINOP_LOGICAL_OR;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  return;
// OBSOLETE 	}
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE       parse_operand1 ();
// OBSOLETE       write_exp_elt_opcode (op);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_expr (void)
// OBSOLETE {
// OBSOLETE   parse_operand0 ();
// OBSOLETE   if (check_token (GDB_ASSIGNMENT))
// OBSOLETE     {
// OBSOLETE       parse_expr ();
// OBSOLETE       write_exp_elt_opcode (BINOP_ASSIGN);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_then_alternative (void)
// OBSOLETE {
// OBSOLETE   expect (THEN, "missing 'THEN' in 'IF' expression");
// OBSOLETE   parse_expr ();
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_else_alternative (void)
// OBSOLETE {
// OBSOLETE   if (check_token (ELSIF))
// OBSOLETE     parse_if_expression_body ();
// OBSOLETE   else if (check_token (ELSE))
// OBSOLETE     parse_expr ();
// OBSOLETE   else
// OBSOLETE     error ("missing ELSE/ELSIF in IF expression");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Matches: <boolean expression> <then alternative> <else alternative> */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_if_expression_body (void)
// OBSOLETE {
// OBSOLETE   parse_expr ();
// OBSOLETE   parse_then_alternative ();
// OBSOLETE   parse_else_alternative ();
// OBSOLETE   write_exp_elt_opcode (TERNOP_COND);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_if_expression (void)
// OBSOLETE {
// OBSOLETE   require (IF);
// OBSOLETE   parse_if_expression_body ();
// OBSOLETE   expect (FI, "missing 'FI' at end of conditional expression");
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* An <untyped_expr> is a superset of <expr>.  It also includes
// OBSOLETE    <conditional expressions> and untyped <tuples>, whose types
// OBSOLETE    are not given by their constituents.  Hence, these are only
// OBSOLETE    allowed in certain contexts that expect a certain type.
// OBSOLETE    You should call convert() to fix up the <untyped_expr>. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE parse_untyped_expr (void)
// OBSOLETE {
// OBSOLETE   switch (PEEK_TOKEN ())
// OBSOLETE     {
// OBSOLETE     case IF:
// OBSOLETE       parse_if_expression ();
// OBSOLETE       return;
// OBSOLETE     case CASE:
// OBSOLETE       error ("not implemented:  CASE expression");
// OBSOLETE     case '(':
// OBSOLETE       switch (PEEK_TOKEN1 ())
// OBSOLETE 	{
// OBSOLETE 	case IF:
// OBSOLETE 	case CASE:
// OBSOLETE 	  goto skip_lprn;
// OBSOLETE 	case '[':
// OBSOLETE 	skip_lprn:
// OBSOLETE 	  FORWARD_TOKEN ();
// OBSOLETE 	  parse_untyped_expr ();
// OBSOLETE 	  expect (')', "missing ')'");
// OBSOLETE 	  return;
// OBSOLETE 	default:;
// OBSOLETE 	  /* fall through */
// OBSOLETE 	}
// OBSOLETE     default:
// OBSOLETE       parse_operand0 ();
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE int
// OBSOLETE chill_parse (void)
// OBSOLETE {
// OBSOLETE   terminal_buffer[0] = TOKEN_NOT_READ;
// OBSOLETE   if (PEEK_TOKEN () == TYPENAME && PEEK_TOKEN1 () == END_TOKEN)
// OBSOLETE     {
// OBSOLETE       write_exp_elt_opcode (OP_TYPE);
// OBSOLETE       write_exp_elt_type (PEEK_LVAL ().tsym.type);
// OBSOLETE       write_exp_elt_opcode (OP_TYPE);
// OBSOLETE       FORWARD_TOKEN ();
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     parse_expr ();
// OBSOLETE   if (terminal_buffer[0] != END_TOKEN)
// OBSOLETE     {
// OBSOLETE       if (comma_terminates && terminal_buffer[0] == ',')
// OBSOLETE 	lexptr--;		/* Put the comma back.  */
// OBSOLETE       else
// OBSOLETE 	error ("Junk after end of expression.");
// OBSOLETE     }
// OBSOLETE   return 0;
// OBSOLETE }
// OBSOLETE 
// OBSOLETE 
// OBSOLETE /* Implementation of a dynamically expandable buffer for processing input
// OBSOLETE    characters acquired through lexptr and building a value to return in
// OBSOLETE    yylval. */
// OBSOLETE 
// OBSOLETE static char *tempbuf;		/* Current buffer contents */
// OBSOLETE static int tempbufsize;		/* Size of allocated buffer */
// OBSOLETE static int tempbufindex;	/* Current index into buffer */
// OBSOLETE 
// OBSOLETE #define GROWBY_MIN_SIZE 64	/* Minimum amount to grow buffer by */
// OBSOLETE 
// OBSOLETE #define CHECKBUF(size) \
// OBSOLETE   do { \
// OBSOLETE     if (tempbufindex + (size) >= tempbufsize) \
// OBSOLETE       { \
// OBSOLETE 	growbuf_by_size (size); \
// OBSOLETE       } \
// OBSOLETE   } while (0);
// OBSOLETE 
// OBSOLETE /* Grow the static temp buffer if necessary, including allocating the first one
// OBSOLETE    on demand. */
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE growbuf_by_size (int count)
// OBSOLETE {
// OBSOLETE   int growby;
// OBSOLETE 
// OBSOLETE   growby = max (count, GROWBY_MIN_SIZE);
// OBSOLETE   tempbufsize += growby;
// OBSOLETE   if (tempbuf == NULL)
// OBSOLETE     {
// OBSOLETE       tempbuf = (char *) xmalloc (tempbufsize);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       tempbuf = (char *) xrealloc (tempbuf, tempbufsize);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Try to consume a simple name string token.  If successful, returns
// OBSOLETE    a pointer to a nullbyte terminated copy of the name that can be used
// OBSOLETE    in symbol table lookups.  If not successful, returns NULL. */
// OBSOLETE 
// OBSOLETE static char *
// OBSOLETE match_simple_name_string (void)
// OBSOLETE {
// OBSOLETE   char *tokptr = lexptr;
// OBSOLETE 
// OBSOLETE   if (isalpha (*tokptr) || *tokptr == '_')
// OBSOLETE     {
// OBSOLETE       char *result;
// OBSOLETE       do
// OBSOLETE 	{
// OBSOLETE 	  tokptr++;
// OBSOLETE 	}
// OBSOLETE       while (isalnum (*tokptr) || (*tokptr == '_'));
// OBSOLETE       yylval.sval.ptr = lexptr;
// OBSOLETE       yylval.sval.length = tokptr - lexptr;
// OBSOLETE       lexptr = tokptr;
// OBSOLETE       result = copy_name (yylval.sval);
// OBSOLETE       return result;
// OBSOLETE     }
// OBSOLETE   return (NULL);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Start looking for a value composed of valid digits as set by the base
// OBSOLETE    in use.  Note that '_' characters are valid anywhere, in any quantity,
// OBSOLETE    and are simply ignored.  Since we must find at least one valid digit,
// OBSOLETE    or reject this token as an integer literal, we keep track of how many
// OBSOLETE    digits we have encountered. */
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE decode_integer_value (int base, char **tokptrptr, LONGEST *ivalptr)
// OBSOLETE {
// OBSOLETE   char *tokptr = *tokptrptr;
// OBSOLETE   int temp;
// OBSOLETE   int digits = 0;
// OBSOLETE 
// OBSOLETE   while (*tokptr != '\0')
// OBSOLETE     {
// OBSOLETE       temp = *tokptr;
// OBSOLETE       if (isupper (temp))
// OBSOLETE 	temp = tolower (temp);
// OBSOLETE       tokptr++;
// OBSOLETE       switch (temp)
// OBSOLETE 	{
// OBSOLETE 	case '_':
// OBSOLETE 	  continue;
// OBSOLETE 	case '0':
// OBSOLETE 	case '1':
// OBSOLETE 	case '2':
// OBSOLETE 	case '3':
// OBSOLETE 	case '4':
// OBSOLETE 	case '5':
// OBSOLETE 	case '6':
// OBSOLETE 	case '7':
// OBSOLETE 	case '8':
// OBSOLETE 	case '9':
// OBSOLETE 	  temp -= '0';
// OBSOLETE 	  break;
// OBSOLETE 	case 'a':
// OBSOLETE 	case 'b':
// OBSOLETE 	case 'c':
// OBSOLETE 	case 'd':
// OBSOLETE 	case 'e':
// OBSOLETE 	case 'f':
// OBSOLETE 	  temp -= 'a';
// OBSOLETE 	  temp += 10;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  temp = base;
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE       if (temp < base)
// OBSOLETE 	{
// OBSOLETE 	  digits++;
// OBSOLETE 	  *ivalptr *= base;
// OBSOLETE 	  *ivalptr += temp;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* Found something not in domain for current base. */
// OBSOLETE 	  tokptr--;		/* Unconsume what gave us indigestion. */
// OBSOLETE 	  break;
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* If we didn't find any digits, then we don't have a valid integer
// OBSOLETE      value, so reject the entire token.  Otherwise, update the lexical
// OBSOLETE      scan pointer, and return non-zero for success. */
// OBSOLETE 
// OBSOLETE   if (digits == 0)
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       *tokptrptr = tokptr;
// OBSOLETE       return (1);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static int
// OBSOLETE decode_integer_literal (LONGEST *valptr, char **tokptrptr)
// OBSOLETE {
// OBSOLETE   char *tokptr = *tokptrptr;
// OBSOLETE   int base = 0;
// OBSOLETE   LONGEST ival = 0;
// OBSOLETE   int explicit_base = 0;
// OBSOLETE 
// OBSOLETE   /* Look for an explicit base specifier, which is optional. */
// OBSOLETE 
// OBSOLETE   switch (*tokptr)
// OBSOLETE     {
// OBSOLETE     case 'd':
// OBSOLETE     case 'D':
// OBSOLETE       explicit_base++;
// OBSOLETE       base = 10;
// OBSOLETE       tokptr++;
// OBSOLETE       break;
// OBSOLETE     case 'b':
// OBSOLETE     case 'B':
// OBSOLETE       explicit_base++;
// OBSOLETE       base = 2;
// OBSOLETE       tokptr++;
// OBSOLETE       break;
// OBSOLETE     case 'h':
// OBSOLETE     case 'H':
// OBSOLETE       explicit_base++;
// OBSOLETE       base = 16;
// OBSOLETE       tokptr++;
// OBSOLETE       break;
// OBSOLETE     case 'o':
// OBSOLETE     case 'O':
// OBSOLETE       explicit_base++;
// OBSOLETE       base = 8;
// OBSOLETE       tokptr++;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       base = 10;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* If we found an explicit base ensure that the character after the
// OBSOLETE      explicit base is a single quote. */
// OBSOLETE 
// OBSOLETE   if (explicit_base && (*tokptr++ != '\''))
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Attempt to decode whatever follows as an integer value in the
// OBSOLETE      indicated base, updating the token pointer in the process and
// OBSOLETE      computing the value into ival.  Also, if we have an explicit
// OBSOLETE      base, then the next character must not be a single quote, or we
// OBSOLETE      have a bitstring literal, so reject the entire token in this case.
// OBSOLETE      Otherwise, update the lexical scan pointer, and return non-zero
// OBSOLETE      for success. */
// OBSOLETE 
// OBSOLETE   if (!decode_integer_value (base, &tokptr, &ival))
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else if (explicit_base && (*tokptr == '\''))
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       *valptr = ival;
// OBSOLETE       *tokptrptr = tokptr;
// OBSOLETE       return (1);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /*  If it wasn't for the fact that floating point values can contain '_'
// OBSOLETE    characters, we could just let strtod do all the hard work by letting it
// OBSOLETE    try to consume as much of the current token buffer as possible and
// OBSOLETE    find a legal conversion.  Unfortunately we need to filter out the '_'
// OBSOLETE    characters before calling strtod, which we do by copying the other
// OBSOLETE    legal chars to a local buffer to be converted.  However since we also
// OBSOLETE    need to keep track of where the last unconsumed character in the input
// OBSOLETE    buffer is, we have transfer only as many characters as may compose a
// OBSOLETE    legal floating point value. */
// OBSOLETE 
// OBSOLETE static enum ch_terminal
// OBSOLETE match_float_literal (void)
// OBSOLETE {
// OBSOLETE   char *tokptr = lexptr;
// OBSOLETE   char *buf;
// OBSOLETE   char *copy;
// OBSOLETE   double dval;
// OBSOLETE   extern double strtod ();
// OBSOLETE 
// OBSOLETE   /* Make local buffer in which to build the string to convert.  This is
// OBSOLETE      required because underscores are valid in chill floating point numbers
// OBSOLETE      but not in the string passed to strtod to convert.  The string will be
// OBSOLETE      no longer than our input string. */
// OBSOLETE 
// OBSOLETE   copy = buf = (char *) alloca (strlen (tokptr) + 1);
// OBSOLETE 
// OBSOLETE   /* Transfer all leading digits to the conversion buffer, discarding any
// OBSOLETE      underscores. */
// OBSOLETE 
// OBSOLETE   while (isdigit (*tokptr) || *tokptr == '_')
// OBSOLETE     {
// OBSOLETE       if (*tokptr != '_')
// OBSOLETE 	{
// OBSOLETE 	  *copy++ = *tokptr;
// OBSOLETE 	}
// OBSOLETE       tokptr++;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Now accept either a '.', or one of [eEdD].  Dot is legal regardless
// OBSOLETE      of whether we found any leading digits, and we simply accept it and
// OBSOLETE      continue on to look for the fractional part and/or exponent.  One of
// OBSOLETE      [eEdD] is legal only if we have seen digits, and means that there
// OBSOLETE      is no fractional part.  If we find neither of these, then this is
// OBSOLETE      not a floating point number, so return failure. */
// OBSOLETE 
// OBSOLETE   switch (*tokptr++)
// OBSOLETE     {
// OBSOLETE     case '.':
// OBSOLETE       /* Accept and then look for fractional part and/or exponent. */
// OBSOLETE       *copy++ = '.';
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     case 'e':
// OBSOLETE     case 'E':
// OBSOLETE     case 'd':
// OBSOLETE     case 'D':
// OBSOLETE       if (copy == buf)
// OBSOLETE 	{
// OBSOLETE 	  return (0);
// OBSOLETE 	}
// OBSOLETE       *copy++ = 'e';
// OBSOLETE       goto collect_exponent;
// OBSOLETE       break;
// OBSOLETE 
// OBSOLETE     default:
// OBSOLETE       return (0);
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* We found a '.', copy any fractional digits to the conversion buffer, up
// OBSOLETE      to the first nondigit, non-underscore character. */
// OBSOLETE 
// OBSOLETE   while (isdigit (*tokptr) || *tokptr == '_')
// OBSOLETE     {
// OBSOLETE       if (*tokptr != '_')
// OBSOLETE 	{
// OBSOLETE 	  *copy++ = *tokptr;
// OBSOLETE 	}
// OBSOLETE       tokptr++;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Look for an exponent, which must start with one of [eEdD].  If none
// OBSOLETE      is found, jump directly to trying to convert what we have collected
// OBSOLETE      so far. */
// OBSOLETE 
// OBSOLETE   switch (*tokptr)
// OBSOLETE     {
// OBSOLETE     case 'e':
// OBSOLETE     case 'E':
// OBSOLETE     case 'd':
// OBSOLETE     case 'D':
// OBSOLETE       *copy++ = 'e';
// OBSOLETE       tokptr++;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       goto convert_float;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Accept an optional '-' or '+' following one of [eEdD]. */
// OBSOLETE 
// OBSOLETE collect_exponent:
// OBSOLETE   if (*tokptr == '+' || *tokptr == '-')
// OBSOLETE     {
// OBSOLETE       *copy++ = *tokptr++;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Now copy an exponent into the conversion buffer.  Note that at the 
// OBSOLETE      moment underscores are *not* allowed in exponents. */
// OBSOLETE 
// OBSOLETE   while (isdigit (*tokptr))
// OBSOLETE     {
// OBSOLETE       *copy++ = *tokptr++;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* If we transfered any chars to the conversion buffer, try to interpret its
// OBSOLETE      contents as a floating point value.  If any characters remain, then we
// OBSOLETE      must not have a valid floating point string. */
// OBSOLETE 
// OBSOLETE convert_float:
// OBSOLETE   *copy = '\0';
// OBSOLETE   if (copy != buf)
// OBSOLETE     {
// OBSOLETE       dval = strtod (buf, &copy);
// OBSOLETE       if (*copy == '\0')
// OBSOLETE 	{
// OBSOLETE 	  yylval.dval = dval;
// OBSOLETE 	  lexptr = tokptr;
// OBSOLETE 	  return (FLOAT_LITERAL);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   return (0);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Recognize a string literal.  A string literal is a sequence
// OBSOLETE    of characters enclosed in matching single or double quotes, except that
// OBSOLETE    a single character inside single quotes is a character literal, which
// OBSOLETE    we reject as a string literal.  To embed the terminator character inside
// OBSOLETE    a string, it is simply doubled (I.E. "this""is""one""string") */
// OBSOLETE 
// OBSOLETE static enum ch_terminal
// OBSOLETE match_string_literal (void)
// OBSOLETE {
// OBSOLETE   char *tokptr = lexptr;
// OBSOLETE   int in_ctrlseq = 0;
// OBSOLETE   LONGEST ival;
// OBSOLETE 
// OBSOLETE   for (tempbufindex = 0, tokptr++; *tokptr != '\0'; tokptr++)
// OBSOLETE     {
// OBSOLETE       CHECKBUF (1);
// OBSOLETE     tryagain:;
// OBSOLETE       if (in_ctrlseq)
// OBSOLETE 	{
// OBSOLETE 	  /* skip possible whitespaces */
// OBSOLETE 	  while ((*tokptr == ' ' || *tokptr == '\t') && *tokptr)
// OBSOLETE 	    tokptr++;
// OBSOLETE 	  if (*tokptr == ')')
// OBSOLETE 	    {
// OBSOLETE 	      in_ctrlseq = 0;
// OBSOLETE 	      tokptr++;
// OBSOLETE 	      goto tryagain;
// OBSOLETE 	    }
// OBSOLETE 	  else if (*tokptr != ',')
// OBSOLETE 	    error ("Invalid control sequence");
// OBSOLETE 	  tokptr++;
// OBSOLETE 	  /* skip possible whitespaces */
// OBSOLETE 	  while ((*tokptr == ' ' || *tokptr == '\t') && *tokptr)
// OBSOLETE 	    tokptr++;
// OBSOLETE 	  if (!decode_integer_literal (&ival, &tokptr))
// OBSOLETE 	    error ("Invalid control sequence");
// OBSOLETE 	  tokptr--;
// OBSOLETE 	}
// OBSOLETE       else if (*tokptr == *lexptr)
// OBSOLETE 	{
// OBSOLETE 	  if (*(tokptr + 1) == *lexptr)
// OBSOLETE 	    {
// OBSOLETE 	      ival = *tokptr++;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    {
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else if (*tokptr == '^')
// OBSOLETE 	{
// OBSOLETE 	  if (*(tokptr + 1) == '(')
// OBSOLETE 	    {
// OBSOLETE 	      in_ctrlseq = 1;
// OBSOLETE 	      tokptr += 2;
// OBSOLETE 	      if (!decode_integer_literal (&ival, &tokptr))
// OBSOLETE 		error ("Invalid control sequence");
// OBSOLETE 	      tokptr--;
// OBSOLETE 	    }
// OBSOLETE 	  else if (*(tokptr + 1) == '^')
// OBSOLETE 	    ival = *tokptr++;
// OBSOLETE 	  else
// OBSOLETE 	    error ("Invalid control sequence");
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	ival = *tokptr;
// OBSOLETE       tempbuf[tempbufindex++] = ival;
// OBSOLETE     }
// OBSOLETE   if (in_ctrlseq)
// OBSOLETE     error ("Invalid control sequence");
// OBSOLETE 
// OBSOLETE   if (*tokptr == '\0'		/* no terminator */
// OBSOLETE       || (tempbufindex == 1 && *tokptr == '\''))	/* char literal */
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       tempbuf[tempbufindex] = '\0';
// OBSOLETE       yylval.sval.ptr = tempbuf;
// OBSOLETE       yylval.sval.length = tempbufindex;
// OBSOLETE       lexptr = ++tokptr;
// OBSOLETE       return (CHARACTER_STRING_LITERAL);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Recognize a character literal.  A character literal is single character
// OBSOLETE    or a control sequence, enclosed in single quotes.  A control sequence
// OBSOLETE    is a comma separated list of one or more integer literals, enclosed
// OBSOLETE    in parenthesis and introduced with a circumflex character.
// OBSOLETE 
// OBSOLETE    EX:  'a'  '^(7)'  '^(7,8)'
// OBSOLETE 
// OBSOLETE    As a GNU chill extension, the syntax C'xx' is also recognized as a 
// OBSOLETE    character literal, where xx is a hex value for the character.
// OBSOLETE 
// OBSOLETE    Note that more than a single character, enclosed in single quotes, is
// OBSOLETE    a string literal.
// OBSOLETE 
// OBSOLETE    Returns CHARACTER_LITERAL if a match is found.
// OBSOLETE  */
// OBSOLETE 
// OBSOLETE static enum ch_terminal
// OBSOLETE match_character_literal (void)
// OBSOLETE {
// OBSOLETE   char *tokptr = lexptr;
// OBSOLETE   LONGEST ival = 0;
// OBSOLETE 
// OBSOLETE   if ((*tokptr == 'c' || *tokptr == 'C') && (*(tokptr + 1) == '\''))
// OBSOLETE     {
// OBSOLETE       /* We have a GNU chill extension form, so skip the leading "C'",
// OBSOLETE          decode the hex value, and then ensure that we have a trailing
// OBSOLETE          single quote character. */
// OBSOLETE       tokptr += 2;
// OBSOLETE       if (!decode_integer_value (16, &tokptr, &ival) || (*tokptr != '\''))
// OBSOLETE 	{
// OBSOLETE 	  return (0);
// OBSOLETE 	}
// OBSOLETE       tokptr++;
// OBSOLETE     }
// OBSOLETE   else if (*tokptr == '\'')
// OBSOLETE     {
// OBSOLETE       tokptr++;
// OBSOLETE 
// OBSOLETE       /* Determine which form we have, either a control sequence or the
// OBSOLETE          single character form. */
// OBSOLETE 
// OBSOLETE       if (*tokptr == '^')
// OBSOLETE 	{
// OBSOLETE 	  if (*(tokptr + 1) == '(')
// OBSOLETE 	    {
// OBSOLETE 	      /* Match and decode a control sequence.  Return zero if we don't
// OBSOLETE 	         find a valid integer literal, or if the next unconsumed character
// OBSOLETE 	         after the integer literal is not the trailing ')'. */
// OBSOLETE 	      tokptr += 2;
// OBSOLETE 	      if (!decode_integer_literal (&ival, &tokptr) || (*tokptr++ != ')'))
// OBSOLETE 		{
// OBSOLETE 		  return (0);
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	  else if (*(tokptr + 1) == '^')
// OBSOLETE 	    {
// OBSOLETE 	      ival = *tokptr;
// OBSOLETE 	      tokptr += 2;
// OBSOLETE 	    }
// OBSOLETE 	  else
// OBSOLETE 	    /* fail */
// OBSOLETE 	    error ("Invalid control sequence");
// OBSOLETE 	}
// OBSOLETE       else if (*tokptr == '\'')
// OBSOLETE 	{
// OBSOLETE 	  /* this must be duplicated */
// OBSOLETE 	  ival = *tokptr;
// OBSOLETE 	  tokptr += 2;
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  ival = *tokptr++;
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       /* The trailing quote has not yet been consumed.  If we don't find
// OBSOLETE          it, then we have no match. */
// OBSOLETE 
// OBSOLETE       if (*tokptr++ != '\'')
// OBSOLETE 	{
// OBSOLETE 	  return (0);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       /* Not a character literal. */
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   yylval.typed_val.val = ival;
// OBSOLETE   yylval.typed_val.type = builtin_type_chill_char;
// OBSOLETE   lexptr = tokptr;
// OBSOLETE   return (CHARACTER_LITERAL);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Recognize an integer literal, as specified in Z.200 sec 5.2.4.2.
// OBSOLETE    Note that according to 5.2.4.2, a single "_" is also a valid integer
// OBSOLETE    literal, however GNU-chill requires there to be at least one "digit"
// OBSOLETE    in any integer literal. */
// OBSOLETE 
// OBSOLETE static enum ch_terminal
// OBSOLETE match_integer_literal (void)
// OBSOLETE {
// OBSOLETE   char *tokptr = lexptr;
// OBSOLETE   LONGEST ival;
// OBSOLETE 
// OBSOLETE   if (!decode_integer_literal (&ival, &tokptr))
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       yylval.typed_val.val = ival;
// OBSOLETE #if defined(CC_HAS_LONG_LONG)
// OBSOLETE       if (ival > (LONGEST) 2147483647U || ival < -(LONGEST) 2147483648U)
// OBSOLETE 	yylval.typed_val.type = builtin_type_long_long;
// OBSOLETE       else
// OBSOLETE #endif
// OBSOLETE 	yylval.typed_val.type = builtin_type_int;
// OBSOLETE       lexptr = tokptr;
// OBSOLETE       return (INTEGER_LITERAL);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE /* Recognize a bit-string literal, as specified in Z.200 sec 5.2.4.8
// OBSOLETE    Note that according to 5.2.4.8, a single "_" is also a valid bit-string
// OBSOLETE    literal, however GNU-chill requires there to be at least one "digit"
// OBSOLETE    in any bit-string literal. */
// OBSOLETE 
// OBSOLETE static enum ch_terminal
// OBSOLETE match_bitstring_literal (void)
// OBSOLETE {
// OBSOLETE   register char *tokptr = lexptr;
// OBSOLETE   int bitoffset = 0;
// OBSOLETE   int bitcount = 0;
// OBSOLETE   int bits_per_char;
// OBSOLETE   int digit;
// OBSOLETE 
// OBSOLETE   tempbufindex = 0;
// OBSOLETE   CHECKBUF (1);
// OBSOLETE   tempbuf[0] = 0;
// OBSOLETE 
// OBSOLETE   /* Look for the required explicit base specifier. */
// OBSOLETE 
// OBSOLETE   switch (*tokptr++)
// OBSOLETE     {
// OBSOLETE     case 'b':
// OBSOLETE     case 'B':
// OBSOLETE       bits_per_char = 1;
// OBSOLETE       break;
// OBSOLETE     case 'o':
// OBSOLETE     case 'O':
// OBSOLETE       bits_per_char = 3;
// OBSOLETE       break;
// OBSOLETE     case 'h':
// OBSOLETE     case 'H':
// OBSOLETE       bits_per_char = 4;
// OBSOLETE       break;
// OBSOLETE     default:
// OBSOLETE       return (0);
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Ensure that the character after the explicit base is a single quote. */
// OBSOLETE 
// OBSOLETE   if (*tokptr++ != '\'')
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   while (*tokptr != '\0' && *tokptr != '\'')
// OBSOLETE     {
// OBSOLETE       digit = *tokptr;
// OBSOLETE       if (isupper (digit))
// OBSOLETE 	digit = tolower (digit);
// OBSOLETE       tokptr++;
// OBSOLETE       switch (digit)
// OBSOLETE 	{
// OBSOLETE 	case '_':
// OBSOLETE 	  continue;
// OBSOLETE 	case '0':
// OBSOLETE 	case '1':
// OBSOLETE 	case '2':
// OBSOLETE 	case '3':
// OBSOLETE 	case '4':
// OBSOLETE 	case '5':
// OBSOLETE 	case '6':
// OBSOLETE 	case '7':
// OBSOLETE 	case '8':
// OBSOLETE 	case '9':
// OBSOLETE 	  digit -= '0';
// OBSOLETE 	  break;
// OBSOLETE 	case 'a':
// OBSOLETE 	case 'b':
// OBSOLETE 	case 'c':
// OBSOLETE 	case 'd':
// OBSOLETE 	case 'e':
// OBSOLETE 	case 'f':
// OBSOLETE 	  digit -= 'a';
// OBSOLETE 	  digit += 10;
// OBSOLETE 	  break;
// OBSOLETE 	default:
// OBSOLETE 	  /* this is not a bitstring literal, probably an integer */
// OBSOLETE 	  return 0;
// OBSOLETE 	}
// OBSOLETE       if (digit >= 1 << bits_per_char)
// OBSOLETE 	{
// OBSOLETE 	  /* Found something not in domain for current base. */
// OBSOLETE 	  error ("Too-large digit in bitstring or integer.");
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  /* Extract bits from digit, packing them into the bitstring byte. */
// OBSOLETE 	  int k = TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? bits_per_char - 1 : 0;
// OBSOLETE 	  for (; TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? k >= 0 : k < bits_per_char;
// OBSOLETE 	       TARGET_BYTE_ORDER == BFD_ENDIAN_BIG ? k-- : k++)
// OBSOLETE 	    {
// OBSOLETE 	      bitcount++;
// OBSOLETE 	      if (digit & (1 << k))
// OBSOLETE 		{
// OBSOLETE 		  tempbuf[tempbufindex] |=
// OBSOLETE 		    (TARGET_BYTE_ORDER == BFD_ENDIAN_BIG)
// OBSOLETE 		    ? (1 << (HOST_CHAR_BIT - 1 - bitoffset))
// OBSOLETE 		    : (1 << bitoffset);
// OBSOLETE 		}
// OBSOLETE 	      bitoffset++;
// OBSOLETE 	      if (bitoffset == HOST_CHAR_BIT)
// OBSOLETE 		{
// OBSOLETE 		  bitoffset = 0;
// OBSOLETE 		  tempbufindex++;
// OBSOLETE 		  CHECKBUF (1);
// OBSOLETE 		  tempbuf[tempbufindex] = 0;
// OBSOLETE 		}
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Verify that we consumed everything up to the trailing single quote,
// OBSOLETE      and that we found some bits (IE not just underbars). */
// OBSOLETE 
// OBSOLETE   if (*tokptr++ != '\'')
// OBSOLETE     {
// OBSOLETE       return (0);
// OBSOLETE     }
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       yylval.sval.ptr = tempbuf;
// OBSOLETE       yylval.sval.length = bitcount;
// OBSOLETE       lexptr = tokptr;
// OBSOLETE       return (BIT_STRING_LITERAL);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE struct token
// OBSOLETE {
// OBSOLETE   char *operator;
// OBSOLETE   int token;
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static const struct token idtokentab[] =
// OBSOLETE {
// OBSOLETE   {"array", ARRAY},
// OBSOLETE   {"length", LENGTH},
// OBSOLETE   {"lower", LOWER},
// OBSOLETE   {"upper", UPPER},
// OBSOLETE   {"andif", ANDIF},
// OBSOLETE   {"pred", PRED},
// OBSOLETE   {"succ", SUCC},
// OBSOLETE   {"card", CARD},
// OBSOLETE   {"size", SIZE},
// OBSOLETE   {"orif", ORIF},
// OBSOLETE   {"num", NUM},
// OBSOLETE   {"abs", ABS},
// OBSOLETE   {"max", MAX_TOKEN},
// OBSOLETE   {"min", MIN_TOKEN},
// OBSOLETE   {"mod", MOD},
// OBSOLETE   {"rem", REM},
// OBSOLETE   {"not", NOT},
// OBSOLETE   {"xor", LOGXOR},
// OBSOLETE   {"and", LOGAND},
// OBSOLETE   {"in", IN},
// OBSOLETE   {"or", LOGIOR},
// OBSOLETE   {"up", UP},
// OBSOLETE   {"addr", ADDR_TOKEN},
// OBSOLETE   {"null", EMPTINESS_LITERAL}
// OBSOLETE };
// OBSOLETE 
// OBSOLETE static const struct token tokentab2[] =
// OBSOLETE {
// OBSOLETE   {":=", GDB_ASSIGNMENT},
// OBSOLETE   {"//", SLASH_SLASH},
// OBSOLETE   {"->", POINTER},
// OBSOLETE   {"/=", NOTEQUAL},
// OBSOLETE   {"<=", LEQ},
// OBSOLETE   {">=", GEQ}
// OBSOLETE };
// OBSOLETE 
// OBSOLETE /* Read one token, getting characters through lexptr.  */
// OBSOLETE /* This is where we will check to make sure that the language and the
// OBSOLETE    operators used are compatible.  */
// OBSOLETE 
// OBSOLETE static enum ch_terminal
// OBSOLETE ch_lex (void)
// OBSOLETE {
// OBSOLETE   unsigned int i;
// OBSOLETE   enum ch_terminal token;
// OBSOLETE   char *inputname;
// OBSOLETE   struct symbol *sym;
// OBSOLETE 
// OBSOLETE   /* Skip over any leading whitespace. */
// OBSOLETE   while (isspace (*lexptr))
// OBSOLETE     {
// OBSOLETE       lexptr++;
// OBSOLETE     }
// OBSOLETE   /* Look for special single character cases which can't be the first
// OBSOLETE      character of some other multicharacter token. */
// OBSOLETE   switch (*lexptr)
// OBSOLETE     {
// OBSOLETE     case '\0':
// OBSOLETE       return END_TOKEN;
// OBSOLETE     case ',':
// OBSOLETE     case '=':
// OBSOLETE     case ';':
// OBSOLETE     case '!':
// OBSOLETE     case '+':
// OBSOLETE     case '*':
// OBSOLETE     case '(':
// OBSOLETE     case ')':
// OBSOLETE     case '[':
// OBSOLETE     case ']':
// OBSOLETE       return (*lexptr++);
// OBSOLETE     }
// OBSOLETE   /* Look for characters which start a particular kind of multicharacter
// OBSOLETE      token, such as a character literal, register name, convenience
// OBSOLETE      variable name, string literal, etc. */
// OBSOLETE   switch (*lexptr)
// OBSOLETE     {
// OBSOLETE     case '\'':
// OBSOLETE     case '\"':
// OBSOLETE       /* First try to match a string literal, which is any
// OBSOLETE          sequence of characters enclosed in matching single or double
// OBSOLETE          quotes, except that a single character inside single quotes
// OBSOLETE          is a character literal, so we have to catch that case also. */
// OBSOLETE       token = match_string_literal ();
// OBSOLETE       if (token != 0)
// OBSOLETE 	{
// OBSOLETE 	  return (token);
// OBSOLETE 	}
// OBSOLETE       if (*lexptr == '\'')
// OBSOLETE 	{
// OBSOLETE 	  token = match_character_literal ();
// OBSOLETE 	  if (token != 0)
// OBSOLETE 	    {
// OBSOLETE 	      return (token);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     case 'C':
// OBSOLETE     case 'c':
// OBSOLETE       token = match_character_literal ();
// OBSOLETE       if (token != 0)
// OBSOLETE 	{
// OBSOLETE 	  return (token);
// OBSOLETE 	}
// OBSOLETE       break;
// OBSOLETE     case '$':
// OBSOLETE       yylval.sval.ptr = lexptr;
// OBSOLETE       do
// OBSOLETE 	{
// OBSOLETE 	  lexptr++;
// OBSOLETE 	}
// OBSOLETE       while (isalnum (*lexptr) || *lexptr == '_' || *lexptr == '$');
// OBSOLETE       yylval.sval.length = lexptr - yylval.sval.ptr;
// OBSOLETE       write_dollar_variable (yylval.sval);
// OBSOLETE       return GDB_VARIABLE;
// OBSOLETE       break;
// OBSOLETE     }
// OBSOLETE   /* See if it is a special token of length 2.  */
// OBSOLETE   for (i = 0; i < sizeof (tokentab2) / sizeof (tokentab2[0]); i++)
// OBSOLETE     {
// OBSOLETE       if (STREQN (lexptr, tokentab2[i].operator, 2))
// OBSOLETE 	{
// OBSOLETE 	  lexptr += 2;
// OBSOLETE 	  return (tokentab2[i].token);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE   /* Look for single character cases which which could be the first
// OBSOLETE      character of some other multicharacter token, but aren't, or we
// OBSOLETE      would already have found it. */
// OBSOLETE   switch (*lexptr)
// OBSOLETE     {
// OBSOLETE     case '-':
// OBSOLETE     case ':':
// OBSOLETE     case '/':
// OBSOLETE     case '<':
// OBSOLETE     case '>':
// OBSOLETE       return (*lexptr++);
// OBSOLETE     }
// OBSOLETE   /* Look for a float literal before looking for an integer literal, so
// OBSOLETE      we match as much of the input stream as possible. */
// OBSOLETE   token = match_float_literal ();
// OBSOLETE   if (token != 0)
// OBSOLETE     {
// OBSOLETE       return (token);
// OBSOLETE     }
// OBSOLETE   token = match_bitstring_literal ();
// OBSOLETE   if (token != 0)
// OBSOLETE     {
// OBSOLETE       return (token);
// OBSOLETE     }
// OBSOLETE   token = match_integer_literal ();
// OBSOLETE   if (token != 0)
// OBSOLETE     {
// OBSOLETE       return (token);
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Try to match a simple name string, and if a match is found, then
// OBSOLETE      further classify what sort of name it is and return an appropriate
// OBSOLETE      token.  Note that attempting to match a simple name string consumes
// OBSOLETE      the token from lexptr, so we can't back out if we later find that
// OBSOLETE      we can't classify what sort of name it is. */
// OBSOLETE 
// OBSOLETE   inputname = match_simple_name_string ();
// OBSOLETE 
// OBSOLETE   if (inputname != NULL)
// OBSOLETE     {
// OBSOLETE       char *simplename = (char *) alloca (strlen (inputname) + 1);
// OBSOLETE 
// OBSOLETE       char *dptr = simplename, *sptr = inputname;
// OBSOLETE       for (; *sptr; sptr++)
// OBSOLETE 	*dptr++ = isupper (*sptr) ? tolower (*sptr) : *sptr;
// OBSOLETE       *dptr = '\0';
// OBSOLETE 
// OBSOLETE       /* See if it is a reserved identifier. */
// OBSOLETE       for (i = 0; i < sizeof (idtokentab) / sizeof (idtokentab[0]); i++)
// OBSOLETE 	{
// OBSOLETE 	  if (STREQ (simplename, idtokentab[i].operator))
// OBSOLETE 	    {
// OBSOLETE 	      return (idtokentab[i].token);
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       /* Look for other special tokens. */
// OBSOLETE       if (STREQ (simplename, "true"))
// OBSOLETE 	{
// OBSOLETE 	  yylval.ulval = 1;
// OBSOLETE 	  return (BOOLEAN_LITERAL);
// OBSOLETE 	}
// OBSOLETE       if (STREQ (simplename, "false"))
// OBSOLETE 	{
// OBSOLETE 	  yylval.ulval = 0;
// OBSOLETE 	  return (BOOLEAN_LITERAL);
// OBSOLETE 	}
// OBSOLETE 
// OBSOLETE       sym = lookup_symbol (inputname, expression_context_block,
// OBSOLETE 			   VAR_NAMESPACE, (int *) NULL,
// OBSOLETE 			   (struct symtab **) NULL);
// OBSOLETE       if (sym == NULL && strcmp (inputname, simplename) != 0)
// OBSOLETE 	{
// OBSOLETE 	  sym = lookup_symbol (simplename, expression_context_block,
// OBSOLETE 			       VAR_NAMESPACE, (int *) NULL,
// OBSOLETE 			       (struct symtab **) NULL);
// OBSOLETE 	}
// OBSOLETE       if (sym != NULL)
// OBSOLETE 	{
// OBSOLETE 	  yylval.ssym.stoken.ptr = NULL;
// OBSOLETE 	  yylval.ssym.stoken.length = 0;
// OBSOLETE 	  yylval.ssym.sym = sym;
// OBSOLETE 	  yylval.ssym.is_a_field_of_this = 0;	/* FIXME, C++'ism */
// OBSOLETE 	  switch (SYMBOL_CLASS (sym))
// OBSOLETE 	    {
// OBSOLETE 	    case LOC_BLOCK:
// OBSOLETE 	      /* Found a procedure name. */
// OBSOLETE 	      return (GENERAL_PROCEDURE_NAME);
// OBSOLETE 	    case LOC_STATIC:
// OBSOLETE 	      /* Found a global or local static variable. */
// OBSOLETE 	      return (LOCATION_NAME);
// OBSOLETE 	    case LOC_REGISTER:
// OBSOLETE 	    case LOC_ARG:
// OBSOLETE 	    case LOC_REF_ARG:
// OBSOLETE 	    case LOC_REGPARM:
// OBSOLETE 	    case LOC_REGPARM_ADDR:
// OBSOLETE 	    case LOC_LOCAL:
// OBSOLETE 	    case LOC_LOCAL_ARG:
// OBSOLETE 	    case LOC_BASEREG:
// OBSOLETE 	    case LOC_BASEREG_ARG:
// OBSOLETE 	      if (innermost_block == NULL
// OBSOLETE 		  || contained_in (block_found, innermost_block))
// OBSOLETE 		{
// OBSOLETE 		  innermost_block = block_found;
// OBSOLETE 		}
// OBSOLETE 	      return (LOCATION_NAME);
// OBSOLETE 	      break;
// OBSOLETE 	    case LOC_CONST:
// OBSOLETE 	    case LOC_LABEL:
// OBSOLETE 	      return (LOCATION_NAME);
// OBSOLETE 	      break;
// OBSOLETE 	    case LOC_TYPEDEF:
// OBSOLETE 	      yylval.tsym.type = SYMBOL_TYPE (sym);
// OBSOLETE 	      return TYPENAME;
// OBSOLETE 	    case LOC_UNDEF:
// OBSOLETE 	    case LOC_CONST_BYTES:
// OBSOLETE 	    case LOC_OPTIMIZED_OUT:
// OBSOLETE 	      error ("Symbol \"%s\" names no location.", inputname);
// OBSOLETE 	      break;
// OBSOLETE 	    default:
// OBSOLETE 	      internal_error (__FILE__, __LINE__,
// OBSOLETE 			      "unhandled SYMBOL_CLASS in ch_lex()");
// OBSOLETE 	      break;
// OBSOLETE 	    }
// OBSOLETE 	}
// OBSOLETE       else if (!have_full_symbols () && !have_partial_symbols ())
// OBSOLETE 	{
// OBSOLETE 	  error ("No symbol table is loaded.  Use the \"file\" command.");
// OBSOLETE 	}
// OBSOLETE       else
// OBSOLETE 	{
// OBSOLETE 	  error ("No symbol \"%s\" in current context.", inputname);
// OBSOLETE 	}
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   /* Catch single character tokens which are not part of some
// OBSOLETE      longer token. */
// OBSOLETE 
// OBSOLETE   switch (*lexptr)
// OBSOLETE     {
// OBSOLETE     case '.':			/* Not float for example. */
// OBSOLETE       lexptr++;
// OBSOLETE       while (isspace (*lexptr))
// OBSOLETE 	lexptr++;
// OBSOLETE       inputname = match_simple_name_string ();
// OBSOLETE       if (!inputname)
// OBSOLETE 	return '.';
// OBSOLETE       return DOT_FIELD_NAME;
// OBSOLETE     }
// OBSOLETE 
// OBSOLETE   return (ILLEGAL_TOKEN);
// OBSOLETE }
// OBSOLETE 
// OBSOLETE static void
// OBSOLETE write_lower_upper_value (enum exp_opcode opcode,	/* Either UNOP_LOWER or UNOP_UPPER */
// OBSOLETE 			 struct type *type)
// OBSOLETE {
// OBSOLETE   if (type == NULL)
// OBSOLETE     write_exp_elt_opcode (opcode);
// OBSOLETE   else
// OBSOLETE     {
// OBSOLETE       struct type *result_type;
// OBSOLETE       LONGEST val = type_lower_upper (opcode, type, &result_type);
// OBSOLETE       write_exp_elt_opcode (OP_LONG);
// OBSOLETE       write_exp_elt_type (result_type);
// OBSOLETE       write_exp_elt_longcst (val);
// OBSOLETE       write_exp_elt_opcode (OP_LONG);
// OBSOLETE     }
// OBSOLETE }
// OBSOLETE 
// OBSOLETE void
// OBSOLETE chill_error (char *msg)
// OBSOLETE {
// OBSOLETE   /* Never used. */
// OBSOLETE }
