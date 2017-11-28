/* Skipping uninteresting files and functions while stepping.

   Copyright (C) 2011-2017 Free Software Foundation, Inc.

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
#include "skip.h"
#include "value.h"
#include "valprint.h"
#include "ui-out.h"
#include "symtab.h"
#include "gdbcmd.h"
#include "command.h"
#include "completer.h"
#include "stack.h"
#include "cli/cli-utils.h"
#include "arch-utils.h"
#include "linespec.h"
#include "objfiles.h"
#include "breakpoint.h" /* for get_sal_arch () */
#include "source.h"
#include "filenames.h"
#include "fnmatch.h"
#include "gdb_regex.h"

struct skiplist_entry
{
  int number;

  /* Non-zero if FILE is a glob-style pattern.
     Otherewise it is the plain file name (possibly with directories).  */
  int file_is_glob;

  /* The name of the file or NULL.
     The skiplist entry owns this pointer.  */
  char *file;

  /* Non-zero if FUNCTION is a regexp.
     Otherwise it is a plain function name (possibly with arguments,
     for C++).  */
  int function_is_regexp;

  /* The name of the function or NULL.
     The skiplist entry owns this pointer.  */
  char *function;

  /* If this is a function regexp, the compiled form.  */
  regex_t compiled_function_regexp;

  /* Non-zero if the function regexp has been compiled.  */
  int compiled_function_regexp_is_valid;

  int enabled;

  struct skiplist_entry *next;
};

static void add_skiplist_entry (struct skiplist_entry *e);

static struct skiplist_entry *skiplist_entry_chain;
static int skiplist_entry_count;

#define ALL_SKIPLIST_ENTRIES(E) \
  for (E = skiplist_entry_chain; E; E = E->next)

#define ALL_SKIPLIST_ENTRIES_SAFE(E,TMP) \
  for (E = skiplist_entry_chain;         \
       E ? (TMP = E->next, 1) : 0;       \
       E = TMP)

/* Create a skip object.  */

static struct skiplist_entry *
make_skip_entry (int file_is_glob, const char *file,
		 int function_is_regexp, const char *function)
{
  struct skiplist_entry *e = XCNEW (struct skiplist_entry);

  gdb_assert (file != NULL || function != NULL);
  if (file_is_glob)
    gdb_assert (file != NULL);
  if (function_is_regexp)
    gdb_assert (function != NULL);

  if (file != NULL)
    e->file = xstrdup (file);
  if (function != NULL)
    e->function = xstrdup (function);
  e->file_is_glob = file_is_glob;
  e->function_is_regexp = function_is_regexp;
  e->enabled = 1;

  return e;
}

/* Free a skiplist entry.  */

static void
free_skiplist_entry (struct skiplist_entry *e)
{
  xfree (e->file);
  xfree (e->function);
  if (e->function_is_regexp && e->compiled_function_regexp_is_valid)
    regfree (&e->compiled_function_regexp);
  xfree (e);
}

/* Wrapper to free_skiplist_entry for use as a cleanup.  */

static void
free_skiplist_entry_cleanup (void *e)
{
  free_skiplist_entry ((struct skiplist_entry *) e);
}

/* Create a cleanup to free skiplist entry E.  */

static struct cleanup *
make_free_skiplist_entry_cleanup (struct skiplist_entry *e)
{
  return make_cleanup (free_skiplist_entry_cleanup, e);
}

static void
skip_file_command (char *arg, int from_tty)
{
  struct symtab *symtab;
  const char *filename = NULL;

  /* If no argument was given, try to default to the last
     displayed codepoint.  */
  if (arg == NULL)
    {
      symtab = get_last_displayed_symtab ();
      if (symtab == NULL)
	error (_("No default file now."));

      /* It is not a typo, symtab_to_filename_for_display woule be needlessly
	 ambiguous.  */
      filename = symtab_to_fullname (symtab);
    }
  else
    filename = arg;

  add_skiplist_entry (make_skip_entry (0, filename, 0, NULL));

  printf_filtered (_("File %s will be skipped when stepping.\n"), filename);
}

/* Create a skiplist entry for the given function NAME and add it to the
   list.  */

static void
skip_function (const char *name)
{
  add_skiplist_entry (make_skip_entry (0, NULL, 0, name));

  printf_filtered (_("Function %s will be skipped when stepping.\n"), name);
}

