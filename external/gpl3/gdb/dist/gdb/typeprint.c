/* Language independent support for printing types for GDB, the GNU debugger.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include "gdb_obstack.h"
#include "bfd.h"		/* Binary File Description */
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "cp-abi.h"
#include "typeprint.h"
#include <string.h>
#include "exceptions.h"
#include "valprint.h"
#include <errno.h>
#include <ctype.h>
#include "cli/cli-utils.h"
#include "python/python.h"
#include "completer.h"

extern void _initialize_typeprint (void);

static void ptype_command (char *, int);

static void whatis_command (char *, int);

static void whatis_exp (char *, int);

const struct type_print_options type_print_raw_options =
{
  1,				/* raw */
  1,				/* print_methods */
  1,				/* print_typedefs */
  NULL,				/* local_typedefs */
  NULL,				/* global_table */
  NULL				/* global_printers */
};

/* The default flags for 'ptype' and 'whatis'.  */

static struct type_print_options default_ptype_flags =
{
  0,				/* raw */
  1,				/* print_methods */
  1,				/* print_typedefs */
  NULL,				/* local_typedefs */
  NULL,				/* global_table */
  NULL				/* global_printers */
};



/* A hash table holding typedef_field objects.  This is more
   complicated than an ordinary hash because it must also track the
   lifetime of some -- but not all -- of the contained objects.  */

struct typedef_hash_table
{
  /* The actual hash table.  */
  htab_t table;

  /* Storage for typedef_field objects that must be synthesized.  */
  struct obstack storage;
};

/* A hash function for a typedef_field.  */

static hashval_t
hash_typedef_field (const void *p)
{
  const struct typedef_field *tf = p;
  struct type *t = check_typedef (tf->type);

  return htab_hash_string (TYPE_SAFE_NAME (t));
}

/* An equality function for a typedef field.  */

static int
eq_typedef_field (const void *a, const void *b)
{
  const struct typedef_field *tfa = a;
  const struct typedef_field *tfb = b;

  return types_equal (tfa->type, tfb->type);
}

/* Add typedefs from T to the hash table TABLE.  */

void
recursively_update_typedef_hash (struct typedef_hash_table *table,
				 struct type *t)
{
  int i;

  if (table == NULL)
    return;

  for (i = 0; i < TYPE_TYPEDEF_FIELD_COUNT (t); ++i)
    {
      struct typedef_field *tdef = &TYPE_TYPEDEF_FIELD (t, i);
      void **slot;

      slot = htab_find_slot (table->table, tdef, INSERT);
      /* Only add a given typedef name once.  Really this shouldn't
	 happen; but it is safe enough to do the updates breadth-first
	 and thus use the most specific typedef.  */
      if (*slot == NULL)
	*slot = tdef;
    }

  /* Recurse into superclasses.  */
  for (i = 0; i < TYPE_N_BASECLASSES (t); ++i)
    recursively_update_typedef_hash (table, TYPE_BASECLASS (t, i));
}

/* Add template parameters from T to the typedef hash TABLE.  */

void
add_template_parameters (struct typedef_hash_table *table, struct type *t)
{
  int i;

  if (table == NULL)
    return;

  for (i = 0; i < TYPE_N_TEMPLATE_ARGUMENTS (t); ++i)
    {
      struct typedef_field *tf;
      void **slot;

      /* We only want type-valued template parameters in the hash.  */
      if (SYMBOL_CLASS (TYPE_TEMPLATE_ARGUMENT (t, i)) != LOC_TYPEDEF)
	continue;

      tf = XOBNEW (&table->storage, struct typedef_field);
      tf->name = SYMBOL_LINKAGE_NAME (TYPE_TEMPLATE_ARGUMENT (t, i));
      tf->type = SYMBOL_TYPE (TYPE_TEMPLATE_ARGUMENT (t, i));

      slot = htab_find_slot (table->table, tf, INSERT);
      if (*slot == NULL)
	*slot = tf;
    }
}

/* Create a new typedef-lookup hash table.  */

struct typedef_hash_table *
create_typedef_hash (void)
{
  struct typedef_hash_table *result;

  result = XNEW (struct typedef_hash_table);
  result->table = htab_create_alloc (10, hash_typedef_field, eq_typedef_field,
				     NULL, xcalloc, xfree);
  obstack_init (&result->storage);

  return result;
}

/* Free a typedef field table.  */

void
free_typedef_hash (struct typedef_hash_table *table)
{
  if (table != NULL)
    {
      htab_delete (table->table);
      obstack_free (&table->storage, NULL);
      xfree (table);
    }
}

/* A cleanup for freeing a typedef_hash_table.  */

