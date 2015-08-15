/* Parse expressions for GDB.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include <string.h>
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
#include "doublest.h"
#include "gdb_assert.h"
#include "block.h"
#include "source.h"
#include "objfiles.h"
#include "exceptions.h"
#include "user-regs.h"

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

/* Global variables declared in parser-defs.h (and commented there).  */
struct expression *expout;
int expout_size;
int expout_ptr;
const struct block *expression_context_block;
CORE_ADDR expression_context_pc;
const struct block *innermost_block;
int arglist_len;
static struct type_stack type_stack;
const char *lexptr;
const char *prev_lexptr;
int paren_depth;
int comma_terminates;

/* True if parsing an expression to attempt completion.  */
int parse_completion;

/* The index of the last struct expression directly before a '.' or
   '->'.  This is set when parsing and is only used when completing a
   field name.  It is -1 if no dereference operation was found.  */
static int expout_last_struct = -1;

/* If we are completing a tagged type name, this will be nonzero.  */
static enum type_code expout_tag_completion_type = TYPE_CODE_UNDEF;

/* The token for tagged type name completion.  */
static char *expout_completion_name;


static unsigned int expressiondebug = 0;
static void
show_expressiondebug (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Expression debugging is %s.\n"), value);
}


/* Non-zero if an expression parser should set yydebug.  */
int parser_debug;

static void
show_parserdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Parser debugging is %s.\n"), value);
}


static void free_funcalls (void *ignore);

static int prefixify_subexp (struct expression *, struct expression *, int,
			     int);

static struct expression *parse_exp_in_context (const char **, CORE_ADDR,
						const struct block *, int, 
						int, int *);
static struct expression *parse_exp_in_context_1 (const char **, CORE_ADDR,
						  const struct block *, int,
						  int, int *);

void _initialize_parse (void);

/* Data structure for saving values of arglist_len for function calls whose
   arguments contain other function calls.  */

struct funcall
  {
    struct funcall *next;
    int arglist_len;
  };

static struct funcall *funcall_chain;

/* Begin counting arguments for a function call,
   saving the data about any containing call.  */

void
start_arglist (void)
{
  struct funcall *new;

  new = (struct funcall *) xmalloc (sizeof (struct funcall));
  new->next = funcall_chain;
  new->arglist_len = arglist_len;
  arglist_len = 0;
  funcall_chain = new;
}

/* Return the number of arguments in a function call just terminated,
   and restore the data for the containing function call.  */

int
end_arglist (void)
{
  int val = arglist_len;
  struct funcall *call = funcall_chain;

  funcall_chain = call->next;
  arglist_len = call->arglist_len;
  xfree (call);
  return val;
}

/* Free everything in the funcall chain.
   Used when there is an error inside parsing.  */

static void
free_funcalls (void *ignore)
{
  struct funcall *call, *next;

  for (call = funcall_chain; call; call = next)
    {
      next = call->next;
      xfree (call);
    }
}

/* This page contains the functions for adding data to the struct expression
   being constructed.  */

/* See definition in parser-defs.h.  */

void
initialize_expout (int initial_size, const struct language_defn *lang,
		   struct gdbarch *gdbarch)
{
  expout_size = initial_size;
  expout_ptr = 0;
  expout = xmalloc (sizeof (struct expression)
		    + EXP_ELEM_TO_BYTES (expout_size));
  expout->language_defn = lang;
  expout->gdbarch = gdbarch;
}

/* See definition in parser-defs.h.  */

void
reallocate_expout (void)
{
  /* Record the actual number of expression elements, and then
     reallocate the expression memory so that we free up any
     excess elements.  */

  expout->nelts = expout_ptr;
  expout = xrealloc ((char *) expout,
		     sizeof (struct expression)
		     + EXP_ELEM_TO_BYTES (expout_ptr));
}

/* Add one element to the end of the expression.  */

/* To avoid a bug in the Sun 4 compiler, we pass things that can fit into
   a register through here.  */

static void
write_exp_elt (const union exp_element *expelt)
{
  if (expout_ptr >= expout_size)
    {
      expout_size *= 2;
      expout = (struct expression *)
	xrealloc ((char *) expout, sizeof (struct expression)
		  + EXP_ELEM_TO_BYTES (expout_size));
    }
  expout->elts[expout_ptr++] = *expelt;
}

void
write_exp_elt_opcode (enum exp_opcode expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.opcode = expelt;
  write_exp_elt (&tmp);
}

void
write_exp_elt_sym (struct symbol *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.symbol = expelt;
  write_exp_elt (&tmp);
}

void
write_exp_elt_block (const struct block *b)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.block = b;
  write_exp_elt (&tmp);
}