static void
skip_function_command (char *arg, int from_tty)
{
  /* Default to the current function if no argument is given.  */
  if (arg == NULL)
    {
      const char *name = NULL;
      CORE_ADDR pc;

      if (!last_displayed_sal_is_valid ())
	error (_("No default function now."));

      pc = get_last_displayed_addr ();
      if (!find_pc_partial_function (pc, &name, NULL, NULL))
	{
	  error (_("No function found containing current program point %s."),
		  paddress (get_current_arch (), pc));
	}
      skip_function (name);
      return;
    }

  skip_function (arg);
}

/* Compile the regexp in E.
   An error is thrown if there's an error.
   MESSAGE is used as a prefix of the error message.  */

static void
compile_skip_regexp (struct skiplist_entry *e, const char *message)
{
  int code;
  int flags = REG_NOSUB;

#ifdef REG_EXTENDED
  flags |= REG_EXTENDED;
#endif

  gdb_assert (e->function_is_regexp && e->function != NULL);

  code = regcomp (&e->compiled_function_regexp, e->function, flags);
  if (code != 0)
    {
      char *err = get_regcomp_error (code, &e->compiled_function_regexp);

      make_cleanup (xfree, err);
      error (_("%s: %s"), message, err);
    }
  e->compiled_function_regexp_is_valid = 1;
}

/* Process "skip ..." that does not match "skip file" or "skip function".  */

static void
skip_command (char *arg, int from_tty)
{
  const char *file = NULL;
  const char *gfile = NULL;
  const char *function = NULL;
  const char *rfunction = NULL;
  char **argv;
  struct cleanup *cleanups;
  struct skiplist_entry *e;
  int i;

  if (arg == NULL)
    {
      skip_function_command (arg, from_tty);
      return;
    }

  argv = buildargv (arg);
  cleanups = make_cleanup_freeargv (argv);

  for (i = 0; argv[i] != NULL; ++i)
    {
      const char *p = argv[i];
      const char *value = argv[i + 1];

      if (strcmp (p, "-fi") == 0
	  || strcmp (p, "-file") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  file = value;
	  ++i;
	}
      else if (strcmp (p, "-gfi") == 0
	       || strcmp (p, "-gfile") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  gfile = value;
	  ++i;
	}
      else if (strcmp (p, "-fu") == 0
	       || strcmp (p, "-function") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  function = value;
	  ++i;
	}
      else if (strcmp (p, "-rfu") == 0
	       || strcmp (p, "-rfunction") == 0)
	{
	  if (value == NULL)
	    error (_("Missing value for %s option."), p);
	  rfunction = value;
	  ++i;
	}
      else if (*p == '-')
	error (_("Invalid skip option: %s"), p);
      else if (i == 0)
	{
	  /* Assume the user entered "skip FUNCTION-NAME".
	     FUNCTION-NAME may be `foo (int)', and therefore we pass the
	     complete original arg to skip_function command as if the user
	     typed "skip function arg".  */
	  do_cleanups (cleanups);
	  skip_function_command (arg, from_tty);
	  return;
	}
      else
	error (_("Invalid argument: %s"), p);
    }

  if (file != NULL && gfile != NULL)
    error (_("Cannot specify both -file and -gfile."));

  if (function != NULL && rfunction != NULL)
    error (_("Cannot specify both -function and -rfunction."));

  /* This shouldn't happen as "skip" by itself gets punted to
     skip_function_command.  */
  gdb_assert (file != NULL || gfile != NULL
	      || function != NULL || rfunction != NULL);

  e = make_skip_entry (gfile != NULL, file ? file : gfile,
		       rfunction != NULL, function ? function : rfunction);
  if (rfunction != NULL)
    {
      struct cleanup *rf_cleanups = make_free_skiplist_entry_cleanup (e);

      compile_skip_regexp (e, _("regexp"));
      discard_cleanups (rf_cleanups);
    }
  add_skiplist_entry (e);

  /* I18N concerns drive some of the choices here (we can't piece together
     the output too much).  OTOH we want to keep this simple.  Therefore the
     only polish we add to the output is to append "(s)" to "File" or
     "Function" if they're a glob/regexp.  */
  {
    const char *file_to_print = file != NULL ? file : gfile;
    const char *function_to_print = function != NULL ? function : rfunction;
    const char *file_text = gfile != NULL ? _("File(s)") : _("File");
    const char *lower_file_text = gfile != NULL ? _("file(s)") : _("file");
    const char *function_text
      = rfunction != NULL ? _("Function(s)") : _("Function");

    if (function_to_print == NULL)
      {
	printf_filtered (_("%s %s will be skipped when stepping.\n"),
			 file_text, file_to_print);
      }
    else if (file_to_print == NULL)
      {
	printf_filtered (_("%s %s will be skipped when stepping.\n"),
			 function_text, function_to_print);
      }
    else
      {
	printf_filtered (_("%s %s in %s %s will be skipped"
			   " when stepping.\n"),
			 function_text, function_to_print,
			 lower_file_text, file_to_print);
      }
  }

  do_cleanups (cleanups);
}