static void
do_free_typedef_hash (void *arg)
{
  free_typedef_hash (arg);
}

/* Return a new cleanup that frees TABLE.  */

struct cleanup *
make_cleanup_free_typedef_hash (struct typedef_hash_table *table)
{
  return make_cleanup (do_free_typedef_hash, table);
}

/* Helper function for copy_typedef_hash.  */

static int
copy_typedef_hash_element (void **slot, void *nt)
{
  htab_t new_table = nt;
  void **new_slot;

  new_slot = htab_find_slot (new_table, *slot, INSERT);
  if (*new_slot == NULL)
    *new_slot = *slot;

  return 1;
}

/* Copy a typedef hash.  */

struct typedef_hash_table *
copy_typedef_hash (struct typedef_hash_table *table)
{
  struct typedef_hash_table *result;

  if (table == NULL)
    return NULL;

  result = create_typedef_hash ();
  htab_traverse_noresize (table->table, copy_typedef_hash_element,
			  result->table);
  return result;
}

/* A cleanup to free the global typedef hash.  */

static void
do_free_global_table (void *arg)
{
  struct type_print_options *flags = arg;

  free_typedef_hash (flags->global_typedefs);
  free_type_printers (flags->global_printers);
}

/* Create the global typedef hash.  */

static struct cleanup *
create_global_typedef_table (struct type_print_options *flags)
{
  gdb_assert (flags->global_typedefs == NULL && flags->global_printers == NULL);
  flags->global_typedefs = create_typedef_hash ();
  flags->global_printers = start_type_printers ();
  return make_cleanup (do_free_global_table, flags);
}

/* Look up the type T in the global typedef hash.  If it is found,
   return the typedef name.  If it is not found, apply the
   type-printers, if any, given by start_type_printers and return the
   result.  A NULL return means that the name was not found.  */

static const char *
find_global_typedef (const struct type_print_options *flags,
		     struct type *t)
{
  char *applied;
  void **slot;
  struct typedef_field tf, *new_tf;

  if (flags->global_typedefs == NULL)
    return NULL;

  tf.name = NULL;
  tf.type = t;

  slot = htab_find_slot (flags->global_typedefs->table, &tf, INSERT);
  if (*slot != NULL)
    {
      new_tf = *slot;
      return new_tf->name;
    }

  /* Put an entry into the hash table now, in case apply_type_printers
     recurses.  */
  new_tf = XOBNEW (&flags->global_typedefs->storage, struct typedef_field);
  new_tf->name = NULL;
  new_tf->type = t;

  *slot = new_tf;

  applied = apply_type_printers (flags->global_printers, t);

  if (applied != NULL)
    {
      new_tf->name = obstack_copy0 (&flags->global_typedefs->storage, applied,
				    strlen (applied));
      xfree (applied);
    }

  return new_tf->name;
}

/* Look up the type T in the typedef hash table in with FLAGS.  If T
   is in the table, return its short (class-relative) typedef name.
   Otherwise return NULL.  If the table is NULL, this always returns
   NULL.  */

const char *
find_typedef_in_hash (const struct type_print_options *flags, struct type *t)
{
  if (flags->local_typedefs != NULL)
    {
      struct typedef_field tf, *found;

      tf.name = NULL;
      tf.type = t;
      found = htab_find (flags->local_typedefs->table, &tf);

      if (found != NULL)
	return found->name;
    }

  return find_global_typedef (flags, t);
}



/* Print a description of a type in the format of a 
   typedef for the current language.
   NEW is the new name for a type TYPE.  */

void
typedef_print (struct type *type, struct symbol *new, struct ui_file *stream)
{
  LA_PRINT_TYPEDEF (type, new, stream);
}

/* The default way to print a typedef.  */

void
default_print_typedef (struct type *type, struct symbol *new_symbol,
		       struct ui_file *stream)
{
  error (_("Language not supported."));
}

/* Print a description of a type TYPE in the form of a declaration of a
   variable named VARSTRING.  (VARSTRING is demangled if necessary.)
   Output goes to STREAM (via stdio).
   If SHOW is positive, we show the contents of the outermost level
   of structure even if there is a type name that could be used instead.
   If SHOW is negative, we never show the details of elements' types.  */

void
type_print (struct type *type, const char *varstring, struct ui_file *stream,
	    int show)
{
  LA_PRINT_TYPE (type, varstring, stream, show, 0, &default_ptype_flags);
}

/* Print TYPE to a string, returning it.  The caller is responsible for
   freeing the string.  */

