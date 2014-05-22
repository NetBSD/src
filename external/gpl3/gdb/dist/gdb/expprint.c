/* Print in infix form a struct expression.

   Copyright (C) 1986-2013 Free Software Foundation, Inc.

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

#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "language.h"
#include "parser-defs.h"
#include "user-regs.h"		/* For user_reg_map_regnum_to_name.  */
#include "target.h"
#include "gdb_string.h"
#include "block.h"
#include "objfiles.h"
#include "gdb_assert.h"
#include "valprint.h"

#include <ctype.h>

void
print_expression (struct expression *exp, struct ui_file *stream)
{
  int pc = 0;

  print_subexp (exp, &pc, stream, PREC_NULL);
}

/* Print the subexpression of EXP that starts in position POS, on STREAM.
   PREC is the precedence of the surrounding operator;
   if the precedence of the main operator of this subexpression is less,
   parentheses are needed here.  */

void
print_subexp (struct expression *exp, int *pos,
	      struct ui_file *stream, enum precedence prec)
{
  exp->language_defn->la_exp_desc->print_subexp (exp, pos, stream, prec);
}

/* Standard implementation of print_subexp for use in language_defn
   vectors.  */
void
print_subexp_standard (struct expression *exp, int *pos,
		       struct ui_file *stream, enum precedence prec)
{
  unsigned tem;
  const struct op_print *op_print_tab;
  int pc;
  unsigned nargs;
  char *op_str;
  int assign_modify = 0;
  enum exp_opcode opcode;
  enum precedence myprec = PREC_NULL;
  /* Set to 1 for a right-associative operator.  */
  int assoc = 0;
  struct value *val;
  char *tempstr = NULL;