static void
skip_info (char *arg, int from_tty)
{
  struct skiplist_entry *e;
  int num_printable_entries = 0;
  struct value_print_options opts;
  struct cleanup *tbl_chain;

  get_user_print_options (&opts);

  /* Count the number of rows in the table and see if we need space for a
     64-bit address anywhere.  */
  ALL_SKIPLIST_ENTRIES (e)
    if (arg == NULL || number_is_in_list (arg, e->number))
      num_printable_entries++;

  if (num_printable_entries == 0)
    {
      if (arg == NULL)
	current_uiout->message (_("Not skipping any files or functions.\n"));
      else
	current_uiout->message (
	  _("No skiplist entries found with number %s.\n"), arg);

      return;
    }

  tbl_chain = make_cleanup_ui_out_table_begin_end (current_uiout, 6,
						   num_printable_entries,
						   "SkiplistTable");

  current_uiout->table_header (5, ui_left, "number", "Num");   /* 1 */
  current_uiout->table_header (3, ui_left, "enabled", "Enb");  /* 2 */
  current_uiout->table_header (4, ui_right, "regexp", "Glob"); /* 3 */
  current_uiout->table_header (20, ui_left, "file", "File");   /* 4 */
  current_uiout->table_header (2, ui_right, "regexp", "RE");   /* 5 */
  current_uiout->table_header (40, ui_noalign, "function", "Function"); /* 6 */
  current_uiout->table_body ();

  ALL_SKIPLIST_ENTRIES (e)
    {
      struct cleanup *entry_chain;

      QUIT;
      if (arg != NULL && !number_is_in_list (arg, e->number))
	continue;

      entry_chain = make_cleanup_ui_out_tuple_begin_end (current_uiout,
							 "blklst-entry");
      current_uiout->field_int ("number", e->number); /* 1 */

      if (e->enabled)
	current_uiout->field_string ("enabled", "y"); /* 2 */
      else
	current_uiout->field_string ("enabled", "n"); /* 2 */

      if (e->file_is_glob)
	current_uiout->field_string ("regexp", "y"); /* 3 */
      else
	current_uiout->field_string ("regexp", "n"); /* 3 */

      current_uiout->field_string ("file",
			   e->file ? e->file : "<none>"); /* 4 */
      if (e->function_is_regexp)
	current_uiout->field_string ("regexp", "y"); /* 5 */
      else
	current_uiout->field_string ("regexp", "n"); /* 5 */

      current_uiout->field_string (
	"function", e->function ? e->function : "<none>"); /* 6 */

      current_uiout->text ("\n");
      do_cleanups (entry_chain);
    }

  do_cleanups (tbl_chain);
}

