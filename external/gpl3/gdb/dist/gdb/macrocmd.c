/* C preprocessor macro expansion commands for GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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
#include "macrotab.h"
#include "macroexp.h"
#include "macroscope.h"
#include "cli/cli-utils.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdb_string.h"
#include "linespec.h"


/* The `macro' prefix command.  */

static struct cmd_list_element *macrolist;

static void
macro_command (char *arg, int from_tty)
{
  printf_unfiltered
    ("\"macro\" must be followed by the name of a macro command.\n");
  help_list (macrolist, "macro ", -1, gdb_stdout);
}



/* Macro expansion commands.  */


/* Prints an informational message regarding the lack of macro information.  */
static void macro_inform_no_debuginfo()
{
  puts_filtered ("GDB has no preprocessor macro information for that code.\n");
}

static void
macro_expand_command (char *exp, int from_tty)
{
  struct macro_scope *ms = NULL;
  char *expanded = NULL;
  struct cleanup *cleanup_chain = make_cleanup (free_current_contents, &ms);

  make_cleanup (free_current_contents, &expanded);

  /* You know, when the user doesn't specify any expression, it would be
     really cool if this defaulted to the last expression evaluated.
     Then it would be easy to ask, "Hey, what did I just evaluate?"  But
     at the moment, the `print' commands don't save the last expression
     evaluated, just its value.  */
  if (! exp || ! *exp)
    error (_("You must follow the `macro expand' command with the"
           " expression you\n"
           "want to expand."));

  ms = default_macro_scope ();
  if (ms)
    {
      expanded = macro_expand (exp, standard_macro_lookup, ms);
      fputs_filtered ("expands to: ", gdb_stdout);
      fputs_filtered (expanded, gdb_stdout);
      fputs_filtered ("\n", gdb_stdout);
    }
  else
    macro_inform_no_debuginfo ();

  do_cleanups (cleanup_chain);
  return;
}


static void
macro_expand_once_command (char *exp, int from_tty)
{
  struct macro_scope *ms = NULL;
  char *expanded = NULL;
  struct cleanup *cleanup_chain = make_cleanup (free_current_contents, &ms);
  make_cleanup (free_current_contents, &expanded);

  /* You know, when the user doesn't specify any expression, it would be
     really cool if this defaulted to the last expression evaluated.
     And it should set the once-expanded text as the new `last
     expression'.  That way, you could just hit return over and over and
     see the expression expanded one level at a time.  */
  if (! exp || ! *exp)
    error (_("You must follow the `macro expand-once' command with"
           " the expression\n"
           "you want to expand."));

  ms = default_macro_scope ();
  if (ms)
    {
      expanded = macro_expand_once (exp, standard_macro_lookup, ms);
      fputs_filtered ("expands to: ", gdb_stdout);
      fputs_filtered (expanded, gdb_stdout);
      fputs_filtered ("\n", gdb_stdout);
    }
  else
    macro_inform_no_debuginfo ();

  do_cleanups (cleanup_chain);
  return;
}

/*  Outputs the include path of a macro starting at FILE and LINE to STREAM.

    Care should be taken that this function does not cause any lookups into
    the splay tree so that it can be safely used while iterating.  */
static void
show_pp_source_pos (struct ui_file *stream,
                    struct macro_source_file *file,
                    int line)
{
  char *fullname;

  fullname = macro_source_fullname (file);
  fprintf_filtered (stream, "%s:%d\n", fullname, line);
  xfree (fullname);

  while (file->included_by)
    {
      fullname = macro_source_fullname (file->included_by);
      fprintf_filtered (gdb_stdout, "  included at %s:%d\n", fullname,
                        file->included_at_line);
      xfree (fullname);
      file = file->included_by;
    }
}

/* Outputs a macro for human consumption, detailing the include path
   and macro definition.  NAME is the name of the macro.
   D the definition.  FILE the start of the include path, and LINE the
   line number in FILE.

   Care should be taken that this function does not cause any lookups into
   the splay tree so that it can be safely used while iterating.  */