  op_print_tab = exp->language_defn->la_op_print_tab;
  pc = (*pos)++;
  opcode = exp->elts[pc].opcode;
  switch (opcode)
    {
      /* Common ops */

    case OP_TYPE:
      (*pos) += 2;
      type_print (exp->elts[pc + 1].type, "", stream, 0);
      return;

    case OP_SCOPE:
      myprec = PREC_PREFIX;
      assoc = 0;
      fputs_filtered (type_name_no_tag (exp->elts[pc + 1].type), stream);
      fputs_filtered ("::", stream);
      nargs = longest_to_int (exp->elts[pc + 2].longconst);
      (*pos) += 4 + BYTES_TO_EXP_ELEM (nargs + 1);
      fputs_filtered (&exp->elts[pc + 3].string, stream);
      return;

    case OP_LONG:
      {
	struct value_print_options opts;

	get_raw_print_options (&opts);
	(*pos) += 3;
	value_print (value_from_longest (exp->elts[pc + 1].type,
					 exp->elts[pc + 2].longconst),
		     stream, &opts);
      }
      return;

    case OP_DOUBLE:
      {
	struct value_print_options opts;

	get_raw_print_options (&opts);
	(*pos) += 3;
	value_print (value_from_double (exp->elts[pc + 1].type,
					exp->elts[pc + 2].doubleconst),
		     stream, &opts);
      }
      return;

    case OP_VAR_VALUE:
      {
	const struct block *b;

	(*pos) += 3;
	b = exp->elts[pc + 1].block;
	if (b != NULL
	    && BLOCK_FUNCTION (b) != NULL
	    && SYMBOL_PRINT_NAME (BLOCK_FUNCTION (b)) != NULL)
	  {
	    fputs_filtered (SYMBOL_PRINT_NAME (BLOCK_FUNCTION (b)), stream);
	    fputs_filtered ("::", stream);
	  }
	fputs_filtered (SYMBOL_PRINT_NAME (exp->elts[pc + 2].symbol), stream);
      }
      return;

    case OP_VAR_ENTRY_VALUE:
      {
	(*pos) += 2;
	fprintf_filtered (stream, "%s@entry",
			  SYMBOL_PRINT_NAME (exp->elts[pc + 1].symbol));
      }
      return;

    case OP_LAST:
      (*pos) += 2;
      fprintf_filtered (stream, "$%d",
			longest_to_int (exp->elts[pc + 1].longconst));
      return;

    case OP_REGISTER:
      {
	const char *name = &exp->elts[pc + 2].string;

	(*pos) += 3 + BYTES_TO_EXP_ELEM (exp->elts[pc + 1].longconst + 1);
	fprintf_filtered (stream, "$%s", name);
	return;
      }

    case OP_BOOL:
      (*pos) += 2;
      fprintf_filtered (stream, "%s",
			longest_to_int (exp->elts[pc + 1].longconst)
			? "TRUE" : "FALSE");
      return;

    case OP_INTERNALVAR:
      (*pos) += 2;
      fprintf_filtered (stream, "$%s",
			internalvar_name (exp->elts[pc + 1].internalvar));
      return;

    case OP_FUNCALL:
      (*pos) += 2;
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered (" (", stream);
      for (tem = 0; tem < nargs; tem++)
	{
	  if (tem != 0)
	    fputs_filtered (", ", stream);
	  print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
	}
      fputs_filtered (")", stream);
      return;

    case OP_NAME:
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (nargs + 1);
      fputs_filtered (&exp->elts[pc + 2].string, stream);
      return;

    case OP_STRING:
      {
	struct value_print_options opts;

	nargs = longest_to_int (exp->elts[pc + 1].longconst);
	(*pos) += 3 + BYTES_TO_EXP_ELEM (nargs + 1);
	/* LA_PRINT_STRING will print using the current repeat count threshold.
	   If necessary, we can temporarily set it to zero, or pass it as an
	   additional parameter to LA_PRINT_STRING.  -fnf */
	get_user_print_options (&opts);
	LA_PRINT_STRING (stream, builtin_type (exp->gdbarch)->builtin_char,
			 (gdb_byte *) &exp->elts[pc + 2].string, nargs,
			 NULL, 0, &opts);
      }
      return;

    case OP_OBJC_NSSTRING:	/* Objective-C Foundation Class
				   NSString constant.  */
      {
	struct value_print_options opts;

	nargs = longest_to_int (exp->elts[pc + 1].longconst);
	(*pos) += 3 + BYTES_TO_EXP_ELEM (nargs + 1);
	fputs_filtered ("@\"", stream);
	get_user_print_options (&opts);
	LA_PRINT_STRING (stream, builtin_type (exp->gdbarch)->builtin_char,
			 (gdb_byte *) &exp->elts[pc + 2].string, nargs,
			 NULL, 0, &opts);
	fputs_filtered ("\"", stream);
      }
      return;

    case OP_OBJC_MSGCALL:
      {			/* Objective C message (method) call.  */
	char *selector;

	(*pos) += 3;
	nargs = longest_to_int (exp->elts[pc + 2].longconst);
	fprintf_unfiltered (stream, "[");
	print_subexp (exp, pos, stream, PREC_SUFFIX);
	if (0 == target_read_string (exp->elts[pc + 1].longconst,
				     &selector, 1024, NULL))
	  {
	    error (_("bad selector"));
	    return;
	  }
	if (nargs)
	  {
	    char *s, *nextS;

	    s = alloca (strlen (selector) + 1);
	    strcpy (s, selector);
	    for (tem = 0; tem < nargs; tem++)
	      {
		nextS = strchr (s, ':');
		gdb_assert (nextS);	/* Make sure we found ':'.  */
		*nextS = '\0';
		fprintf_unfiltered (stream, " %s: ", s);
		s = nextS + 1;
		print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
	      }
	  }
	else
	  {
	    fprintf_unfiltered (stream, " %s", selector);
	  }
	fprintf_unfiltered (stream, "]");
	/* "selector" was malloc'd by target_read_string.  Free it.  */
	xfree (selector);
	return;
      }

    case OP_ARRAY:
      (*pos) += 3;
      nargs = longest_to_int (exp->elts[pc + 2].longconst);
      nargs -= longest_to_int (exp->elts[pc + 1].longconst);
      nargs++;
      tem = 0;
      if (exp->elts[pc + 4].opcode == OP_LONG
	  && exp->elts[pc + 5].type
	     == builtin_type (exp->gdbarch)->builtin_char
	  && exp->language_defn->la_language == language_c)
	{
	  /* Attempt to print C character arrays using string syntax.
	     Walk through the args, picking up one character from each
	     of the OP_LONG expression elements.  If any array element
	     does not match our expection of what we should find for
	     a simple string, revert back to array printing.  Note that
	     the last expression element is an explicit null terminator
	     byte, which doesn't get printed.  */
	  tempstr = alloca (nargs);
	  pc += 4;
	  while (tem < nargs)
	    {
	      if (exp->elts[pc].opcode != OP_LONG
		  || exp->elts[pc + 1].type
		     != builtin_type (exp->gdbarch)->builtin_char)
		{
		  /* Not a simple array of char, use regular array
		     printing.  */
		  tem = 0;
		  break;
		}
	      else
		{
		  tempstr[tem++] =
		    longest_to_int (exp->elts[pc + 2].longconst);
		  pc += 4;
		}
	    }
	}
      if (tem > 0)
	{
	  struct value_print_options opts;

	  get_user_print_options (&opts);
	  LA_PRINT_STRING (stream, builtin_type (exp->gdbarch)->builtin_char,
			   (gdb_byte *) tempstr, nargs - 1, NULL, 0, &opts);
	  (*pos) = pc;
	}
      else
	{
	  fputs_filtered (" {", stream);
	  for (tem = 0; tem < nargs; tem++)
	    {
	      if (tem != 0)
		{
		  fputs_filtered (", ", stream);
		}
	      print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
	    }
	  fputs_filtered ("}", stream);
	}
      return;

    case TERNOP_COND:
      if ((int) prec > (int) PREC_COMMA)
	fputs_filtered ("(", stream);
      /* Print the subexpressions, forcing parentheses
         around any binary operations within them.
         This is more parentheses than are strictly necessary,
         but it looks clearer.  */
      print_subexp (exp, pos, stream, PREC_HYPER);
      fputs_filtered (" ? ", stream);
      print_subexp (exp, pos, stream, PREC_HYPER);
      fputs_filtered (" : ", stream);
      print_subexp (exp, pos, stream, PREC_HYPER);
      if ((int) prec > (int) PREC_COMMA)
	fputs_filtered (")", stream);
      return;

    case TERNOP_SLICE:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("(", stream);
      print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
      fputs_filtered (opcode == TERNOP_SLICE ? " : " : " UP ", stream);
      print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
      fputs_filtered (")", stream);
      return;

    case STRUCTOP_STRUCT:
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered (".", stream);
      fputs_filtered (&exp->elts[pc + 2].string, stream);
      return;

      /* Will not occur for Modula-2.  */
    case STRUCTOP_PTR:
      tem = longest_to_int (exp->elts[pc + 1].longconst);
      (*pos) += 3 + BYTES_TO_EXP_ELEM (tem + 1);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("->", stream);
      fputs_filtered (&exp->elts[pc + 2].string, stream);
      return;

    case STRUCTOP_MEMBER:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered (".*", stream);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      return;

    case STRUCTOP_MPTR:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("->*", stream);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      return;

    case BINOP_SUBSCRIPT:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("[", stream);
      print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
      fputs_filtered ("]", stream);
      return;

    case UNOP_POSTINCREMENT:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("++", stream);
      return;

    case UNOP_POSTDECREMENT:
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fputs_filtered ("--", stream);
      return;

    case UNOP_CAST:
      (*pos) += 2;
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered ("(", stream);
      fputs_filtered ("(", stream);
      type_print (exp->elts[pc + 1].type, "", stream, 0);
      fputs_filtered (") ", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered (")", stream);
      return;

    case UNOP_CAST_TYPE:
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered ("(", stream);
      fputs_filtered ("(", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      fputs_filtered (") ", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered (")", stream);
      return;

    case UNOP_DYNAMIC_CAST:
    case UNOP_REINTERPRET_CAST:
      fputs_filtered (opcode == UNOP_DYNAMIC_CAST ? "dynamic_cast"
		      : "reinterpret_cast", stream);
      fputs_filtered ("<", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      fputs_filtered ("> (", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      fputs_filtered (")", stream);
      return;

    case UNOP_MEMVAL:
      (*pos) += 2;
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered ("(", stream);
      if (TYPE_CODE (exp->elts[pc + 1].type) == TYPE_CODE_FUNC
	  && exp->elts[pc + 3].opcode == OP_LONG)
	{
	  struct value_print_options opts;

	  /* We have a minimal symbol fn, probably.  It's encoded
	     as a UNOP_MEMVAL (function-type) of an OP_LONG (int, address).
	     Swallow the OP_LONG (including both its opcodes); ignore
	     its type; print the value in the type of the MEMVAL.  */
	  (*pos) += 4;
	  val = value_at_lazy (exp->elts[pc + 1].type,
			       (CORE_ADDR) exp->elts[pc + 5].longconst);
	  get_raw_print_options (&opts);
	  value_print (val, stream, &opts);
	}
      else
	{
	  fputs_filtered ("{", stream);
	  type_print (exp->elts[pc + 1].type, "", stream, 0);
	  fputs_filtered ("} ", stream);
	  print_subexp (exp, pos, stream, PREC_PREFIX);
	}
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered (")", stream);
      return;

    case UNOP_MEMVAL_TYPE:
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered ("(", stream);
      fputs_filtered ("{", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      fputs_filtered ("} ", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered (")", stream);
      return;

    case UNOP_MEMVAL_TLS:
      (*pos) += 3;
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered ("(", stream);
      fputs_filtered ("{", stream);
      type_print (exp->elts[pc + 2].type, "", stream, 0);
      fputs_filtered ("} ", stream);
      print_subexp (exp, pos, stream, PREC_PREFIX);
      if ((int) prec > (int) PREC_PREFIX)
	fputs_filtered (")", stream);
      return;

    case BINOP_ASSIGN_MODIFY:
      opcode = exp->elts[pc + 1].opcode;
      (*pos) += 2;
      myprec = PREC_ASSIGN;
      assoc = 1;
      assign_modify = 1;
      op_str = "???";
      for (tem = 0; op_print_tab[tem].opcode != OP_NULL; tem++)
	if (op_print_tab[tem].opcode == opcode)
	  {
	    op_str = op_print_tab[tem].string;
	    break;
	  }
      if (op_print_tab[tem].opcode != opcode)
	/* Not found; don't try to keep going because we don't know how
	   to interpret further elements.  */
	error (_("Invalid expression"));
      break;

      /* C++ ops */

    case OP_THIS:
      ++(*pos);
      if (exp->language_defn->la_name_of_this)
	fputs_filtered (exp->language_defn->la_name_of_this, stream);
      else
	fprintf_filtered (stream, _("<language %s has no 'this'>"),
			  exp->language_defn->la_name);
      return;

      /* Modula-2 ops */

    case MULTI_SUBSCRIPT:
      (*pos) += 2;
      nargs = longest_to_int (exp->elts[pc + 1].longconst);
      print_subexp (exp, pos, stream, PREC_SUFFIX);
      fprintf_unfiltered (stream, " [");
      for (tem = 0; tem < nargs; tem++)
	{
	  if (tem != 0)
	    fprintf_unfiltered (stream, ", ");
	  print_subexp (exp, pos, stream, PREC_ABOVE_COMMA);
	}
      fprintf_unfiltered (stream, "]");
      return;

    case BINOP_VAL:
      (*pos) += 2;
      fprintf_unfiltered (stream, "VAL(");
      type_print (exp->elts[pc + 1].type, "", stream, 0);
      fprintf_unfiltered (stream, ",");
      print_subexp (exp, pos, stream, PREC_PREFIX);
      fprintf_unfiltered (stream, ")");
      return;

    case TYPE_INSTANCE:
      {
	LONGEST count = exp->elts[pc + 1].longconst;

	/* The COUNT.  */
	(*pos)++;
	fputs_unfiltered ("TypesInstance(", stream);
	while (count-- > 0)
	  {
	    type_print (exp->elts[(*pos)++].type, "", stream, 0);
	    if (count > 0)
	      fputs_unfiltered (",", stream);
	  }
	fputs_unfiltered (",", stream);
	/* Ending COUNT and ending TYPE_INSTANCE.  */
	(*pos) += 2;
	print_subexp (exp, pos, stream, PREC_PREFIX);
	fputs_unfiltered (")", stream);
	return;
      }

      /* Default ops */

    default:
      op_str = "???";
      for (tem = 0; op_print_tab[tem].opcode != OP_NULL; tem++)
	if (op_print_tab[tem].opcode == opcode)
	  {
	    op_str = op_print_tab[tem].string;
	    myprec = op_print_tab[tem].precedence;
	    assoc = op_print_tab[tem].right_assoc;
	    break;
	  }
      if (op_print_tab[tem].opcode != opcode)
	/* Not found; don't try to keep going because we don't know how
	   to interpret further elements.  For example, this happens
	   if opcode is OP_TYPE.  */
	error (_("Invalid expression"));
    }

  /* Note that PREC_BUILTIN will always emit parentheses.  */
  if ((int) myprec < (int) prec)
    fputs_filtered ("(", stream);
  if ((int) opcode > (int) BINOP_END)
    {
      if (assoc)
	{
	  /* Unary postfix operator.  */
	  print_subexp (exp, pos, stream, PREC_SUFFIX);
	  fputs_filtered (op_str, stream);
	}
      else
	{
	  /* Unary prefix operator.  */
	  fputs_filtered (op_str, stream);
	  if (myprec == PREC_BUILTIN_FUNCTION)
	    fputs_filtered ("(", stream);
	  print_subexp (exp, pos, stream, PREC_PREFIX);
	  if (myprec == PREC_BUILTIN_FUNCTION)
	    fputs_filtered (")", stream);
	}
    }
  else
    {
      /* Binary operator.  */
      /* Print left operand.
         If operator is right-associative,
         increment precedence for this operand.  */
      print_subexp (exp, pos, stream,
		    (enum precedence) ((int) myprec + assoc));
      /* Print the operator itself.  */
      if (assign_modify)
	fprintf_filtered (stream, " %s= ", op_str);
      else if (op_str[0] == ',')
	fprintf_filtered (stream, "%s ", op_str);
      else
	fprintf_filtered (stream, " %s ", op_str);
      /* Print right operand.
         If operator is left-associative,
         increment precedence for this operand.  */
      print_subexp (exp, pos, stream,
		    (enum precedence) ((int) myprec + !assoc));
    }

  if ((int) myprec < (int) prec)
    fputs_filtered (")", stream);
}

/* Return the operator corresponding to opcode OP as
   a string.   NULL indicates that the opcode was not found in the
   current language table.  */
char *
op_string (enum exp_opcode op)
{
  int tem;
  const struct op_print *op_print_tab;

  op_print_tab = current_language->la_op_print_tab;
  for (tem = 0; op_print_tab[tem].opcode != OP_NULL; tem++)
    if (op_print_tab[tem].opcode == op)
      return op_print_tab[tem].string;
  return NULL;
}

/* Support for dumping the raw data from expressions in a human readable
   form.  */

static int dump_subexp_body (struct expression *exp, struct ui_file *, int);

/* Name for OPCODE, when it appears in expression EXP.  */

char *
op_name (struct expression *exp, enum exp_opcode opcode)
{
  return exp->language_defn->la_exp_desc->op_name (opcode);
}

/* Default name for the standard operator OPCODE (i.e., one defined in
   the definition of enum exp_opcode).  */

char *
op_name_standard (enum exp_opcode opcode)
{
  switch (opcode)
    {
    default:
      {
	static char buf[30];

	xsnprintf (buf, sizeof (buf), "<unknown %d>", opcode);
	return buf;
      }
#define OP(name)	\
    case name:		\
      return #name ;
#include "std-operator.def"
#undef OP
    }
}

/* Print a raw dump of expression EXP to STREAM.
   NOTE, if non-NULL, is printed as extra explanatory text.  */

void
dump_raw_expression (struct expression *exp, struct ui_file *stream,
		     char *note)
{
  int elt;
  char *opcode_name;
  char *eltscan;
  int eltsize;

  fprintf_filtered (stream, "Dump of expression @ ");
  gdb_print_host_address (exp, stream);
  if (note)
    fprintf_filtered (stream, ", %s:", note);
  fprintf_filtered (stream, "\n\tLanguage %s, %d elements, %ld bytes each.\n",
		    exp->language_defn->la_name, exp->nelts,
		    (long) sizeof (union exp_element));
  fprintf_filtered (stream, "\t%5s  %20s  %16s  %s\n", "Index", "Opcode",
		    "Hex Value", "String Value");
  for (elt = 0; elt < exp->nelts; elt++)
    {
      fprintf_filtered (stream, "\t%5d  ", elt);
      opcode_name = op_name (exp, exp->elts[elt].opcode);

      fprintf_filtered (stream, "%20s  ", opcode_name);
      print_longest (stream, 'd', 0, exp->elts[elt].longconst);
      fprintf_filtered (stream, "  ");

      for (eltscan = (char *) &exp->elts[elt],
	   eltsize = sizeof (union exp_element);
	   eltsize-- > 0;
	   eltscan++)
	{
	  fprintf_filtered (stream, "%c",
			    isprint (*eltscan) ? (*eltscan & 0xFF) : '.');
	}
      fprintf_filtered (stream, "\n");
    }
}

/* Dump the subexpression of prefix expression EXP whose operator is at
   position ELT onto STREAM.  Returns the position of the next 
   subexpression in EXP.  */

int
dump_subexp (struct expression *exp, struct ui_file *stream, int elt)
{
  static int indent = 0;
  int i;

  fprintf_filtered (stream, "\n");
  fprintf_filtered (stream, "\t%5d  ", elt);

  for (i = 1; i <= indent; i++)
    fprintf_filtered (stream, " ");
  indent += 2;

  fprintf_filtered (stream, "%-20s  ", op_name (exp, exp->elts[elt].opcode));

  elt = dump_subexp_body (exp, stream, elt);

  indent -= 2;

  return elt;
}

/* Dump the operands of prefix expression EXP whose opcode is at
   position ELT onto STREAM.  Returns the position of the next 
   subexpression in EXP.  */

static int
dump_subexp_body (struct expression *exp, struct ui_file *stream, int elt)
{
  return exp->language_defn->la_exp_desc->dump_subexp_body (exp, stream, elt);
}

/* Default value for subexp_body in exp_descriptor vector.  */

int
dump_subexp_body_standard (struct expression *exp, 
			   struct ui_file *stream, int elt)
{
  int opcode = exp->elts[elt++].opcode;

  switch (opcode)
    {
    case TERNOP_COND:
    case TERNOP_SLICE:
      elt = dump_subexp (exp, stream, elt);
      /* FALL THROUGH */
    case BINOP_ADD:
    case BINOP_SUB:
    case BINOP_MUL:
    case BINOP_DIV:
    case BINOP_REM:
    case BINOP_MOD:
    case BINOP_LSH:
    case BINOP_RSH:
    case BINOP_LOGICAL_AND:
    case BINOP_LOGICAL_OR:
    case BINOP_BITWISE_AND:
    case BINOP_BITWISE_IOR:
    case BINOP_BITWISE_XOR:
    case BINOP_EQUAL:
    case BINOP_NOTEQUAL:
    case BINOP_LESS:
    case BINOP_GTR:
    case BINOP_LEQ:
    case BINOP_GEQ:
    case BINOP_REPEAT:
    case BINOP_ASSIGN:
    case BINOP_COMMA:
    case BINOP_SUBSCRIPT:
    case BINOP_EXP:
    case BINOP_MIN:
    case BINOP_MAX:
    case BINOP_INTDIV:
    case BINOP_ASSIGN_MODIFY:
    case BINOP_VAL:
    case BINOP_CONCAT:
    case BINOP_IN:
    case BINOP_RANGE:
    case BINOP_END:
    case STRUCTOP_MEMBER:
    case STRUCTOP_MPTR:
      elt = dump_subexp (exp, stream, elt);
      /* FALL THROUGH */
    case UNOP_NEG:
    case UNOP_LOGICAL_NOT:
    case UNOP_COMPLEMENT:
    case UNOP_IND:
    case UNOP_ADDR:
    case UNOP_PREINCREMENT:
    case UNOP_POSTINCREMENT:
    case UNOP_PREDECREMENT:
    case UNOP_POSTDECREMENT:
    case UNOP_SIZEOF:
    case UNOP_PLUS:
    case UNOP_CAP:
    case UNOP_CHR:
    case UNOP_ORD:
    case UNOP_ABS:
    case UNOP_FLOAT:
    case UNOP_HIGH:
    case UNOP_MAX:
    case UNOP_MIN:
    case UNOP_ODD:
    case UNOP_TRUNC:
      elt = dump_subexp (exp, stream, elt);
      break;
    case OP_LONG:
      fprintf_filtered (stream, "Type @");
      gdb_print_host_address (exp->elts[elt].type, stream);
      fprintf_filtered (stream, " (");
      type_print (exp->elts[elt].type, NULL, stream, 0);
      fprintf_filtered (stream, "), value %ld (0x%lx)",
			(long) exp->elts[elt + 1].longconst,
			(long) exp->elts[elt + 1].longconst);
      elt += 3;
      break;
    case OP_DOUBLE:
      fprintf_filtered (stream, "Type @");
      gdb_print_host_address (exp->elts[elt].type, stream);
      fprintf_filtered (stream, " (");
      type_print (exp->elts[elt].type, NULL, stream, 0);
      fprintf_filtered (stream, "), value %g",
			(double) exp->elts[elt + 1].doubleconst);
      elt += 3;
      break;
    case OP_VAR_VALUE:
      fprintf_filtered (stream, "Block @");
      gdb_print_host_address (exp->elts[elt].block, stream);
      fprintf_filtered (stream, ", symbol @");
      gdb_print_host_address (exp->elts[elt + 1].symbol, stream);
      fprintf_filtered (stream, " (%s)",
			SYMBOL_PRINT_NAME (exp->elts[elt + 1].symbol));
      elt += 3;
      break;
    case OP_VAR_ENTRY_VALUE:
      fprintf_filtered (stream, "Entry value of symbol @");
      gdb_print_host_address (exp->elts[elt].symbol, stream);
      fprintf_filtered (stream, " (%s)",
			SYMBOL_PRINT_NAME (exp->elts[elt].symbol));
      elt += 2;
      break;
    case OP_LAST:
      fprintf_filtered (stream, "History element %ld",
			(long) exp->elts[elt].longconst);
      elt += 2;
      break;
    case OP_REGISTER:
      fprintf_filtered (stream, "Register $%s", &exp->elts[elt + 1].string);
      elt += 3 + BYTES_TO_EXP_ELEM (exp->elts[elt].longconst + 1);
      break;
    case OP_INTERNALVAR:
      fprintf_filtered (stream, "Internal var @");
      gdb_print_host_address (exp->elts[elt].internalvar, stream);
      fprintf_filtered (stream, " (%s)",
			internalvar_name (exp->elts[elt].internalvar));
      elt += 2;
      break;
    case OP_FUNCALL:
      {
	int i, nargs;

	nargs = longest_to_int (exp->elts[elt].longconst);

	fprintf_filtered (stream, "Number of args: %d", nargs);
	elt += 2;

	for (i = 1; i <= nargs + 1; i++)
	  elt = dump_subexp (exp, stream, elt);
      }
      break;
    case OP_ARRAY:
      {
	int lower, upper;
	int i;

	lower = longest_to_int (exp->elts[elt].longconst);
	upper = longest_to_int (exp->elts[elt + 1].longconst);

	fprintf_filtered (stream, "Bounds [%d:%d]", lower, upper);
	elt += 3;

	for (i = 1; i <= upper - lower + 1; i++)
	  elt = dump_subexp (exp, stream, elt);
      }
      break;
    case UNOP_DYNAMIC_CAST:
    case UNOP_REINTERPRET_CAST:
    case UNOP_CAST_TYPE:
    case UNOP_MEMVAL_TYPE:
      fprintf_filtered (stream, " (");
      elt = dump_subexp (exp, stream, elt);
      fprintf_filtered (stream, ")");
      elt = dump_subexp (exp, stream, elt);
      break;
    case UNOP_MEMVAL:
    case UNOP_CAST:
      fprintf_filtered (stream, "Type @");
      gdb_print_host_address (exp->elts[elt].type, stream);
      fprintf_filtered (stream, " (");
      type_print (exp->elts[elt].type, NULL, stream, 0);
      fprintf_filtered (stream, ")");
      elt = dump_subexp (exp, stream, elt + 2);
      break;
    case UNOP_MEMVAL_TLS:
      fprintf_filtered (stream, "TLS type @");
      gdb_print_host_address (exp->elts[elt + 1].type, stream);
      fprintf_filtered (stream, " (__thread /* \"%s\" */ ",
                        (exp->elts[elt].objfile == NULL ? "(null)"
			 : exp->elts[elt].objfile->name));
      type_print (exp->elts[elt + 1].type, NULL, stream, 0);
      fprintf_filtered (stream, ")");
      elt = dump_subexp (exp, stream, elt + 3);
      break;
    case OP_TYPE:
      fprintf_filtered (stream, "Type @");
      gdb_print_host_address (exp->elts[elt].type, stream);
      fprintf_filtered (stream, " (");
      type_print (exp->elts[elt].type, NULL, stream, 0);
      fprintf_filtered (stream, ")");
      elt += 2;
      break;
    case OP_TYPEOF:
    case OP_DECLTYPE:
      fprintf_filtered (stream, "Typeof (");
      elt = dump_subexp (exp, stream, elt);
      fprintf_filtered (stream, ")");
      break;
    case STRUCTOP_STRUCT:
    case STRUCTOP_PTR:
      {
	char *elem_name;
	int len;

	len = longest_to_int (exp->elts[elt].longconst);
	elem_name = &exp->elts[elt + 1].string;

	fprintf_filtered (stream, "Element name: `%.*s'", len, elem_name);
	elt = dump_subexp (exp, stream, elt + 3 + BYTES_TO_EXP_ELEM (len + 1));
      }
      break;
    case OP_SCOPE:
      {
	char *elem_name;
	int len;

	fprintf_filtered (stream, "Type @");
	gdb_print_host_address (exp->elts[elt].type, stream);
	fprintf_filtered (stream, " (");
	type_print (exp->elts[elt].type, NULL, stream, 0);
	fprintf_filtered (stream, ") ");

	len = longest_to_int (exp->elts[elt + 1].longconst);
	elem_name = &exp->elts[elt + 2].string;

	fprintf_filtered (stream, "Field name: `%.*s'", len, elem_name);
	elt += 4 + BYTES_TO_EXP_ELEM (len + 1);
      }
      break;
    case TYPE_INSTANCE:
      {
	LONGEST len;

	len = exp->elts[elt++].longconst;
	fprintf_filtered (stream, "%s TypeInstance: ", plongest (len));
	while (len-- > 0)
	  {
	    fprintf_filtered (stream, "Type @");
	    gdb_print_host_address (exp->elts[elt].type, stream);
	    fprintf_filtered (stream, " (");
	    type_print (exp->elts[elt].type, NULL, stream, 0);
	    fprintf_filtered (stream, ")");
	    elt++;
	    if (len > 0)
	      fputs_filtered (", ", stream);
	  }
	/* Ending LEN and ending TYPE_INSTANCE.  */
	elt += 2;
	elt = dump_subexp (exp, stream, elt);
      }
      break;
    default:
    case OP_NULL:
    case MULTI_SUBSCRIPT:
    case OP_F77_UNDETERMINED_ARGLIST:
    case OP_COMPLEX:
    case OP_STRING:
    case OP_BOOL:
    case OP_M2_STRING:
    case OP_THIS:
    case OP_NAME:
      fprintf_filtered (stream, "Unknown format");
    }

  return elt;
}

void
dump_prefix_expression (struct expression *exp, struct ui_file *stream)
{
  int elt;

  fprintf_filtered (stream, "Dump of expression @ ");
  gdb_print_host_address (exp, stream);
  fputs_filtered (", after conversion to prefix form:\nExpression: `", stream);
  print_expression (exp, stream);
  fprintf_filtered (stream, "'\n\tLanguage %s, %d elements, %ld bytes each.\n",
		    exp->language_defn->la_name, exp->nelts,
		    (long) sizeof (union exp_element));
  fputs_filtered ("\n", stream);

  for (elt = 0; elt < exp->nelts;)
    elt = dump_subexp (exp, stream, elt);
  fputs_filtered ("\n", stream);
}