char *
type_to_string (struct type *type)
{
  char *s = NULL;
  struct ui_file *stb;
  struct cleanup *old_chain;
  volatile struct gdb_exception except;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  TRY_CATCH (except, RETURN_MASK_ALL)
    {
      type_print (type, "", stb, -1);
      s = ui_file_xstrdup (stb, NULL);
    }
  if (except.reason < 0)
    s = NULL;

  do_cleanups (old_chain);

  return s;
}

/* Print type of EXP, or last thing in value history if EXP == NULL.
   show is passed to type_print.  */

static void
whatis_exp (char *exp, int show)
{
  struct expression *expr;
  struct value *val;
  struct cleanup *old_chain;
  struct type *real_type = NULL;
  struct type *type;
  int full = 0;
  int top = -1;
  int using_enc = 0;
  struct value_print_options opts;
  struct type_print_options flags = default_ptype_flags;

  old_chain = make_cleanup (null_cleanup, NULL);

  if (exp)
    {
      if (*exp == '/')
	{
	  int seen_one = 0;

	  for (++exp; *exp && !isspace (*exp); ++exp)
	    {
	      switch (*exp)
		{
		case 'r':
		  flags.raw = 1;
		  break;
		case 'm':
		  flags.print_methods = 0;
		  break;
		case 'M':
		  flags.print_methods = 1;
		  break;
		case 't':
		  flags.print_typedefs = 0;
		  break;
		case 'T':
		  flags.print_typedefs = 1;
		  break;
		default:
		  error (_("unrecognized flag '%c'"), *exp);
		}
	      seen_one = 1;
	    }

	  if (!*exp && !seen_one)
	    error (_("flag expected"));
	  if (!isspace (*exp))
	    error (_("expected space after format"));
	  exp = skip_spaces (exp);
	}

      expr = parse_expression (exp);
      make_cleanup (free_current_contents, &expr);
      val = evaluate_type (expr);
    }
  else
    val = access_value_history (0);

  type = value_type (val);

  get_user_print_options (&opts);
  if (opts.objectprint)
    {
      if (((TYPE_CODE (type) == TYPE_CODE_PTR)
	   || (TYPE_CODE (type) == TYPE_CODE_REF))
	  && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_CLASS))
        real_type = value_rtti_indirect_type (val, &full, &top, &using_enc);
      else if (TYPE_CODE (type) == TYPE_CODE_CLASS)
	real_type = value_rtti_type (val, &full, &top, &using_enc);
    }

  printf_filtered ("type = ");

  if (!flags.raw)
    create_global_typedef_table (&flags);

  if (real_type)
    {
      printf_filtered ("/* real type = ");
      type_print (real_type, "", gdb_stdout, -1);
      if (! full)
        printf_filtered (" (incomplete object)");
      printf_filtered (" */\n");    
    }

  LA_PRINT_TYPE (type, "", gdb_stdout, show, 0, &flags);
  printf_filtered ("\n");

  do_cleanups (old_chain);
}

static void
whatis_command (char *exp, int from_tty)
{
  /* Most of the time users do not want to see all the fields
     in a structure.  If they do they can use the "ptype" command.
     Hence the "-1" below.  */
  whatis_exp (exp, -1);
}

/* TYPENAME is either the name of a type, or an expression.  */

static void
ptype_command (char *typename, int from_tty)
{
  whatis_exp (typename, 1);
}

/* Print integral scalar data VAL, of type TYPE, onto stdio stream STREAM.
   Used to print data from type structures in a specified type.  For example,
   array bounds may be characters or booleans in some languages, and this
   allows the ranges to be printed in their "natural" form rather than as
   decimal integer values.

   FIXME:  This is here simply because only the type printing routines
   currently use it, and it wasn't clear if it really belonged somewhere
   else (like printcmd.c).  There are a lot of other gdb routines that do
   something similar, but they are generally concerned with printing values
   that come from the inferior in target byte order and target size.  */

void
print_type_scalar (struct type *type, LONGEST val, struct ui_file *stream)
{
  unsigned int i;
  unsigned len;

  CHECK_TYPEDEF (type);

  switch (TYPE_CODE (type))
    {

    case TYPE_CODE_ENUM:
      len = TYPE_NFIELDS (type);
      for (i = 0; i < len; i++)
	{
	  if (TYPE_FIELD_ENUMVAL (type, i) == val)
	    {
	      break;
	    }
	}
      if (i < len)
	{
	  fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	}
      else
	{
	  print_longest (stream, 'd', 0, val);
	}
      break;

    case TYPE_CODE_INT:
      print_longest (stream, TYPE_UNSIGNED (type) ? 'u' : 'd', 0, val);
      break;

    case TYPE_CODE_CHAR:
      LA_PRINT_CHAR ((unsigned char) val, type, stream);
      break;

    case TYPE_CODE_BOOL:
      fprintf_filtered (stream, val ? "TRUE" : "FALSE");
      break;

    case TYPE_CODE_RANGE:
      print_type_scalar (TYPE_TARGET_TYPE (type), val, stream);
      return;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_SET:
    case TYPE_CODE_STRING:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_METHODPTR:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_REF:
    case TYPE_CODE_NAMESPACE:
      error (_("internal error: unhandled type in print_type_scalar"));
      break;

    default:
      error (_("Invalid type code in symbol table."));
    }
  gdb_flush (stream);
}