static void
print_macro_definition (const char *name,
			const struct macro_definition *d,
			struct macro_source_file *file,
			int line)
{
      fprintf_filtered (gdb_stdout, "Defined at ");
      show_pp_source_pos (gdb_stdout, file, line);

      if (line != 0)
	fprintf_filtered (gdb_stdout, "#define %s", name);
      else
	fprintf_filtered (gdb_stdout, "-D%s", name);

      if (d->kind == macro_function_like)
        {
          int i;

          fputs_filtered ("(", gdb_stdout);
          for (i = 0; i < d->argc; i++)
            {
              fputs_filtered (d->argv[i], gdb_stdout);
              if (i + 1 < d->argc)
                fputs_filtered (", ", gdb_stdout);
            }
          fputs_filtered (")", gdb_stdout);
        }

      if (line != 0)
	fprintf_filtered (gdb_stdout, " %s\n", d->replacement);
      else
	fprintf_filtered (gdb_stdout, "=%s\n", d->replacement);
}

/* A callback function for usage with macro_for_each and friends.
   If USER_DATA is null all macros will be printed.
   Otherwise USER_DATA is considered to be a string, printing
   only macros who's NAME matches USER_DATA.  Other arguments are
   routed to print_macro_definition.  */
static void
print_macro_callback (const char *name, const struct macro_definition *macro,
		   struct macro_source_file *source, int line,
		   void *user_data)
{
  if (! user_data || strcmp (user_data, name) == 0)
    print_macro_definition (name, macro, source, line);
}

/* The implementation of the `info macro' command.  */
static void
info_macro_command (char *args, int from_tty)
{
  struct macro_scope *ms = NULL;
  struct cleanup *cleanup_chain;
  char *name;
  int show_all_macros_named = 0;
  char *arg_start = args;
  int processing_args = 1;

  while (processing_args
	 && arg_start && *arg_start == '-' && *arg_start != '\0')
    {
      char *p = skip_to_space (arg_start);

      if (strncmp (arg_start, "-a", p - arg_start) == 0
	  || strncmp (arg_start, "-all", p - arg_start) == 0)
	show_all_macros_named = 1;
      else if (strncmp (arg_start, "--", p - arg_start) == 0)
          /* Our macro support seems rather C specific but this would
             seem necessary for languages allowing - in macro names.
	     e.g. Scheme's (defmacro ->foo () "bar\n")  */
	processing_args = 0;
      else
	{
	  /* Relies on modified 'args' not making it in to history */
	  *p = '\0';
	  error (_("Unrecognized option '%s' to info macro command.  "
		   "Try \"help info macro\"."), arg_start);
	}

        arg_start = skip_spaces (p);
    }

  name = arg_start;

  if (! name || ! *name)
    error (_("You must follow the `info macro' command with the name"
	     " of the macro\n"
	     "whose definition you want to see."));

  ms = default_macro_scope ();
  cleanup_chain = make_cleanup (free_current_contents, &ms);

  if (! ms)
    macro_inform_no_debuginfo ();
  else if (show_all_macros_named)
    macro_for_each (ms->file->table, print_macro_callback, name);
  else
    {
      struct macro_definition *d;

      d = macro_lookup_definition (ms->file, ms->line, name);
      if (d)
	{
	  int line;
	  struct macro_source_file *file
	    = macro_definition_location (ms->file, ms->line, name, &line);

	  print_macro_definition (name, d, file, line);
	}
      else
        {
          fprintf_filtered (gdb_stdout,
                            "The symbol `%s' has no definition as a C/C++"
                            " preprocessor macro\n"
                            "at ", name);
          show_pp_source_pos (gdb_stdout, ms->file, ms->line);
	}
    }

  do_cleanups (cleanup_chain);
}

/* Implementation of the "info macros" command. */
static void
info_macros_command (char *args, int from_tty)
{
  struct macro_scope *ms = NULL;
  struct cleanup *cleanup_chain = make_cleanup (free_current_contents, &ms);

  if (args == NULL)
    ms = default_macro_scope ();
  else
    {
      struct symtabs_and_lines sals =
	decode_line_with_current_source (args, 0);

      if (sals.nelts)
        ms = sal_macro_scope (sals.sals[0]);
    }

  if (! ms || ! ms->file || ! ms->file->table)
    macro_inform_no_debuginfo ();
  else
    macro_for_each_in_scope (ms->file, ms->line, print_macro_callback, NULL);

  do_cleanups (cleanup_chain);
}