void
write_exp_elt_objfile (struct objfile *objfile)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.objfile = objfile;
  write_exp_elt (&tmp);
}

void
write_exp_elt_longcst (LONGEST expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.longconst = expelt;
  write_exp_elt (&tmp);
}

void
write_exp_elt_dblcst (DOUBLEST expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.doubleconst = expelt;
  write_exp_elt (&tmp);
}

void
write_exp_elt_decfloatcst (gdb_byte expelt[16])
{
  union exp_element tmp;
  int index;

  for (index = 0; index < 16; index++)
    tmp.decfloatconst[index] = expelt[index];

  write_exp_elt (&tmp);
}

void
write_exp_elt_type (struct type *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.type = expelt;
  write_exp_elt (&tmp);
}

void
write_exp_elt_intern (struct internalvar *expelt)
{
  union exp_element tmp;

  memset (&tmp, 0, sizeof (union exp_element));
  tmp.internalvar = expelt;
  write_exp_elt (&tmp);
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
write_exp_string (struct stoken str)
{
  int len = str.length;
  int lenelt;
  char *strdata;

  /* Compute the number of expression elements required to hold the string
     (including a null byte terminator), along with one expression element
     at each end to record the actual string length (not including the
     null byte terminator).  */

  lenelt = 2 + BYTES_TO_EXP_ELEM (len + 1);

  /* Ensure that we have enough available expression elements to store
     everything.  */

  if ((expout_ptr + lenelt) >= expout_size)
    {
      expout_size = max (expout_size * 2, expout_ptr + lenelt + 10);
      expout = (struct expression *)
	xrealloc ((char *) expout, (sizeof (struct expression)
				    + EXP_ELEM_TO_BYTES (expout_size)));
    }

  /* Write the leading length expression element (which advances the current
     expression element index), then write the string constant followed by a
     terminating null byte, and then write the trailing length expression
     element.  */

  write_exp_elt_longcst ((LONGEST) len);
  strdata = (char *) &expout->elts[expout_ptr];
  memcpy (strdata, str.ptr, len);
  *(strdata + len) = '\0';
  expout_ptr += lenelt - 2;
  write_exp_elt_longcst ((LONGEST) len);
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
write_exp_string_vector (int type, struct stoken_vector *vec)
{
  int i, n_slots, len;

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
  if ((expout_ptr + n_slots) >= expout_size)
    {
      expout_size = max (expout_size * 2, expout_ptr + n_slots + 10);
      expout = (struct expression *)
	xrealloc ((char *) expout, (sizeof (struct expression)
				    + EXP_ELEM_TO_BYTES (expout_size)));
    }

  write_exp_elt_opcode (OP_STRING);
  write_exp_elt_longcst (len);
  write_exp_elt_longcst (type);

  for (i = 0; i < vec->len; ++i)
    {
      write_exp_elt_longcst (vec->tokens[i].length);
      memcpy (&expout->elts[expout_ptr], vec->tokens[i].ptr,
	      vec->tokens[i].length);
      expout_ptr += BYTES_TO_EXP_ELEM (vec->tokens[i].length);
    }

  write_exp_elt_longcst (len);
  write_exp_elt_opcode (OP_STRING);
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
write_exp_bitstring (struct stoken str)
{
  int bits = str.length;	/* length in bits */
  int len = (bits + HOST_CHAR_BIT - 1) / HOST_CHAR_BIT;
  int lenelt;
  char *strdata;

  /* Compute the number of expression elements required to hold the bitstring,
     along with one expression element at each end to record the actual
     bitstring length in bits.  */

  lenelt = 2 + BYTES_TO_EXP_ELEM (len);

  /* Ensure that we have enough available expression elements to store
     everything.  */

  if ((expout_ptr + lenelt) >= expout_size)
    {
      expout_size = max (expout_size * 2, expout_ptr + lenelt + 10);
      expout = (struct expression *)
	xrealloc ((char *) expout, (sizeof (struct expression)
				    + EXP_ELEM_TO_BYTES (expout_size)));
    }

  /* Write the leading length expression element (which advances the current
     expression element index), then write the bitstring constant, and then
     write the trailing length expression element.  */

  write_exp_elt_longcst ((LONGEST) bits);
  strdata = (char *) &expout->elts[expout_ptr];
  memcpy (strdata, str.ptr, len);
  expout_ptr += lenelt - 2;
  write_exp_elt_longcst ((LONGEST) bits);
}

/* Add the appropriate elements for a minimal symbol to the end of
   the expression.  */

void
write_exp_msymbol (struct bound_minimal_symbol bound_msym)
{
  struct minimal_symbol *msymbol = bound_msym.minsym;
  struct objfile *objfile = bound_msym.objfile;
  struct gdbarch *gdbarch = get_objfile_arch (objfile);

  CORE_ADDR addr = SYMBOL_VALUE_ADDRESS (msymbol);
  struct obj_section *section = SYMBOL_OBJ_SECTION (objfile, msymbol);
  enum minimal_symbol_type type = MSYMBOL_TYPE (msymbol);
  CORE_ADDR pc;

  /* The minimal symbol might point to a function descriptor;
     resolve it to the actual code address instead.  */
  pc = gdbarch_convert_from_func_ptr_addr (gdbarch, addr, &current_target);
  if (pc != addr)
    {
      struct bound_minimal_symbol ifunc_msym = lookup_minimal_symbol_by_pc (pc);

      /* In this case, assume we have a code symbol instead of
	 a data symbol.  */

      if (ifunc_msym.minsym != NULL
	  && MSYMBOL_TYPE (ifunc_msym.minsym) == mst_text_gnu_ifunc
	  && SYMBOL_VALUE_ADDRESS (ifunc_msym.minsym) == pc)
	{
	  /* A function descriptor has been resolved but PC is still in the
	     STT_GNU_IFUNC resolver body (such as because inferior does not
	     run to be able to call it).  */

	  type = mst_text_gnu_ifunc;
	}
      else
	type = mst_text;
      section = NULL;
      addr = pc;
    }

  if (overlay_debugging)
    addr = symbol_overlayed_address (addr, section);

  write_exp_elt_opcode (OP_LONG);
  /* Let's make the type big enough to hold a 64-bit address.  */
  write_exp_elt_type (objfile_type (objfile)->builtin_core_addr);
  write_exp_elt_longcst ((LONGEST) addr);
  write_exp_elt_opcode (OP_LONG);

  if (section && section->the_bfd_section->flags & SEC_THREAD_LOCAL)
    {
      write_exp_elt_opcode (UNOP_MEMVAL_TLS);
      write_exp_elt_objfile (objfile);
      write_exp_elt_type (objfile_type (objfile)->nodebug_tls_symbol);
      write_exp_elt_opcode (UNOP_MEMVAL_TLS);
      return;
    }

  write_exp_elt_opcode (UNOP_MEMVAL);
  switch (type)
    {
    case mst_text:
    case mst_file_text:
    case mst_solib_trampoline:
      write_exp_elt_type (objfile_type (objfile)->nodebug_text_symbol);
      break;

    case mst_text_gnu_ifunc:
      write_exp_elt_type (objfile_type (objfile)
					       ->nodebug_text_gnu_ifunc_symbol);
      break;

    case mst_data:
    case mst_file_data:
    case mst_bss:
    case mst_file_bss:
      write_exp_elt_type (objfile_type (objfile)->nodebug_data_symbol);
      break;

    case mst_slot_got_plt:
      write_exp_elt_type (objfile_type (objfile)->nodebug_got_plt_symbol);
      break;

    default:
      write_exp_elt_type (objfile_type (objfile)->nodebug_unknown_symbol);
      break;
    }
  write_exp_elt_opcode (UNOP_MEMVAL);
}

/* Mark the current index as the starting location of a structure
   expression.  This is used when completing on field names.  */

void
mark_struct_expression (void)
{
  gdb_assert (parse_completion
	      && expout_tag_completion_type == TYPE_CODE_UNDEF);
  expout_last_struct = expout_ptr;
}

/* Indicate that the current parser invocation is completing a tag.
   TAG is the type code of the tag, and PTR and LENGTH represent the
   start of the tag name.  */

void
mark_completion_tag (enum type_code tag, const char *ptr, int length)
{
  gdb_assert (parse_completion
	      && expout_tag_completion_type == TYPE_CODE_UNDEF
	      && expout_completion_name == NULL
	      && expout_last_struct == -1);
  gdb_assert (tag == TYPE_CODE_UNION
	      || tag == TYPE_CODE_STRUCT
	      || tag == TYPE_CODE_CLASS
	      || tag == TYPE_CODE_ENUM);
  expout_tag_completion_type = tag;
  expout_completion_name = xmalloc (length + 1);
  memcpy (expout_completion_name, ptr, length);
  expout_completion_name[length] = '\0';
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
write_dollar_variable (struct stoken str)
{
  struct symbol *sym = NULL;
  struct bound_minimal_symbol msym;
  struct internalvar *isym = NULL;

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
  i = user_reg_map_name_to_regnum (parse_gdbarch,
				   str.ptr + 1, str.length - 1);
  if (i >= 0)
    goto handle_register;

  /* Any names starting with $ are probably debugger internal variables.  */

  isym = lookup_only_internalvar (copy_name (str) + 1);
  if (isym)
    {
      write_exp_elt_opcode (OP_INTERNALVAR);
      write_exp_elt_intern (isym);
      write_exp_elt_opcode (OP_INTERNALVAR);
      return;
    }

  /* On some systems, such as HP-UX and hppa-linux, certain system routines 
     have names beginning with $ or $$.  Check for those, first.  */

  sym = lookup_symbol (copy_name (str), (struct block *) NULL,
		       VAR_DOMAIN, NULL);
  if (sym)
    {
      write_exp_elt_opcode (OP_VAR_VALUE);
      write_exp_elt_block (block_found);	/* set by lookup_symbol */
      write_exp_elt_sym (sym);
      write_exp_elt_opcode (OP_VAR_VALUE);
      return;
    }
  msym = lookup_bound_minimal_symbol (copy_name (str));
  if (msym.minsym)
    {
      write_exp_msymbol (msym);
      return;
    }

  /* Any other names are assumed to be debugger internal variables.  */

  write_exp_elt_opcode (OP_INTERNALVAR);
  write_exp_elt_intern (create_internalvar (copy_name (str) + 1));
  write_exp_elt_opcode (OP_INTERNALVAR);
  return;
handle_last:
  write_exp_elt_opcode (OP_LAST);
  write_exp_elt_longcst ((LONGEST) i);
  write_exp_elt_opcode (OP_LAST);
  return;
handle_register:
  write_exp_elt_opcode (OP_REGISTER);
  str.length--;
  str.ptr++;
  write_exp_string (str);
  write_exp_elt_opcode (OP_REGISTER);
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
   into a separate block of storage.

   N.B. A single buffer is reused on each call.  */

char *
copy_name (struct stoken token)
{
  /* A temporary buffer for identifiers, so we can null-terminate them.
     We allocate this with xrealloc.  parse_exp_1 used to allocate with
     alloca, using the size of the whole expression as a conservative
     estimate of the space needed.  However, macro expansion can
     introduce names longer than the original expression; there's no
     practical way to know beforehand how large that might be.  */
  static char *namecopy;
  static size_t namecopy_size;

  /* Make sure there's enough space for the token.  */
  if (namecopy_size < token.length + 1)
    {
      namecopy_size = token.length + 1;
      namecopy = xrealloc (namecopy, token.length + 1);
    }
      
  memcpy (namecopy, token.ptr, token.length);
  namecopy[token.length] = 0;

  return namecopy;
}


/* See comments on parser-defs.h.  */

int
prefixify_expression (struct expression *expr)
{
  int len = sizeof (struct expression) + EXP_ELEM_TO_BYTES (expr->nelts);
  struct expression *temp;
  int inpos = expr->nelts, outpos = 0;

  temp = (struct expression *) alloca (len);

  /* Copy the original expression into temp.  */
  memcpy (temp, expr, len);

  return prefixify_subexp (temp, expr, inpos, outpos);
}

/* Return the number of exp_elements in the postfix subexpression 
   of EXPR whose operator is at index ENDPOS - 1 in EXPR.  */

int
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
  enum f90_range_type range_type;
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
    case OP_DOUBLE:
    case OP_DECFLOAT:
    case OP_VAR_VALUE:
      oplen = 4;
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
      oplen = 4 + longest_to_int (expr->elts[endpos - 2].longconst);
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

    case UNOP_MEMVAL_TLS:
      oplen = 4;
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

    case OP_F90_RANGE:
      oplen = 3;

      range_type = longest_to_int (expr->elts[endpos - 2].longconst);
      switch (range_type)
	{
	case LOW_BOUND_DEFAULT:
	case HIGH_BOUND_DEFAULT:
	  args = 1;
	  break;
	case BOTH_BOUND_DEFAULT:
	  args = 0;
	  break;
	case NONE_BOUND_DEFAULT:
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
   If EXPOUT_LAST_STRUCT is -1, then this function always returns -1.
   Otherwise, it returns the index of the subexpression which is the
   left-hand-side of the expression at EXPOUT_LAST_STRUCT.  */

static int
prefixify_subexp (struct expression *inexpr,
		  struct expression *outexpr, int inend, int outbeg)
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

  if (expout_last_struct == inend)
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
      r = prefixify_subexp (inexpr, outexpr, inend, outbeg);
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

struct expression *
parse_exp_1 (const char **stringptr, CORE_ADDR pc, const struct block *block,
	     int comma)
{
  return parse_exp_in_context (stringptr, pc, block, comma, 0, NULL);
}

static struct expression *
parse_exp_in_context (const char **stringptr, CORE_ADDR pc,
		      const struct block *block,
		      int comma, int void_context_p, int *out_subexp)
{
  return parse_exp_in_context_1 (stringptr, pc, block, comma,
				 void_context_p, out_subexp);
}

/* As for parse_exp_1, except that if VOID_CONTEXT_P, then
   no value is expected from the expression.
   OUT_SUBEXP is set when attempting to complete a field name; in this
   case it is set to the index of the subexpression on the
   left-hand-side of the struct op.  If not doing such completion, it
   is left untouched.  */

static struct expression *
parse_exp_in_context_1 (const char **stringptr, CORE_ADDR pc,
			const struct block *block,
			int comma, int void_context_p, int *out_subexp)
{
  volatile struct gdb_exception except;
  struct cleanup *old_chain, *inner_chain;
  const struct language_defn *lang = NULL;
  int subexp;

  lexptr = *stringptr;
  prev_lexptr = NULL;

  paren_depth = 0;
  type_stack.depth = 0;
  expout_last_struct = -1;
  expout_tag_completion_type = TYPE_CODE_UNDEF;
  xfree (expout_completion_name);
  expout_completion_name = NULL;

  comma_terminates = comma;

  if (lexptr == 0 || *lexptr == 0)
    error_no_arg (_("expression to compute"));

  old_chain = make_cleanup (free_funcalls, 0 /*ignore*/);
  funcall_chain = 0;

  expression_context_block = block;

  /* If no context specified, try using the current frame, if any.  */
  if (!expression_context_block)
    expression_context_block = get_selected_block (&expression_context_pc);
  else if (pc == 0)
    expression_context_pc = BLOCK_START (expression_context_block);
  else
    expression_context_pc = pc;

  /* Fall back to using the current source static context, if any.  */

  if (!expression_context_block)
    {
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();
      if (cursal.symtab)
	expression_context_block
	  = BLOCKVECTOR_BLOCK (BLOCKVECTOR (cursal.symtab), STATIC_BLOCK);
      if (expression_context_block)
	expression_context_pc = BLOCK_START (expression_context_block);
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
        lang = language_def (SYMBOL_LANGUAGE (func));
      if (lang == NULL || lang->la_language == language_unknown)
        lang = current_language;
    }
  else
    lang = current_language;

  /* get_current_arch may reset CURRENT_LANGUAGE via select_frame.
     While we need CURRENT_LANGUAGE to be set to LANG (for lookup_symbol
     and others called from *.y) ensure CURRENT_LANGUAGE gets restored
     to the value matching SELECTED_FRAME as set by get_current_arch.  */
  initialize_expout (10, lang, get_current_arch ());
  inner_chain = make_cleanup_restore_current_language ();
  set_language (lang->la_language);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      if (lang->la_parser ())
        lang->la_error (NULL);
    }
  if (except.reason < 0)
    {
      if (! parse_completion)
	{
	  xfree (expout);
	  throw_exception (except);
	}
    }

  reallocate_expout ();

  /* Convert expression from postfix form as generated by yacc
     parser, to a prefix form.  */

  if (expressiondebug)
    dump_raw_expression (expout, gdb_stdlog,
			 "before conversion to prefix form");

  subexp = prefixify_expression (expout);
  if (out_subexp)
    *out_subexp = subexp;

  lang->la_post_parser (&expout, void_context_p);

  if (expressiondebug)
    dump_prefix_expression (expout, gdb_stdlog);

  do_cleanups (inner_chain);
  discard_cleanups (old_chain);

  *stringptr = lexptr;
  return expout;
}

/* Parse STRING as an expression, and complain if this fails
   to use up all of the contents of STRING.  */

struct expression *
parse_expression (const char *string)
{
  struct expression *exp;

  exp = parse_exp_1 (&string, 0, 0, 0);
  if (*string)
    error (_("Junk after end of expression."));
  return exp;
}

/* Parse STRING as an expression.  If parsing ends in the middle of a
   field reference, return the type of the left-hand-side of the
   reference; furthermore, if the parsing ends in the field name,
   return the field name in *NAME.  If the parsing ends in the middle
   of a field reference, but the reference is somehow invalid, throw
   an exception.  In all other cases, return NULL.  Returned non-NULL
   *NAME must be freed by the caller.  */

struct type *
parse_expression_for_completion (const char *string, char **name,
				 enum type_code *code)
{
  struct expression *exp = NULL;
  struct value *val;
  int subexp;
  volatile struct gdb_exception except;

  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      parse_completion = 1;
      exp = parse_exp_in_context (&string, 0, 0, 0, 0, &subexp);
    }
  parse_completion = 0;
  if (except.reason < 0 || ! exp)
    return NULL;

  if (expout_tag_completion_type != TYPE_CODE_UNDEF)
    {
      *code = expout_tag_completion_type;
      *name = expout_completion_name;
      expout_completion_name = NULL;
      return NULL;
    }

  if (expout_last_struct == -1)
    {
      xfree (exp);
      return NULL;
    }

  *name = extract_field_op (exp, &subexp);
  if (!*name)
    {
      xfree (exp);
      return NULL;
    }

  /* This might throw an exception.  If so, we want to let it
     propagate.  */
  val = evaluate_subexpression_type (exp, subexp);
  /* (*NAME) is a part of the EXP memory block freed below.  */
  *name = xstrdup (*name);
  xfree (exp);

  return value_type (val);
}

/* A post-parser that does nothing.  */

void
null_post_parser (struct expression **exp, int void_context_p)
{
}

/* Parse floating point value P of length LEN.
   Return 0 (false) if invalid, 1 (true) if valid.
   The successfully parsed number is stored in D.
   *SUFFIX points to the suffix of the number in P.

   NOTE: This accepts the floating point syntax that sscanf accepts.  */

int
parse_float (const char *p, int len, DOUBLEST *d, const char **suffix)
{
  char *copy;
  int n, num;

  copy = xmalloc (len + 1);
  memcpy (copy, p, len);
  copy[len] = 0;

  num = sscanf (copy, "%" DOUBLEST_SCAN_FORMAT "%n", d, &n);
  xfree (copy);

  /* The sscanf man page suggests not making any assumptions on the effect
     of %n on the result, so we don't.
     That is why we simply test num == 0.  */
  if (num == 0)
    return 0;

  *suffix = p + n;
  return 1;
}

/* Parse floating point value P of length LEN, using the C syntax for floats.
   Return 0 (false) if invalid, 1 (true) if valid.
   The successfully parsed number is stored in *D.
   Its type is taken from builtin_type (gdbarch) and is stored in *T.  */

int
parse_c_float (struct gdbarch *gdbarch, const char *p, int len,
	       DOUBLEST *d, struct type **t)
{
  const char *suffix;
  int suffix_len;
  const struct builtin_type *builtin_types = builtin_type (gdbarch);

  if (! parse_float (p, len, d, &suffix))
    return 0;

  suffix_len = p + len - suffix;

  if (suffix_len == 0)
    *t = builtin_types->builtin_double;
  else if (suffix_len == 1)
    {
      /* Handle suffixes: 'f' for float, 'l' for long double.  */
      if (tolower (*suffix) == 'f')
	*t = builtin_types->builtin_float;
      else if (tolower (*suffix) == 'l')
	*t = builtin_types->builtin_long_double;
      else
	return 0;
    }
  else
    return 0;

  return 1;
}

/* Stuff for maintaining a stack of types.  Currently just used by C, but
   probably useful for any language which declares its types "backwards".  */

/* Ensure that there are HOWMUCH open slots on the type stack STACK.  */

static void
type_stack_reserve (struct type_stack *stack, int howmuch)
{
  if (stack->depth + howmuch >= stack->size)
    {
      stack->size *= 2;
      if (stack->size < howmuch)
	stack->size = howmuch;
      stack->elements = xrealloc (stack->elements,
				  stack->size * sizeof (union type_stack_elt));
    }
}

/* Ensure that there is a single open slot in the global type stack.  */

static void
check_type_stack_depth (void)
{
  type_stack_reserve (&type_stack, 1);
}

/* A helper function for insert_type and insert_type_address_space.
   This does work of expanding the type stack and inserting the new
   element, ELEMENT, into the stack at location SLOT.  */

static void
insert_into_type_stack (int slot, union type_stack_elt element)
{
  check_type_stack_depth ();

  if (slot < type_stack.depth)
    memmove (&type_stack.elements[slot + 1], &type_stack.elements[slot],
	     (type_stack.depth - slot) * sizeof (union type_stack_elt));
  type_stack.elements[slot] = element;
  ++type_stack.depth;
}

/* Insert a new type, TP, at the bottom of the type stack.  If TP is
   tp_pointer or tp_reference, it is inserted at the bottom.  If TP is
   a qualifier, it is inserted at slot 1 (just above a previous
   tp_pointer) if there is anything on the stack, or simply pushed if
   the stack is empty.  Other values for TP are invalid.  */

void
insert_type (enum type_pieces tp)
{
  union type_stack_elt element;
  int slot;

  gdb_assert (tp == tp_pointer || tp == tp_reference
	      || tp == tp_const || tp == tp_volatile);

  /* If there is anything on the stack (we know it will be a
     tp_pointer), insert the qualifier above it.  Otherwise, simply
     push this on the top of the stack.  */
  if (type_stack.depth && (tp == tp_const || tp == tp_volatile))
    slot = 1;
  else
    slot = 0;

  element.piece = tp;
  insert_into_type_stack (slot, element);
}

void
push_type (enum type_pieces tp)
{
  check_type_stack_depth ();
  type_stack.elements[type_stack.depth++].piece = tp;
}

void
push_type_int (int n)
{
  check_type_stack_depth ();
  type_stack.elements[type_stack.depth++].int_val = n;
}

/* Insert a tp_space_identifier and the corresponding address space
   value into the stack.  STRING is the name of an address space, as
   recognized by address_space_name_to_int.  If the stack is empty,
   the new elements are simply pushed.  If the stack is not empty,
   this function assumes that the first item on the stack is a
   tp_pointer, and the new values are inserted above the first
   item.  */

void
insert_type_address_space (char *string)
{
  union type_stack_elt element;
  int slot;

  /* If there is anything on the stack (we know it will be a
     tp_pointer), insert the address space qualifier above it.
     Otherwise, simply push this on the top of the stack.  */
  if (type_stack.depth)
    slot = 1;
  else
    slot = 0;

  element.piece = tp_space_identifier;
  insert_into_type_stack (slot, element);
  element.int_val = address_space_name_to_int (parse_gdbarch, string);
  insert_into_type_stack (slot, element);
}

enum type_pieces
pop_type (void)
{
  if (type_stack.depth)
    return type_stack.elements[--type_stack.depth].piece;
  return tp_end;
}

int
pop_type_int (void)
{
  if (type_stack.depth)
    return type_stack.elements[--type_stack.depth].int_val;
  /* "Can't happen".  */
  return 0;
}

/* Pop a type list element from the global type stack.  */

static VEC (type_ptr) *
pop_typelist (void)
{
  gdb_assert (type_stack.depth);
  return type_stack.elements[--type_stack.depth].typelist_val;
}

/* Pop a type_stack element from the global type stack.  */

static struct type_stack *
pop_type_stack (void)
{
  gdb_assert (type_stack.depth);
  return type_stack.elements[--type_stack.depth].stack_val;
}

/* Append the elements of the type stack FROM to the type stack TO.
   Always returns TO.  */

struct type_stack *
append_type_stack (struct type_stack *to, struct type_stack *from)
{
  type_stack_reserve (to, from->depth);

  memcpy (&to->elements[to->depth], &from->elements[0],
	  from->depth * sizeof (union type_stack_elt));
  to->depth += from->depth;

  return to;
}

/* Push the type stack STACK as an element on the global type stack.  */

void
push_type_stack (struct type_stack *stack)
{
  check_type_stack_depth ();
  type_stack.elements[type_stack.depth++].stack_val = stack;
  push_type (tp_type_stack);
}

/* Copy the global type stack into a newly allocated type stack and
   return it.  The global stack is cleared.  The returned type stack
   must be freed with type_stack_cleanup.  */

struct type_stack *
get_type_stack (void)
{
  struct type_stack *result = XNEW (struct type_stack);

  *result = type_stack;
  type_stack.depth = 0;
  type_stack.size = 0;
  type_stack.elements = NULL;

  return result;
}

/* A cleanup function that destroys a single type stack.  */

void
type_stack_cleanup (void *arg)
{
  struct type_stack *stack = arg;

  xfree (stack->elements);
  xfree (stack);
}

/* Push a function type with arguments onto the global type stack.
   LIST holds the argument types.  If the final item in LIST is NULL,
   then the function will be varargs.  */

void
push_typelist (VEC (type_ptr) *list)
{
  check_type_stack_depth ();
  type_stack.elements[type_stack.depth++].typelist_val = list;
  push_type (tp_function_with_arguments);
}

/* Pop the type stack and return the type which corresponds to FOLLOW_TYPE
   as modified by all the stuff on the stack.  */
struct type *
follow_types (struct type *follow_type)
{
  int done = 0;
  int make_const = 0;
  int make_volatile = 0;
  int make_addr_space = 0;
  int array_size;

  while (!done)
    switch (pop_type ())
      {
      case tp_end:
	done = 1;
	if (make_const)
	  follow_type = make_cv_type (make_const, 
				      TYPE_VOLATILE (follow_type), 
				      follow_type, 0);
	if (make_volatile)
	  follow_type = make_cv_type (TYPE_CONST (follow_type), 
				      make_volatile, 
				      follow_type, 0);
	if (make_addr_space)
	  follow_type = make_type_with_address_space (follow_type, 
						      make_addr_space);
	make_const = make_volatile = 0;
	make_addr_space = 0;
	break;
      case tp_const:
	make_const = 1;
	break;
      case tp_volatile:
	make_volatile = 1;
	break;
      case tp_space_identifier:
	make_addr_space = pop_type_int ();
	break;
      case tp_pointer:
	follow_type = lookup_pointer_type (follow_type);
	if (make_const)
	  follow_type = make_cv_type (make_const, 
				      TYPE_VOLATILE (follow_type), 
				      follow_type, 0);
	if (make_volatile)
	  follow_type = make_cv_type (TYPE_CONST (follow_type), 
				      make_volatile, 
				      follow_type, 0);
	if (make_addr_space)
	  follow_type = make_type_with_address_space (follow_type, 
						      make_addr_space);
	make_const = make_volatile = 0;
	make_addr_space = 0;
	break;
      case tp_reference:
	follow_type = lookup_reference_type (follow_type);
	if (make_const)
	  follow_type = make_cv_type (make_const, 
				      TYPE_VOLATILE (follow_type), 
				      follow_type, 0);
	if (make_volatile)
	  follow_type = make_cv_type (TYPE_CONST (follow_type), 
				      make_volatile, 
				      follow_type, 0);
	if (make_addr_space)
	  follow_type = make_type_with_address_space (follow_type, 
						      make_addr_space);
	make_const = make_volatile = 0;
	make_addr_space = 0;
	break;
      case tp_array:
	array_size = pop_type_int ();
	/* FIXME-type-allocation: need a way to free this type when we are
	   done with it.  */
	follow_type =
	  lookup_array_range_type (follow_type,
				   0, array_size >= 0 ? array_size - 1 : 0);
	if (array_size < 0)
	  TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (follow_type) = 1;
	break;
      case tp_function:
	/* FIXME-type-allocation: need a way to free this type when we are
	   done with it.  */
	follow_type = lookup_function_type (follow_type);
	break;

      case tp_function_with_arguments:
	{
	  VEC (type_ptr) *args = pop_typelist ();

	  follow_type
	    = lookup_function_type_with_arguments (follow_type,
						   VEC_length (type_ptr, args),
						   VEC_address (type_ptr,
								args));
	  VEC_free (type_ptr, args);
	}
	break;

      case tp_type_stack:
	{
	  struct type_stack *stack = pop_type_stack ();
	  /* Sort of ugly, but not really much worse than the
	     alternatives.  */
	  struct type_stack save = type_stack;

	  type_stack = *stack;
	  follow_type = follow_types (follow_type);
	  gdb_assert (type_stack.depth == 0);

	  type_stack = save;
	}
	break;
      default:
	gdb_assert_not_reached ("unrecognized tp_ value in follow_types");
      }
  return follow_type;
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
    case OP_DECFLOAT:
    case OP_DOUBLE:
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
	LONGEST arg, nargs = elts[pos + 1].longconst;

	for (arg = 0; arg < nargs; arg++)
	  {
	    struct type *type = elts[pos + 2 + arg].type;
	    struct objfile *objfile = TYPE_OBJFILE (type);

	    if (objfile && (*objfile_func) (objfile, data))
	      return 1;
	  }
      }
      break;

    case UNOP_MEMVAL_TLS:
      objfile = elts[pos + 1].objfile;
      type = elts[pos + 2].type;
      break;

    case OP_VAR_VALUE:
      {
	const struct block *const block = elts[pos + 1].block;
	const struct symbol *const symbol = elts[pos + 2].symbol;

	/* Check objfile where the variable itself is placed.
	   SYMBOL_OBJ_SECTION (symbol) may be NULL.  */
	if ((*objfile_func) (SYMBOL_SYMTAB (symbol)->objfile, data))
	  return 1;

	/* Check objfile where is placed the code touching the variable.  */
	objfile = lookup_objfile_from_block (block);

	type = SYMBOL_TYPE (symbol);
      }
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

/* Call OBJFILE_FUNC for any TYPE and OBJFILE found being referenced by EXP.
   The functions are never called with NULL OBJFILE.  Functions get passed an
   arbitrary caller supplied DATA pointer.  If any of the functions returns
   non-zero value then (any other) non-zero value is immediately returned to
   the caller.  Otherwise zero is returned after iterating through whole EXP.
   */

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
  struct objfile *objfile = objfile_voidp;

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

void
_initialize_parse (void)
{
  type_stack.size = 0;
  type_stack.depth = 0;
  type_stack.elements = NULL;

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