/* Dump details of a type specified either directly or indirectly.
   Uses the same sort of type lookup mechanism as ptype_command()
   and whatis_command().  */

void
maintenance_print_type (char *typename, int from_tty)
{
  struct value *val;
  struct type *type;
  struct cleanup *old_chain;
  struct expression *expr;

  if (typename != NULL)
    {
      expr = parse_expression (typename);
      old_chain = make_cleanup (free_current_contents, &expr);
      if (expr->elts[0].opcode == OP_TYPE)
	{
	  /* The user expression names a type directly, just use that type.  */
	  type = expr->elts[1].type;
	}
      else
	{
	  /* The user expression may name a type indirectly by naming an
	     object of that type.  Find that indirectly named type.  */
	  val = evaluate_type (expr);
	  type = value_type (val);
	}
      if (type != NULL)
	{
	  recursive_dump_type (type, 0);
	}
      do_cleanups (old_chain);
    }
}


struct cmd_list_element *setprinttypelist;

struct cmd_list_element *showprinttypelist;

static void
set_print_type (char *arg, int from_tty)
{
  printf_unfiltered (
     "\"set print type\" must be followed by the name of a subcommand.\n");
  help_list (setprintlist, "set print type ", -1, gdb_stdout);
}

static void
show_print_type (char *args, int from_tty)
{
  cmd_show_list (showprinttypelist, from_tty, "");
}

static int print_methods = 1;

static void
set_print_type_methods (char *args, int from_tty, struct cmd_list_element *c)
{
  default_ptype_flags.print_methods = print_methods;
}

static void
show_print_type_methods (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of methods defined in a class in %s\n"),
		    value);
}

static int print_typedefs = 1;

static void
set_print_type_typedefs (char *args, int from_tty, struct cmd_list_element *c)
{
  default_ptype_flags.print_typedefs = print_typedefs;
}

static void
show_print_type_typedefs (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Printing of typedefs defined in a class in %s\n"),
		    value);
}

void
_initialize_typeprint (void)
{
  struct cmd_list_element *c;

  c = add_com ("ptype", class_vars, ptype_command, _("\
Print definition of type TYPE.\n\
Usage: ptype[/FLAGS] TYPE | EXPRESSION\n\
Argument may be any type (for example a type name defined by typedef,\n\
or \"struct STRUCT-TAG\" or \"class CLASS-NAME\" or \"union UNION-TAG\"\n\
or \"enum ENUM-TAG\") or an expression.\n\
The selected stack frame's lexical context is used to look up the name.\n\
Contrary to \"whatis\", \"ptype\" always unrolls any typedefs.\n\
\n\
Available FLAGS are:\n\
  /r    print in \"raw\" form; do not substitute typedefs\n\
  /m    do not print methods defined in a class\n\
  /M    print methods defined in a class\n\
  /t    do not print typedefs defined in a class\n\
  /T    print typedefs defined in a class"));
  set_cmd_completer (c, expression_completer);

  c = add_com ("whatis", class_vars, whatis_command,
	       _("Print data type of expression EXP.\n\
Only one level of typedefs is unrolled.  See also \"ptype\"."));
  set_cmd_completer (c, expression_completer);

  add_prefix_cmd ("type", no_class, show_print_type,
		  _("Generic command for showing type-printing settings."),
		  &showprinttypelist, "show print type ", 0, &showprintlist);
  add_prefix_cmd ("type", no_class, set_print_type,
		  _("Generic command for setting how types print."),
		  &setprinttypelist, "show print type ", 0, &setprintlist);

  add_setshow_boolean_cmd ("methods", no_class, &print_methods,
			   _("\
Set printing of methods defined in classes."), _("\
Show printing of methods defined in classes."), NULL,
			   set_print_type_methods,
			   show_print_type_methods,
			   &setprinttypelist, &showprinttypelist);
  add_setshow_boolean_cmd ("typedefs", no_class, &print_typedefs,
			   _("\
Set printing of typedefs defined in classes."), _("\
Show printing of typedefs defined in classes."), NULL,
			   set_print_type_typedefs,
			   show_print_type_typedefs,
			   &setprinttypelist, &showprinttypelist);
}