/* User-defined macros.  */

static void
skip_ws (char **expp)
{
  while (macro_is_whitespace (**expp))
    ++*expp;
}

/* Try to find the bounds of an identifier.  If an identifier is
   found, returns a newly allocated string; otherwise returns NULL.
   EXPP is a pointer to an input string; it is updated to point to the
   text following the identifier.  If IS_PARAMETER is true, this
   function will also allow "..." forms as used in varargs macro
   parameters.  */

static char *
extract_identifier (char **expp, int is_parameter)
{
  char *result;
  char *p = *expp;
  unsigned int len;

  if (is_parameter && !strncmp (p, "...", 3))
    {
      /* Ok.  */
    }
  else
    {
      if (! *p || ! macro_is_identifier_nondigit (*p))
	return NULL;
      for (++p;
	   *p && (macro_is_identifier_nondigit (*p) || macro_is_digit (*p));
	   ++p)
	;
    }

  if (is_parameter && !strncmp (p, "...", 3))      
    p += 3;

  len = p - *expp;
  result = (char *) xmalloc (len + 1);
  memcpy (result, *expp, len);
  result[len] = '\0';
  *expp += len;
  return result;
}

/* Helper function to clean up a temporarily-constructed macro object.
   This assumes that the contents were all allocated with xmalloc.  */
static void
free_macro_definition_ptr (void *ptr)
{
  int i;
  struct macro_definition *loc = (struct macro_definition *) ptr;

  for (i = 0; i < loc->argc; ++i)
    xfree ((char *) loc->argv[i]);
  xfree ((char *) loc->argv);
  /* Note that the 'replacement' field is not allocated.  */
}

static void
macro_define_command (char *exp, int from_tty)
{
  struct macro_definition new_macro;
  char *name = NULL;
  struct cleanup *cleanup_chain;

  if (!exp)
    error (_("usage: macro define NAME[(ARGUMENT-LIST)] [REPLACEMENT-LIST]"));

  cleanup_chain = make_cleanup (free_macro_definition_ptr, &new_macro);
  make_cleanup (free_current_contents, &name);

  memset (&new_macro, 0, sizeof (struct macro_definition));

  skip_ws (&exp);
  name = extract_identifier (&exp, 0);
  if (! name)
    error (_("Invalid macro name."));
  if (*exp == '(')
    {
      /* Function-like macro.  */
      int alloced = 5;
      char **argv = (char **) xmalloc (alloced * sizeof (char *));

      new_macro.kind = macro_function_like;
      new_macro.argc = 0;
      new_macro.argv = (const char * const *) argv;

      /* Skip the '(' and whitespace.  */
      ++exp;
      skip_ws (&exp);

      while (*exp != ')')
	{
	  int i;

	  if (new_macro.argc == alloced)
	    {
	      alloced *= 2;
	      argv = (char **) xrealloc (argv, alloced * sizeof (char *));
	      /* Must update new_macro as well...  */
	      new_macro.argv = (const char * const *) argv;
	    }
	  argv[new_macro.argc] = extract_identifier (&exp, 1);
	  if (! argv[new_macro.argc])
	    error (_("Macro is missing an argument."));
	  ++new_macro.argc;

	  for (i = new_macro.argc - 2; i >= 0; --i)
	    {
	      if (! strcmp (argv[i], argv[new_macro.argc - 1]))
		error (_("Two macro arguments with identical names."));
	    }

	  skip_ws (&exp);
	  if (*exp == ',')
	    {
	      ++exp;
	      skip_ws (&exp);
	    }
	  else if (*exp != ')')
	    error (_("',' or ')' expected at end of macro arguments."));
	}
      /* Skip the closing paren.  */
      ++exp;
      skip_ws (&exp);

      macro_define_function (macro_main (macro_user_macros), -1, name,
			     new_macro.argc, (const char **) new_macro.argv,
			     exp);
    }
  else
    {
      skip_ws (&exp);
      macro_define_object (macro_main (macro_user_macros), -1, name, exp);
    }

  do_cleanups (cleanup_chain);
}