static void
skip_enable_command (char *arg, int from_tty)
{
  struct skiplist_entry *e;
  int found = 0;

  ALL_SKIPLIST_ENTRIES (e)
    if (arg == NULL || number_is_in_list (arg, e->number))
      {
        e->enabled = 1;
        found = 1;
      }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

static void
skip_disable_command (char *arg, int from_tty)
{
  struct skiplist_entry *e;
  int found = 0;

  ALL_SKIPLIST_ENTRIES (e)
    if (arg == NULL || number_is_in_list (arg, e->number))
      {
	e->enabled = 0;
        found = 1;
      }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

static void
skip_delete_command (char *arg, int from_tty)
{
  struct skiplist_entry *e, *temp, *b_prev;
  int found = 0;

  b_prev = 0;
  ALL_SKIPLIST_ENTRIES_SAFE (e, temp)
    if (arg == NULL || number_is_in_list (arg, e->number))
      {
	if (b_prev != NULL)
	  b_prev->next = e->next;
	else
	  skiplist_entry_chain = e->next;

	free_skiplist_entry (e);
        found = 1;
      }
    else
      {
	b_prev = e;
      }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

/* Add the given skiplist entry to our list, and set the entry's number.  */

static void
add_skiplist_entry (struct skiplist_entry *e)
{
  struct skiplist_entry *e1;

  e->number = ++skiplist_entry_count;

  /* Add to the end of the chain so that the list of
     skiplist entries will be in numerical order.  */

  e1 = skiplist_entry_chain;
  if (e1 == NULL)
    skiplist_entry_chain = e;
  else
    {
      while (e1->next)
	e1 = e1->next;
      e1->next = e;
    }
}

/* Return non-zero if we're stopped at a file to be skipped.  */

static int
skip_file_p (struct skiplist_entry *e,
	     const struct symtab_and_line *function_sal)
{
  gdb_assert (e->file != NULL && !e->file_is_glob);

  if (function_sal->symtab == NULL)
    return 0;

  /* Check first sole SYMTAB->FILENAME.  It may not be a substring of
     symtab_to_fullname as it may contain "./" etc.  */
  if (compare_filenames_for_search (function_sal->symtab->filename, e->file))
    return 1;

  /* Before we invoke realpath, which can get expensive when many
     files are involved, do a quick comparison of the basenames.  */
  if (!basenames_may_differ
      && filename_cmp (lbasename (function_sal->symtab->filename),
		       lbasename (e->file)) != 0)
    return 0;

  /* Note: symtab_to_fullname caches its result, thus we don't have to.  */
  {
    const char *fullname = symtab_to_fullname (function_sal->symtab);

    if (compare_filenames_for_search (fullname, e->file))
      return 1;
  }

  return 0;
}

/* Return non-zero if we're stopped at a globbed file to be skipped.  */

static int
skip_gfile_p (struct skiplist_entry *e,
	      const struct symtab_and_line *function_sal)
{
  gdb_assert (e->file != NULL && e->file_is_glob);

  if (function_sal->symtab == NULL)
    return 0;

  /* Check first sole SYMTAB->FILENAME.  It may not be a substring of
     symtab_to_fullname as it may contain "./" etc.  */
  if (gdb_filename_fnmatch (e->file, function_sal->symtab->filename,
			    FNM_FILE_NAME | FNM_NOESCAPE) == 0)
    return 1;

  /* Before we invoke symtab_to_fullname, which is expensive, do a quick
     comparison of the basenames.
     Note that we assume that lbasename works with glob-style patterns.
     If the basename of the glob pattern is something like "*.c" then this
     isn't much of a win.  Oh well.  */
  if (!basenames_may_differ
      && gdb_filename_fnmatch (lbasename (e->file),
			       lbasename (function_sal->symtab->filename),
			       FNM_FILE_NAME | FNM_NOESCAPE) != 0)
    return 0;

  /* Note: symtab_to_fullname caches its result, thus we don't have to.  */
  {
    const char *fullname = symtab_to_fullname (function_sal->symtab);

    if (compare_glob_filenames_for_search (fullname, e->file))
      return 1;
  }

  return 0;
}

/* Return non-zero if we're stopped at a function to be skipped.  */

static int
skip_function_p (struct skiplist_entry *e, const char *function_name)
{
  gdb_assert (e->function != NULL && !e->function_is_regexp);
  return strcmp_iw (function_name, e->function) == 0;
}

/* Return non-zero if we're stopped at a function regexp to be skipped.  */

static int
skip_rfunction_p (struct skiplist_entry *e, const char *function_name)
{
  gdb_assert (e->function != NULL && e->function_is_regexp
	      && e->compiled_function_regexp_is_valid);
  return (regexec (&e->compiled_function_regexp, function_name, 0, NULL, 0)
	  == 0);
}

/* See skip.h.  */

int
function_name_is_marked_for_skip (const char *function_name,
				  const struct symtab_and_line *function_sal)
{
  struct skiplist_entry *e;

  if (function_name == NULL)
    return 0;

  ALL_SKIPLIST_ENTRIES (e)
    {
      int skip_by_file = 0;
      int skip_by_function = 0;

      if (!e->enabled)
	continue;

      if (e->file != NULL)
	{
	  if (e->file_is_glob)
	    {
	      if (skip_gfile_p (e, function_sal))
		skip_by_file = 1;
	    }
	  else
	    {
	      if (skip_file_p (e, function_sal))
		skip_by_file = 1;
	    }
	}
      if (e->function != NULL)
	{
	  if (e->function_is_regexp)
	    {
	      if (skip_rfunction_p (e, function_name))
		skip_by_function = 1;
	    }
	  else
	    {
	      if (skip_function_p (e, function_name))
		skip_by_function = 1;
	    }
	}

      /* If both file and function must match, make sure we don't errantly
	 exit if only one of them match.  */
      if (e->file != NULL && e->function != NULL)
	{
	  if (skip_by_file && skip_by_function)
	    return 1;
	}
      /* Only one of file/function is specified.  */
      else if (skip_by_file || skip_by_function)
	return 1;
    }

  return 0;
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_step_skip;

void
_initialize_step_skip (void)
{
  static struct cmd_list_element *skiplist = NULL;
  struct cmd_list_element *c;

  skiplist_entry_chain = 0;
  skiplist_entry_count = 0;

  add_prefix_cmd ("skip", class_breakpoint, skip_command, _("\
Ignore a function while stepping.\n\
\n\
Usage: skip [FUNCTION-NAME]\n\
       skip [<file-spec>] [<function-spec>]\n\
If no arguments are given, ignore the current function.\n\
\n\
<file-spec> is one of:\n\
       -fi|-file FILE-NAME\n\
       -gfi|-gfile GLOB-FILE-PATTERN\n\
<function-spec> is one of:\n\
       -fu|-function FUNCTION-NAME\n\
       -rfu|-rfunction FUNCTION-NAME-REGULAR-EXPRESSION"),
                  &skiplist, "skip ", 1, &cmdlist);

  c = add_cmd ("file", class_breakpoint, skip_file_command, _("\
Ignore a file while stepping.\n\
Usage: skip file [FILE-NAME]\n\
If no filename is given, ignore the current file."),
	       &skiplist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("function", class_breakpoint, skip_function_command, _("\
Ignore a function while stepping.\n\
Usage: skip function [FUNCTION-NAME]\n\
If no function name is given, skip the current function."),
	       &skiplist);
  set_cmd_completer (c, location_completer);

  add_cmd ("enable", class_breakpoint, skip_enable_command, _("\
Enable skip entries.  You can specify numbers (e.g. \"skip enable 1 3\"), \
ranges (e.g. \"skip enable 4-8\"), or both (e.g. \"skip enable 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll enable all skip entries.\n\n\
Usage: skip enable [NUMBERS AND/OR RANGES]"),
	   &skiplist);

  add_cmd ("disable", class_breakpoint, skip_disable_command, _("\
Disable skip entries.  You can specify numbers (e.g. \"skip disable 1 3\"), \
ranges (e.g. \"skip disable 4-8\"), or both (e.g. \"skip disable 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll disable all skip entries.\n\n\
Usage: skip disable [NUMBERS AND/OR RANGES]"),
	   &skiplist);

  add_cmd ("delete", class_breakpoint, skip_delete_command, _("\
Delete skip entries.  You can specify numbers (e.g. \"skip delete 1 3\"), \
ranges (e.g. \"skip delete 4-8\"), or both (e.g. \"skip delete 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll delete all skip entries.\n\n\
Usage: skip delete [NUMBERS AND/OR RANGES]"),
           &skiplist);

  add_info ("skip", skip_info, _("\
Display the status of skips.  You can specify numbers (e.g. \"skip info 1 3\"), \
ranges (e.g. \"skip info 4-8\"), or both (e.g. \"skip info 1 3 4-8\").\n\n\
If you don't specify any numbers or ranges, we'll show all skips.\n\n\
Usage: skip info [NUMBERS AND/OR RANGES]\n\
The \"Type\" column indicates one of:\n\
\tfile        - ignored file\n\
\tfunction    - ignored function"));
}
