/* Parse expressions for GDB.

   Copyright (C) 1986-2020 Free Software Foundation, Inc.

   Modified from expread.y by the Department of Computer Science at the
   State University of New York at Buffalo, 1991.

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

/* Parse an expression from text in a string,
   and return the result as a struct expression pointer.
   That structure contains arithmetic operations in reverse polish,
   with constants represented by operations that are followed by special data.
   See expression.h for the details of the format.
   What is important here is that it can be built up sequentially
   during the process of parsing; the lower levels of the tree always
   come first in the result.  */

#include "defs.h"
#include <ctype.h>
#include "arch-utils.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "expression.h"
#include "value.h"
#include "command.h"
#include "language.h"
#include "f-lang.h"
#include "parser-defs.h"
#include "gdbcmd.h"
#include "symfile.h"		/* for overlay functions */
#include "inferior.h"
#include "target-float.h"
#include "block.h"
#include "source.h"
#include "objfiles.h"
#include "user-regs.h"
#include <algorithm>
#include "gdbsupport/gdb_optional.h"

/* Standard set of definitions for printing, dumping, prefixifying,
 * and evaluating expressions.  */

const struct exp_descriptor exp_descriptor_standard = 
  {
    print_subexp_standard,
    operator_length_standard,
    operator_check_standard,
    op_name_standard,
    dump_subexp_body_standard,
    evaluate_subexp_standard
  };

static unsigned int expressiondebug = 0;
static void
show_expressiondebug (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Expression debugging is %s.\n"), value);
}


/* True if an expression parser should set yydebug.  */
bool parser_debug;

static void
show_parserdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Parser debugging is %s.\n"), value);
}


static int prefixify_subexp (struct expression *, struct expression *, int,
			     int, int);

static expression_up parse_exp_in_context (const char **, CORE_ADDR,
					   const struct block *, int,
					   int, int *,
					   innermost_block_tracker *,
					   expr_completion_state *);

static void increase_expout_size (struct expr_builder *ps, size_t lenelt);


/* Documented at it's declaration.  */

void
innermost_block_tracker::update (const struct block *b,
				 innermost_block_tracker_types t)
{
  if ((m_types & t) != 0
      && (m_innermost_block == NULL
	  || contained_in (b, m_innermost_block)))
    m_innermost_block = b;
}



/* See definition in parser-defs.h.  */

expr_builder::expr_builder (const struct language_defn *lang,
			    struct gdbarch *gdbarch)
  : expout_size (10),
    expout (XNEWVAR (expression,
		     (sizeof (expression)
		      + EXP_ELEM_TO_BYTES (expout_size)))),
    expout_ptr (0)
{
  expout->language_defn = lang;
  expout->gdbarch = gdbarch;
}

expression_up
expr_builder::release ()
{
  /* Record the actual number of expression elements, and then
     reallocate the expression memory so that we free up any
     excess elements.  */

  expout->nelts = expout_ptr;
  expout.reset (XRESIZEVAR (expression, expout.release (),
			    (sizeof (expression)
			     + EXP_ELEM_TO_BYTES (expout_ptr))));

  return std::move (expout);
}

/* This page contains the functions for adding data to the struct expression
   being constructed.  */

/* Add one element to the end of the expression.  */

/* To avoid a bug in the Sun 4 compiler, we pass things that can fit into
   a register through here.  */

static void
write_exp_elt (struct expr_builder *ps, const union exp_element *expelt)
{
  if (ps->expout_ptr >= ps->expout_size)
    {
      ps->expout_size *= 2;
      ps->expout.reset (XRESIZEVAR (expression, ps->expout.release (),
				    (sizeof (expression)
				     + EXP_ELEM_TO_BYTES (ps->expout_size))));
    }
  ps->expout->elts[ps->expout_ptr++] = *expelt;
}