static void
macro_undef_command (char *exp, int from_tty)
{
  char *name;

  if (!exp)
    error (_("usage: macro undef NAME"));

  skip_ws (&exp);
  name = extract_identifier (&exp, 0);
  if (! name)
    error (_("Invalid macro name."));
  macro_undef (macro_main (macro_user_macros), -1, name);
  xfree (name);
}


static void
print_one_macro (const char *name, const struct macro_definition *macro,
		 struct macro_source_file *source, int line,
		 void *ignore)
{
  fprintf_filtered (gdb_stdout, "macro define %s", name);
  if (macro->kind == macro_function_like)
    {
      int i;

      fprintf_filtered (gdb_stdout, "(");
      for (i = 0; i < macro->argc; ++i)
	fprintf_filtered (gdb_stdout, "%s%s", (i > 0) ? ", " : "",
			  macro->argv[i]);
      fprintf_filtered (gdb_stdout, ")");
    }
  fprintf_filtered (gdb_stdout, " %s\n", macro->replacement);
}


static void
macro_list_command (char *exp, int from_tty)
{
  macro_for_each (macro_user_macros, print_one_macro, NULL);
}


/* Initializing the `macrocmd' module.  */

extern initialize_file_ftype _initialize_macrocmd; /* -Wmissing-prototypes */

void
_initialize_macrocmd (void)
{
  /* We introduce a new command prefix, `macro', under which we'll put
     the various commands for working with preprocessor macros.  */
  add_prefix_cmd ("macro", class_info, macro_command,
		  _("Prefix for commands dealing with C preprocessor macros."),
		  &macrolist, "macro ", 0, &cmdlist);

  add_cmd ("expand", no_class, macro_expand_command, _("\
Fully expand any C/C++ preprocessor macro invocations in EXPRESSION.\n\
Show the expanded expression."),
	   &macrolist);
  add_alias_cmd ("exp", "expand", no_class, 1, &macrolist);
  add_cmd ("expand-once", no_class, macro_expand_once_command, _("\
Expand C/C++ preprocessor macro invocations appearing directly in EXPRESSION.\n\
Show the expanded expression.\n\
\n\
This command differs from `macro expand' in that it only expands macro\n\
invocations that appear directly in EXPRESSION; if expanding a macro\n\
introduces further macro invocations, those are left unexpanded.\n\
\n\
`macro expand-once' helps you see how a particular macro expands,\n\
whereas `macro expand' shows you how all the macros involved in an\n\
expression work together to yield a pre-processed expression."),
	   &macrolist);
  add_alias_cmd ("exp1", "expand-once", no_class, 1, &macrolist);

  add_cmd ("macro", no_class, info_macro_command,
	   _("Show the definition of MACRO, and it's source location.\n\
Usage: info macro [-a|-all] [--] MACRO\n\
Options: \n\
  -a, --all    Output all definitions of MACRO in the current compilation\
 unit.\n\
  --           Specify the end of arguments and the beginning of the MACRO."),

	   &infolist);

  add_cmd ("macros", no_class, info_macros_command,
	   _("Show the definitions of all macros at LINESPEC, or the current \
source location.\n\
Usage: info macros [LINESPEC]"),
	   &infolist);

  add_cmd ("define", no_class, macro_define_command, _("\
Define a new C/C++ preprocessor macro.\n\
The GDB command `macro define DEFINITION' is equivalent to placing a\n\
preprocessor directive of the form `#define DEFINITION' such that the\n\
definition is visible in all the inferior's source files.\n\
For example:\n\
  (gdb) macro define PI (3.1415926)\n\
  (gdb) macro define MIN(x,y) ((x) < (y) ? (x) : (y))"),
	   &macrolist);

  add_cmd ("undef", no_class, macro_undef_command, _("\
Remove the definition of the C/C++ preprocessor macro with the given name."),
	   &macrolist);

  add_cmd ("list", no_class, macro_list_command,
	   _("List all the macros defined using the `macro define' command."),
	   &macrolist);
}