void
write_exp_elt_opcode (struct expr_builder *ps, enum exp_opcode expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.opcode = expelt;
  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_sym (struct expr_builder *ps, struct symbol *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.symbol = expelt;
  write_exp_elt (ps, &tmp);
}

static void
write_exp_elt_msym (struct expr_builder *ps, minimal_symbol *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.msymbol = expelt;
  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_block (struct expr_builder *ps, const struct block *b)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.block = b;
  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_objfile (struct expr_builder *ps, struct objfile *objfile)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.objfile = objfile;
  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_longcst (struct expr_builder *ps, LONGEST expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.longconst = expelt;
  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_floatcst (struct expr_builder *ps, const gdb_byte expelt[16])
{
  union exp_element tmp;
  int index;

  for (index = 0; index < 16; index++)
    tmp.floatconst[index] = expelt[index];

  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_type (struct expr_builder *ps, struct type *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.type = expelt;
  write_exp_elt (ps, &tmp);
}

void
write_exp_elt_intern (struct expr_builder *ps, struct internalvar *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.internalvar = expelt;
  write_exp_elt (ps, &tmp);
}

/* Add a string constant to the end of the expression.

   String constants are stored by first writing an expression element
   that contains the length of the string, then stuffing the string
   constant itself into however many expression elements are needed
   to hold it, and then writing another expression element that contains
   the length of the string.  I.e. an expression element at each end of
   the string records the string length, so you can skip over the 
   expression elements containing the actual string bytes from either
   end of the string.  Note that this also allows gdb to handle
   strings with embedded null bytes, as is required for some languages.

   Don't be fooled by the fact that the string is null byte terminated,
   this is strictly for the convenience of debugging gdb itself.
   Gdb does not depend up the string being null terminated, since the
   actual length is recorded in expression elements at each end of the
   string.  The null byte is taken into consideration when computing how
   many expression elements are required to hold the string constant, of
   course.  */


void
write_exp_string (struct expr_builder *ps, struct stoken str)
{
  int len = str.length;
  size_t lenelt;
  char *strdata;

  /* Compute the number of expression elements required to hold the string
     (including a null byte terminator), along with one expression element
     at each end to record the actual string length (not including the
     null byte terminator).  */

  lenelt = 2 + BYTES_TO_EXP_ELEM (len + 1);

  increase_expout_size (ps, lenelt);

  /* Write the leading length expression element (which advances the current
     expression element index), then write the string constant followed by a
     terminating null byte, and then write the trailing length expression
     element.  */

  write_exp_elt_longcst (ps, (LONGEST) len);
  strdata = (char *) &ps->expout->elts[ps->expout_ptr];
  memcpy (strdata, str.ptr, len);
  *(strdata + len) = '\0';
  ps->expout_ptr += lenelt - 2;
  write_exp_elt_longcst (ps, (LONGEST) len);
}

/* Add a vector of string constants to the end of the expression.

   This adds an OP_STRING operation, but encodes the contents
   differently from write_exp_string.  The language is expected to
   handle evaluation of this expression itself.
   
   After the usual OP_STRING header, TYPE is written into the
   expression as a long constant.  The interpretation of this field is
   up to the language evaluator.
   
   Next, each string in VEC is written.  The length is written as a
   long constant, followed by the contents of the string.  */

void
write_exp_string_vector (struct expr_builder *ps, int type,
			 struct stoken_vector *vec)
{
  int i, len;
  size_t n_slots;

  /* Compute the size.  We compute the size in number of slots to
     avoid issues with string padding.  */
  n_slots = 0;
  for (i = 0; i < vec->len; ++i)
    {
      /* One slot for the length of this element, plus the number of
	 slots needed for this string.  */
      n_slots += 1 + BYTES_TO_EXP_ELEM (vec->tokens[i].length);
    }

  /* One more slot for the type of the string.  */
  ++n_slots;

  /* Now compute a phony string length.  */
  len = EXP_ELEM_TO_BYTES (n_slots) - 1;

  n_slots += 4;
  increase_expout_size (ps, n_slots);

  write_exp_elt_opcode (ps, OP_STRING);
  write_exp_elt_longcst (ps, len);
  write_exp_elt_longcst (ps, type);

  for (i = 0; i < vec->len; ++i)
    {
      write_exp_elt_longcst (ps, vec->tokens[i].length);
      memcpy (&ps->expout->elts[ps->expout_ptr], vec->tokens[i].ptr,
	      vec->tokens[i].length);
      ps->expout_ptr += BYTES_TO_EXP_ELEM (vec->tokens[i].length);
    }

  write_exp_elt_longcst (ps, len);
  write_exp_elt_opcode (ps, OP_STRING);
}

/* Add a bitstring constant to the end of the expression.

   Bitstring constants are stored by first writing an expression element
   that contains the length of the bitstring (in bits), then stuffing the
   bitstring constant itself into however many expression elements are
   needed to hold it, and then writing another expression element that
   contains the length of the bitstring.  I.e. an expression element at
   each end of the bitstring records the bitstring length, so you can skip
   over the expression elements containing the actual bitstring bytes from
   either end of the bitstring.  */

void
write_exp_bitstring (struct expr_builder *ps, struct stoken str)
{
  int bits = str.length;	/* length in bits */
  int len = (bits + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
  size_t lenelt;
  char *strdata;

  /* Compute the number of expression elements required to hold the bitstring,
     along with one expression element at each end to record the actual
     bitstring length in bits.  */

  lenelt = 2 + BYTES_TO_EXP_ELEM (len);

  increase_expout_size (ps, lenelt);

  /* Write the leading length expression element (which advances the current
     expression element index), then write the bitstring constant, and then
     write the trailing length expression element.  */

  write_exp_elt_longcst (ps, (LONGEST) bits);
  strdata = (char *) &ps->expout->elts[ps->expout_ptr];
  memcpy (strdata, str.ptr, len);
  ps->expout_ptr += lenelt - 2;
  write_exp_elt_longcst (ps, (LONGEST) bits);
}

/* Return the type of MSYMBOL, a minimal symbol of OBJFILE.  If
   ADDRESS_P is not NULL, set it to the MSYMBOL's resolved
   address.  */

type *
find_minsym_type_and_address (minimal_symbol *msymbol,
			      struct objfile *objfile,
			      CORE_ADDR *address_p)
{
  bound_minimal_symbol bound_msym = {msymbol, objfile};
  struct obj_section *section = MSYMBOL_OBJ_SECTION (objfile, msymbol);
  enum minimal_symbol_type type = MSYMBOL_TYPE (msymbol);

  bool is_tls = (section != NULL
		 && section->the_bfd_section->flags & SEC_THREAD_LOCAL);

  /* The minimal symbol might point to a function descriptor;
     resolve it to the actual code address instead.  */
  CORE_ADDR addr;
  if (is_tls)
    {
      /* Addresses of TLS symbols are really offsets into a
	 per-objfile/per-thread storage block.  */
      addr = MSYMBOL_VALUE_RAW_ADDRESS (bound_msym.minsym);
    }
  else if (msymbol_is_function (objfile, msymbol, &addr))
    {
      if (addr != BMSYMBOL_VALUE_ADDRESS (bound_msym))
	{
	  /* This means we resolved a function descriptor, and we now
	     have an address for a code/text symbol instead of a data
	     symbol.  */
	  if (MSYMBOL_TYPE (msymbol) == mst_data_gnu_ifunc)
	    type = mst_text_gnu_ifunc;
	  else
	    type = mst_text;
	  section = NULL;
	}
    }
  else
    addr = BMSYMBOL_VALUE_ADDRESS (bound_msym);

  if (overlay_debugging)
    addr = symbol_overlayed_address (addr, section);

  if (is_tls)
    {
      /* Skip translation if caller does not need the address.  */
      if (address_p != NULL)
	*address_p = target_translate_tls_address (objfile, addr);
      return objfile_type (objfile)->nodebug_tls_symbol;
    }

  if (address_p != NULL)
    *address_p = addr;

  switch (type)
    {
    case mst_text:
    case mst_file_text:
    case mst_solib_trampoline:
      return objfile_type (objfile)->nodebug_text_symbol;

    case mst_text_gnu_ifunc:
      return objfile_type (objfile)->nodebug_text_gnu_ifunc_symbol;

    case mst_data:
    case mst_file_data:
    case mst_bss:
    case mst_file_bss:
      return objfile_type (objfile)->nodebug_data_symbol;

    case mst_slot_got_plt:
      return objfile_type (objfile)->nodebug_got_plt_symbol;

    default:
      return objfile_type (objfile)->nodebug_unknown_symbol;
    }
}

/* Add the appropriate elements for a minimal symbol to the end of
   the expression.  */

void
write_exp_msymbol (struct expr_builder *ps,
		   struct bound_minimal_symbol bound_msym)
{
  write_exp_elt_opcode (ps, OP_VAR_MSYM_VALUE);
  write_exp_elt_objfile (ps, bound_msym.objfile);
  write_exp_elt_msym (ps, bound_msym.minsym);
  write_exp_elt_opcode (ps, OP_VAR_MSYM_VALUE);
}

/* See parser-defs.h.  */

void
parser_state::mark_struct_expression ()
{
  gdb_assert (parse_completion
	      && (m_completion_state.expout_tag_completion_type
		  == TYPE_CODE_UNDEF));
  m_completion_state.expout_last_struct = expout_ptr;
}

/* Indicate that the current parser invocation is completing a tag.
   TAG is the type code of the tag, and PTR and LENGTH represent the
   start of the tag name.  */

void
parser_state::mark_completion_tag (enum type_code tag, const char *ptr,
				   int length)
{
  gdb_assert (parse_completion
	      && (m_completion_state.expout_tag_completion_type
		  == TYPE_CODE_UNDEF)
	      && m_completion_state.expout_completion_name == NULL
	      && m_completion_state.expout_last_struct == -1);
  gdb_assert (tag == TYPE_CODE_UNION
	      || tag == TYPE_CODE_STRUCT
	      || tag == TYPE_CODE_ENUM);
  m_completion_state.expout_tag_completion_type = tag;
  m_completion_state.expout_completion_name.reset (xstrndup (ptr, length));
}


/* Recognize tokens that start with '$'.  These include:

   $regname     A native register name or a "standard
   register name".

   $variable    A convenience variable with a name chosen
   by the user.

   $digits              Value history with index <digits>, starting
   from the first value which has index 1.

   $$digits     Value history with index <digits> relative
   to the last value.  I.e. $$0 is the last
   value, $$1 is the one previous to that, $$2
   is the one previous to $$1, etc.

   $ | $0 | $$0 The last value in the value history.

   $$           An abbreviation for the second to the last
   value in the value history, I.e. $$1  */

void
write_dollar_variable (struct parser_state *ps, struct stoken str)
{
  struct block_symbol sym;
  struct bound_minimal_symbol msym;
  struct internalvar *isym = NULL;
  std::string copy;

  /* Handle the tokens $digits; also $ (short for $0) and $$ (short for $$1)
     and $$digits (equivalent to $<-digits> if you could type that).  */

  int negate = 0;
  int i = 1;
  /* Double dollar means negate the number and add -1 as well.
     Thus $$ alone means -1.  */
  if (str.length >= 2 && str.ptr[1] == '$')
    {
      negate = 1;
      i = 2;
    }
  if (i == str.length)
    {
      /* Just dollars (one or two).  */
      i = -negate;
      goto handle_last;
    }
  /* Is the rest of the token digits?  */
  for (; i < str.length; i++)
    if (!(str.ptr[i] >= '0' && str.ptr[i] <= '9'))
      break;
  if (i == str.length)
    {
      i = atoi (str.ptr + 1 + negate);
      if (negate)
	i = -i;
      goto handle_last;
    }

  /* Handle tokens that refer to machine registers:
     $ followed by a register name.  */
  i = user_reg_map_name_to_regnum (ps->gdbarch (),
				   str.ptr + 1, str.length - 1);
  if (i >= 0)
    goto handle_register;

  /* Any names starting with $ are probably debugger internal variables.  */

  copy = copy_name (str);
  isym = lookup_only_internalvar (copy.c_str () + 1);
  if (isym)
    {
      write_exp_elt_opcode (ps, OP_INTERNALVAR);
      write_exp_elt_intern (ps, isym);
      write_exp_elt_opcode (ps, OP_INTERNALVAR);
      return;
    }

  /* On some systems, such as HP-UX and hppa-linux, certain system routines 
     have names beginning with $ or $$.  Check for those, first.  */

  sym = lookup_symbol (copy.c_str (), NULL, VAR_DOMAIN, NULL);
  if (sym.symbol)
    {
      write_exp_elt_opcode (ps, OP_VAR_VALUE);
      write_exp_elt_block (ps, sym.block);
      write_exp_elt_sym (ps, sym.symbol);
      write_exp_elt_opcode (ps, OP_VAR_VALUE);
      return;
    }
  msym = lookup_bound_minimal_symbol (copy.c_str ());
  if (msym.minsym)
    {
      write_exp_msymbol (ps, msym);
      return;
    }

  /* Any other names are assumed to be debugger internal variables.  */

  write_exp_elt_opcode (ps, OP_INTERNALVAR);
  write_exp_elt_intern (ps, create_internalvar (copy.c_str () + 1));
  write_exp_elt_opcode (ps, OP_INTERNALVAR);
  return;
handle_last:
  write_exp_elt_opcode (ps, OP_LAST);
  write_exp_elt_longcst (ps, (LONGEST) i);
  write_exp_elt_opcode (ps, OP_LAST);
  return;
handle_register:
  write_exp_elt_opcode (ps, OP_REGISTER);
  str.length--;
  str.ptr++;
  write_exp_string (ps, str);
  write_exp_elt_opcode (ps, OP_REGISTER);
  ps->block_tracker->update (ps->expression_context_block,
			     INNERMOST_BLOCK_FOR_REGISTERS);
  return;
}


const char *
find_template_name_end (const char *p)
{
  int depth = 1;
  int just_seen_right = 0;
  int just_seen_colon = 0;
  int just_seen_space = 0;

  if (!p || (*p != '<'))
    return 0;

  while (*++p)
    {
      switch (*p)
	{
	case '\'':
	case '\"':
	case '{':
	case '}':
	  /* In future, may want to allow these??  */
	  return 0;
	case '<':
	  depth++;		/* start nested template */
	  if (just_seen_colon || just_seen_right || just_seen_space)
	    return 0;		/* but not after : or :: or > or space */
	  break;
	case '>':
	  if (just_seen_colon || just_seen_right)
	    return 0;		/* end a (nested?) template */
	  just_seen_right = 1;	/* but not after : or :: */
	  if (--depth == 0)	/* also disallow >>, insist on > > */
	    return ++p;		/* if outermost ended, return */
	  break;
	case ':':
	  if (just_seen_space || (just_seen_colon > 1))
	    return 0;		/* nested class spec coming up */
	  just_seen_colon++;	/* we allow :: but not :::: */
	  break;
	case ' ':
	  break;
	default:
	  if (!((*p >= 'a' && *p <= 'z') ||	/* allow token chars */
		(*p >= 'A' && *p <= 'Z') ||
		(*p >= '0' && *p <= '9') ||
		(*p == '_') || (*p == ',') ||	/* commas for template args */
		(*p == '&') || (*p == '*') ||	/* pointer and ref types */
		(*p == '(') || (*p == ')') ||	/* function types */
		(*p == '[') || (*p == ']')))	/* array types */
	    return 0;
	}
      if (*p != ' ')
	just_seen_space = 0;
      if (*p != ':')
	just_seen_colon = 0;
      if (*p != '>')
	just_seen_right = 0;
    }
  return 0;
}


/* Return a null-terminated temporary copy of the name of a string token.

   Tokens that refer to names do so with explicit pointer and length,
   so they can share the storage that lexptr is parsing.
   When it is necessary to pass a name to a function that expects
   a null-terminated string, the substring is copied out
   into a separate block of storage.  */

std::string
copy_name (struct stoken token)
{
  return std::string (token.ptr, token.length);
}


/* See comments on parser-defs.h.  */

int
prefixify_expression (struct expression *expr, int last_struct)
{
  gdb_assert (expr->nelts > 0);
  int len = sizeof (struct expression) + EXP_ELEM_TO_BYTES (expr->nelts);
  struct expression *temp;
  int inpos = expr->nelts, outpos = 0;

  temp = (struct expression *) alloca (len);

  /* Copy the original expression into temp.  */
  memcpy (temp, expr, len);

  return prefixify_subexp (temp, expr, inpos, outpos, last_struct);
}

/* Return the number of exp_elements in the postfix subexpression 
   of EXPR whose operator is at index ENDPOS - 1 in EXPR.  */

static int
length_of_subexp (struct expression *expr, int endpos)
{
  int oplen, args;

  operator_length (expr, endpos, &oplen, &args);

  while (args > 0)
    {
      oplen += length_of_subexp (expr, endpos - oplen);
      args--;
    }

  return oplen;
}

/* Sets *OPLENP to the length of the operator whose (last) index is 
   ENDPOS - 1 in EXPR, and sets *ARGSP to the number of arguments that
   operator takes.  */

void
operator_length (const struct expression *expr, int endpos, int *oplenp,
		 int *argsp)
{
  expr->language_defn->la_exp_desc->operator_length (expr, endpos,
						     oplenp, argsp);
}

/* Default value for operator_length in exp_descriptor vectors.  */

void
operator_length_standard (const struct expression *expr, int endpos,
			  int *oplenp, int *argsp)
{
  int oplen = 1;
  int args = 0;
  enum range_type range_type;
  int i;

  if (endpos < 1)
    error (_("?error in operator_length_standard"));

  i = (int) expr->elts[endpos - 1].opcode;

  switch (i)
    {
      /* C++  */
    case OP_SCOPE:
      oplen = longest_to_int (expr->elts[endpos - 2].longconst);
      oplen = 5 + BYTES_TO_EXP_ELEM (oplen + 1);
      break;

    case OP_LONG:
    case OP_FLOAT:
    case OP_VAR_VALUE:
    case OP_VAR_MSYM_VALUE:
      oplen = 4;
      break;

    case OP_FUNC_STATIC_VAR:
      oplen = longest_to_int (expr->elts[endpos - 2].longconst);
      oplen = 4 + BYTES_TO_EXP_ELEM (oplen + 1);
      args = 1;
      break;

    case OP_TYPE:
    case OP_BOOL:
    case OP_LAST:
    case OP_INTERNALVAR:
    case OP_VAR_ENTRY_VALUE:
      oplen = 3;
      break;

    case OP_COMPLEX:
      oplen = 3;
      args = 2;
      break;

    case OP_FUNCALL:
    case OP_F77_UNDETERMINED_ARGLIST:
      oplen = 3;
      args = 1 + longest_to_int (expr->elts[endpos - 2].longconst);
      break;

    case TYPE_INSTANCE:
      oplen = 5 + longest_to_int (expr->elts[endpos - 2].longconst);
      args = 1;
      break;

    case OP_OBJC_MSGCALL:	/* Objective C message (method) call.  */
      oplen = 4;
      args = 1 + longest_to_int (expr->elts[endpos - 2].longconst);
      break;

    case UNOP_MAX:
    case UNOP_MIN:
      oplen = 3;
      break;

    case UNOP_CAST_TYPE:
    case UNOP_DYNAMIC_CAST:
    case UNOP_REINTERPRET_CAST:
    case UNOP_MEMVAL_TYPE:
      oplen = 1;
      args = 2;
      break;

    case BINOP_VAL:
    case UNOP_CAST:
    case UNOP_MEMVAL:
      oplen = 3;
      args = 1;
      break;

    case UNOP_ABS:
    case UNOP_CAP:
    case UNOP_CHR:
    case UNOP_FLOAT:
    case UNOP_HIGH:
    case UNOP_ODD:
    case UNOP_ORD:
    case UNOP_TRUNC:
    case OP_TYPEOF:
    case OP_DECLTYPE:
    case OP_TYPEID:
      oplen = 1;
      args = 1;
      break;

    case OP_ADL_FUNC:
      oplen = longest_to_int (expr->elts[endpos - 2].longconst);
      oplen = 4 + BYTES_TO_EXP_ELEM (oplen + 1);
      oplen++;
      oplen++;
      break;

    case STRUCTOP_STRUCT:
    case STRUCTOP_PTR:
      args = 1;
      /* fall through */
    case OP_REGISTER:
    case OP_M2_STRING:
    case OP_STRING:
    case OP_OBJC_NSSTRING:	/* Objective C Foundation Class
				   NSString constant.  */
    case OP_OBJC_SELECTOR:	/* Objective C "@selector" pseudo-op.  */
    case OP_NAME:
      oplen = longest_to_int (expr->elts[endpos - 2].longconst);
      oplen = 4 + BYTES_TO_EXP_ELEM (oplen + 1);
      break;

    case OP_ARRAY:
      oplen = 4;
      args = longest_to_int (expr->elts[endpos - 2].longconst);
      args -= longest_to_int (expr->elts[endpos - 3].longconst);
      args += 1;
      break;

    case TERNOP_COND:
    case TERNOP_SLICE:
      args = 3;
      break;

      /* Modula-2 */
    case MULTI_SUBSCRIPT:
      oplen = 3;
      args = 1 + longest_to_int (expr->elts[endpos - 2].longconst);
      break;

    case BINOP_ASSIGN_MODIFY:
      oplen = 3;
      args = 2;
      break;

      /* C++ */
    case OP_THIS:
      oplen = 2;
      break;

    case OP_RANGE:
      oplen = 3;
      range_type = (enum range_type)
	longest_to_int (expr->elts[endpos - 2].longconst);

      switch (range_type)
	{
	case LOW_BOUND_DEFAULT:
	case LOW_BOUND_DEFAULT_EXCLUSIVE:
	case HIGH_BOUND_DEFAULT:
	  args = 1;
	  break;
	case BOTH_BOUND_DEFAULT:
	  args = 0;
	  break;
	case NONE_BOUND_DEFAULT:
	case NONE_BOUND_DEFAULT_EXCLUSIVE:
	  args = 2;
	  break;
	}

      break;

    default:
      args = 1 + (i < (int) BINOP_END);
    }

  *oplenp = oplen;
  *argsp = args;
}

/* Copy the subexpression ending just before index INEND in INEXPR
   into OUTEXPR, starting at index OUTBEG.
   In the process, convert it from suffix to prefix form.
   If LAST_STRUCT is -1, then this function always returns -1.
   Otherwise, it returns the index of the subexpression which is the
   left-hand-side of the expression at LAST_STRUCT.  */

static int
prefixify_subexp (struct expression *inexpr,
		  struct expression *outexpr, int inend, int outbeg,
		  int last_struct)
{
  int oplen;
  int args;
  int i;
  int *arglens;
  int result = -1;

  operator_length (inexpr, inend, &oplen, &args);

  /* Copy the final operator itself, from the end of the input
     to the beginning of the output.  */
  inend -= oplen;
  memcpy (&outexpr->elts[outbeg], &inexpr->elts[inend],
	  EXP_ELEM_TO_BYTES (oplen));
  outbeg += oplen;

  if (last_struct == inend)
    result = outbeg - oplen;

  /* Find the lengths of the arg subexpressions.  */
  arglens = (int *) alloca (args * sizeof (int));
  for (i = args - 1; i >= 0; i--)
    {
      oplen = length_of_subexp (inexpr, inend);
      arglens[i] = oplen;
      inend -= oplen;
    }

  /* Now copy each subexpression, preserving the order of
     the subexpressions, but prefixifying each one.
     In this loop, inend starts at the beginning of
     the expression this level is working on
     and marches forward over the arguments.
     outbeg does similarly in the output.  */
  for (i = 0; i < args; i++)
    {
      int r;

      oplen = arglens[i];
      inend += oplen;
      r = prefixify_subexp (inexpr, outexpr, inend, outbeg, last_struct);
      if (r != -1)
	{
	  /* Return immediately.  We probably have only parsed a
	     partial expression, so we don't want to try to reverse
	     the other operands.  */
	  return r;
	}
      outbeg += oplen;
    }

  return result;
}

/* Read an expression from the string *STRINGPTR points to,
   parse it, and return a pointer to a struct expression that we malloc.
   Use block BLOCK as the lexical context for variable names;
   if BLOCK is zero, use the block of the selected stack frame.
   Meanwhile, advance *STRINGPTR to point after the expression,
   at the first nonwhite character that is not part of the expression
   (possibly a null character).

   If COMMA is nonzero, stop if a comma is reached.  */

expression_up
parse_exp_1 (const char **stringptr, CORE_ADDR pc, const struct block *block,
	     int comma, innermost_block_tracker *tracker)
{
  return parse_exp_in_context (stringptr, pc, block, comma, 0, NULL,
			       tracker, nullptr);
}

/* As for parse_exp_1, except that if VOID_CONTEXT_P, then
   no value is expected from the expression.
   OUT_SUBEXP is set when attempting to complete a field name; in this
   case it is set to the index of the subexpression on the
   left-hand-side of the struct op.  If not doing such completion, it
   is left untouched.  */

static expression_up
parse_exp_in_context (const char **stringptr, CORE_ADDR pc,
		      const struct block *block,
		      int comma, int void_context_p, int *out_subexp,
		      innermost_block_tracker *tracker,
		      expr_completion_state *cstate)
{
  const struct language_defn *lang = NULL;
  int subexp;

  if (*stringptr == 0 || **stringptr == 0)
    error_no_arg (_("expression to compute"));

  const struct block *expression_context_block = block;
  CORE_ADDR expression_context_pc = 0;

  innermost_block_tracker local_tracker;
  if (tracker == nullptr)
    tracker = &local_tracker;

  /* If no context specified, try using the current frame, if any.  */
  if (!expression_context_block)
    expression_context_block = get_selected_block (&expression_context_pc);
  else if (pc == 0)
    expression_context_pc = BLOCK_ENTRY_PC (expression_context_block);
  else
    expression_context_pc = pc;

  /* Fall back to using the current source static context, if any.  */

  if (!expression_context_block)
    {
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();
      if (cursal.symtab)
	expression_context_block
	  = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (cursal.symtab),
			       STATIC_BLOCK);
      if (expression_context_block)
	expression_context_pc = BLOCK_ENTRY_PC (expression_context_block);
    }

  if (language_mode == language_mode_auto && block != NULL)
    {
      /* Find the language associated to the given context block.
         Default to the current language if it can not be determined.

         Note that using the language corresponding to the current frame
         can sometimes give unexpected results.  For instance, this
         routine is often called several times during the inferior
         startup phase to re-parse breakpoint expressions after
         a new shared library has been loaded.  The language associated
         to the current frame at this moment is not relevant for
         the breakpoint.  Using it would therefore be silly, so it seems
         better to rely on the current language rather than relying on
         the current frame language to parse the expression.  That's why
         we do the following language detection only if the context block
         has been specifically provided.  */
      struct symbol *func = block_linkage_function (block);

      if (func != NULL)
        lang = language_def (func->language ());
      if (lang == NULL || lang->la_language == language_unknown)
        lang = current_language;
    }
  else
    lang = current_language;

  /* get_current_arch may reset CURRENT_LANGUAGE via select_frame.
     While we need CURRENT_LANGUAGE to be set to LANG (for lookup_symbol
     and others called from *.y) ensure CURRENT_LANGUAGE gets restored
     to the value matching SELECTED_FRAME as set by get_current_arch.  */

  parser_state ps (lang, get_current_arch (), expression_context_block,
		   expression_context_pc, comma, *stringptr,
		   cstate != nullptr, tracker);

  scoped_restore_current_language lang_saver;
  set_language (lang->la_language);

  try
    {
      lang->parser (&ps);
    }
  catch (const gdb_exception &except)
    {
      /* If parsing for completion, allow this to succeed; but if no
	 expression elements have been written, then there's nothing
	 to do, so fail.  */
      if (! ps.parse_completion || ps.expout_ptr == 0)
	throw;
    }

  /* We have to operate on an "expression *", due to la_post_parser,
     which explains this funny-looking double release.  */
  expression_up result = ps.release ();

  /* Convert expression from postfix form as generated by yacc
     parser, to a prefix form.  */

  if (expressiondebug)
    dump_raw_expression (result.get (), gdb_stdlog,
			 "before conversion to prefix form");

  subexp = prefixify_expression (result.get (),
				 ps.m_completion_state.expout_last_struct);
  if (out_subexp)
    *out_subexp = subexp;

  lang->post_parser (&result, void_context_p, ps.parse_completion, tracker);

  if (expressiondebug)
    dump_prefix_expression (result.get (), gdb_stdlog);

  if (cstate != nullptr)
    *cstate = std::move (ps.m_completion_state);
  *stringptr = ps.lexptr;
  return result;
}

/* Parse STRING as an expression, and complain if this fails
   to use up all of the contents of STRING.  */

expression_up
parse_expression (const char *string, innermost_block_tracker *tracker)
{
  expression_up exp = parse_exp_1 (&string, 0, 0, 0, tracker);
  if (*string)
    error (_("Junk after end of expression."));
  return exp;
}

/* Same as parse_expression, but using the given language (LANG)
   to parse the expression.  */

expression_up
parse_expression_with_language (const char *string, enum language lang)
{
  gdb::optional<scoped_restore_current_language> lang_saver;
  if (current_language->la_language != lang)
    {
      lang_saver.emplace ();
      set_language (lang);
    }

  return parse_expression (string);
}

/* Parse STRING as an expression.  If parsing ends in the middle of a
   field reference, return the type of the left-hand-side of the
   reference; furthermore, if the parsing ends in the field name,
   return the field name in *NAME.  If the parsing ends in the middle
   of a field reference, but the reference is somehow invalid, throw
   an exception.  In all other cases, return NULL.  */

struct type *
parse_expression_for_completion (const char *string,
				 gdb::unique_xmalloc_ptr<char> *name,
				 enum type_code *code)
{
  expression_up exp;
  struct value *val;
  int subexp;
  expr_completion_state cstate;

  try
    {
      exp = parse_exp_in_context (&string, 0, 0, 0, 0, &subexp,
				  nullptr, &cstate);
    }
  catch (const gdb_exception_error &except)
    {
      /* Nothing, EXP remains NULL.  */
    }

  if (exp == NULL)
    return NULL;

  if (cstate.expout_tag_completion_type != TYPE_CODE_UNDEF)
    {
      *code = cstate.expout_tag_completion_type;
      *name = std::move (cstate.expout_completion_name);
      return NULL;
    }

  if (cstate.expout_last_struct == -1)
    return NULL;

  const char *fieldname = extract_field_op (exp.get (), &subexp);
  if (fieldname == NULL)
    {
      name->reset ();
      return NULL;
    }

  name->reset (xstrdup (fieldname));
  /* This might throw an exception.  If so, we want to let it
     propagate.  */
  val = evaluate_subexpression_type (exp.get (), subexp);

  return value_type (val);
}

/* Parse floating point value P of length LEN.
   Return false if invalid, true if valid.
   The successfully parsed number is stored in DATA in
   target format for floating-point type TYPE.

   NOTE: This accepts the floating point syntax that sscanf accepts.  */

bool
parse_float (const char *p, int len,
	     const struct type *type, gdb_byte *data)
{
  return target_float_from_string (data, type, std::string (p, len));
}

/* This function avoids direct calls to fprintf 
   in the parser generated debug code.  */
void
parser_fprintf (FILE *x, const char *y, ...)
{ 
  va_list args;

  va_start (args, y);
  if (x == stderr)
    vfprintf_unfiltered (gdb_stderr, y, args); 
  else
    {
      fprintf_unfiltered (gdb_stderr, " Unknown FILE used.\n");
      vfprintf_unfiltered (gdb_stderr, y, args);
    }
  va_end (args);
}

/* Implementation of the exp_descriptor method operator_check.  */

int
operator_check_standard (struct expression *exp, int pos,
			 int (*objfile_func) (struct objfile *objfile,
					      void *data),
			 void *data)
{
  const union exp_element *const elts = exp->elts;
  struct type *type = NULL;
  struct objfile *objfile = NULL;

  /* Extended operators should have been already handled by exp_descriptor
     iterate method of its specific language.  */
  gdb_assert (elts[pos].opcode < OP_EXTENDED0);

  /* Track the callers of write_exp_elt_type for this table.  */

  switch (elts[pos].opcode)
    {
    case BINOP_VAL:
    case OP_COMPLEX:
    case OP_FLOAT:
    case OP_LONG:
    case OP_SCOPE:
    case OP_TYPE:
    case UNOP_CAST:
    case UNOP_MAX:
    case UNOP_MEMVAL:
    case UNOP_MIN:
      type = elts[pos + 1].type;
      break;

    case TYPE_INSTANCE:
      {
	LONGEST arg, nargs = elts[pos + 2].longconst;

	for (arg = 0; arg < nargs; arg++)
	  {
	    struct type *inst_type = elts[pos + 3 + arg].type;
	    struct objfile *inst_objfile = TYPE_OBJFILE (inst_type);

	    if (inst_objfile && (*objfile_func) (inst_objfile, data))
	      return 1;
	  }
      }
      break;

    case OP_VAR_VALUE:
      {
	const struct block *const block = elts[pos + 1].block;
	const struct symbol *const symbol = elts[pos + 2].symbol;

	/* Check objfile where the variable itself is placed.
	   SYMBOL_OBJ_SECTION (symbol) may be NULL.  */
	if ((*objfile_func) (symbol_objfile (symbol), data))
	  return 1;

	/* Check objfile where is placed the code touching the variable.  */
	objfile = block_objfile (block);

	type = SYMBOL_TYPE (symbol);
      }
      break;
    case OP_VAR_MSYM_VALUE:
      objfile = elts[pos + 1].objfile;
      break;
    }

  /* Invoke callbacks for TYPE and OBJFILE if they were set as non-NULL.  */

  if (type && TYPE_OBJFILE (type)
      && (*objfile_func) (TYPE_OBJFILE (type), data))
    return 1;
  if (objfile && (*objfile_func) (objfile, data))
    return 1;

  return 0;
}

/* Call OBJFILE_FUNC for any objfile found being referenced by EXP.
   OBJFILE_FUNC is never called with NULL OBJFILE.  OBJFILE_FUNC get
   passed an arbitrary caller supplied DATA pointer.  If OBJFILE_FUNC
   returns non-zero value then (any other) non-zero value is immediately
   returned to the caller.  Otherwise zero is returned after iterating
   through whole EXP.  */

static int
exp_iterate (struct expression *exp,
	     int (*objfile_func) (struct objfile *objfile, void *data),
	     void *data)
{
  int endpos;

  for (endpos = exp->nelts; endpos > 0; )
    {
      int pos, args, oplen = 0;

      operator_length (exp, endpos, &oplen, &args);
      gdb_assert (oplen > 0);

      pos = endpos - oplen;
      if (exp->language_defn->la_exp_desc->operator_check (exp, pos,
							   objfile_func, data))
	return 1;

      endpos = pos;
    }

  return 0;
}

/* Helper for exp_uses_objfile.  */

static int
exp_uses_objfile_iter (struct objfile *exp_objfile, void *objfile_voidp)
{
  struct objfile *objfile = (struct objfile *) objfile_voidp;

  if (exp_objfile->separate_debug_objfile_backlink)
    exp_objfile = exp_objfile->separate_debug_objfile_backlink;

  return exp_objfile == objfile;
}

/* Return 1 if EXP uses OBJFILE (and will become dangling when OBJFILE
   is unloaded), otherwise return 0.  OBJFILE must not be a separate debug info
   file.  */

int
exp_uses_objfile (struct expression *exp, struct objfile *objfile)
{
  gdb_assert (objfile->separate_debug_objfile_backlink == NULL);

  return exp_iterate (exp, exp_uses_objfile_iter, objfile);
}

/* Reallocate the `expout' pointer inside PS so that it can accommodate
   at least LENELT expression elements.  This function does nothing if
   there is enough room for the elements.  */

static void
increase_expout_size (struct expr_builder *ps, size_t lenelt)
{
  if ((ps->expout_ptr + lenelt) >= ps->expout_size)
    {
      ps->expout_size = std::max (ps->expout_size * 2,
				  ps->expout_ptr + lenelt + 10);
      ps->expout.reset (XRESIZEVAR (expression,
				    ps->expout.release (),
				    (sizeof (struct expression)
				     + EXP_ELEM_TO_BYTES (ps->expout_size))));
    }
}

void _initialize_parse ();
void
_initialize_parse ()
{
  add_setshow_zuinteger_cmd ("expression", class_maintenance,
			     &expressiondebug,
			     _("Set expression debugging."),
			     _("Show expression debugging."),
			     _("When non-zero, the internal representation "
			       "of expressions will be printed."),
			     NULL,
			     show_expressiondebug,
			     &setdebuglist, &showdebuglist);
  add_setshow_boolean_cmd ("parser", class_maintenance,
			    &parser_debug,
			   _("Set parser debugging."),
			   _("Show parser debugging."),
			   _("When non-zero, expression parser "
			     "tracing will be enabled."),
			    NULL,
			    show_parserdebug,
			    &setdebuglist, &showdebuglist);
}
