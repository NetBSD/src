/* Skipping uninteresting files and functions while stepping.

   Copyright (C) 2011-2013 Free Software Foundation, Inc.

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
#include "gdb_string.h"
#include "symtab.h"
#include "gdbcmd.h"
#include "command.h"
#include "completer.h"
#include "stack.h"
#include "cli/cli-utils.h"
#include "arch-utils.h"
#include "linespec.h"
#include "objfiles.h"
#include "exceptions.h"
#include "breakpoint.h" /* for get_sal_arch () */
#include "source.h"
#include "filenames.h"

struct skiplist_entry
{
  int number;

  /* NULL if this isn't a skiplist entry for an entire file.
     The skiplist entry owns this pointer.  */
  char *filename;

  /* The name of the marked-for-skip function, if this is a skiplist
     entry for a function.
     The skiplist entry owns this pointer.  */
  char *function_name;

  int enabled;

  struct skiplist_entry *next;
};

static void add_skiplist_entry (struct skiplist_entry *e);
static void skip_function (const char *name);

static struct skiplist_entry *skiplist_entry_chain;
static int skiplist_entry_count;

#define ALL_SKIPLIST_ENTRIES(E) \
  for (E = skiplist_entry_chain; E; E = E->next)

#define ALL_SKIPLIST_ENTRIES_SAFE(E,TMP) \
  for (E = skiplist_entry_chain;         \
       E ? (TMP = E->next, 1) : 0;       \
       E = TMP)

static void
skip_file_command (char *arg, int from_tty)
{
  struct skiplist_entry *e;
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
    {
      symtab = lookup_symtab (arg);
      if (symtab == NULL)
	{
	  fprintf_filtered (gdb_stderr, _("No source file named %s.\n"), arg);
	  if (!nquery (_("\
Ignore file pending future shared library load? ")))
	    return;
	}
      /* Do not use SYMTAB's filename, later loaded shared libraries may match
         given ARG but not SYMTAB's filename.  */
      filename = arg;
    }

  e = XZALLOC (struct skiplist_entry);
  e->filename = xstrdup (filename);
  e->enabled = 1;

  add_skiplist_entry (e);

  printf_filtered (_("File %s will be skipped when stepping.\n"), filename);
}

static void
skip_function_command (char *arg, int from_tty)
{
  const char *name = NULL;

  /* Default to the current function if no argument is given.  */
  if (arg == NULL)
    {
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
    }
  else
    {
      if (lookup_symbol (arg, NULL, VAR_DOMAIN, NULL) == NULL)
        {
	  fprintf_filtered (gdb_stderr,
			    _("No function found named %s.\n"), arg);

	  if (nquery (_("\
Ignore function pending future shared library load? ")))
	    {
	      /* Add the unverified skiplist entry.  */
	      skip_function (arg);
	    }
	  return;
	}

      skip_function (arg);
    }
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
	ui_out_message (current_uiout, 0, _("\
Not skipping any files or functions.\n"));
      else
	ui_out_message (current_uiout, 0,
			_("No skiplist entries found with number %s.\n"), arg);

      return;
    }

  tbl_chain = make_cleanup_ui_out_table_begin_end (current_uiout, 4,
						   num_printable_entries,
						   "SkiplistTable");

  ui_out_table_header (current_uiout, 7, ui_left, "number", "Num");      /* 1 */
  ui_out_table_header (current_uiout, 14, ui_left, "type", "Type");      /* 2 */
  ui_out_table_header (current_uiout, 3, ui_left, "enabled", "Enb");     /* 3 */
  ui_out_table_header (current_uiout, 40, ui_noalign, "what", "What");   /* 4 */
  ui_out_table_body (current_uiout);

  ALL_SKIPLIST_ENTRIES (e)
    {
      struct cleanup *entry_chain;

      QUIT;
      if (arg != NULL && !number_is_in_list (arg, e->number))
	continue;

      entry_chain = make_cleanup_ui_out_tuple_begin_end (current_uiout,
							 "blklst-entry");
      ui_out_field_int (current_uiout, "number", e->number);             /* 1 */

      if (e->function_name != NULL)
	ui_out_field_string (current_uiout, "type", "function");         /* 2 */
      else if (e->filename != NULL)
	ui_out_field_string (current_uiout, "type", "file");             /* 2 */
      else
	internal_error (__FILE__, __LINE__, _("\
Skiplist entry should have either a filename or a function name."));

      if (e->enabled)
	ui_out_field_string (current_uiout, "enabled", "y");             /* 3 */
      else
	ui_out_field_string (current_uiout, "enabled", "n");             /* 3 */

      if (e->function_name != NULL)
	ui_out_field_string (current_uiout, "what", e->function_name);	 /* 4 */
      else if (e->filename != NULL)
	ui_out_field_string (current_uiout, "what", e->filename);	 /* 4 */

      ui_out_text (current_uiout, "\n");
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

	xfree (e->function_name);
	xfree (e->filename);
	xfree (e);
        found = 1;
      }
    else
      {
	b_prev = e;
      }

  if (!found)
    error (_("No skiplist entries found with number %s."), arg);
}

/* Create a skiplist entry for the given function NAME and add it to the
   list.  */

static void
skip_function (const char *name)
{
  struct skiplist_entry *e = XZALLOC (struct skiplist_entry);

  e->enabled = 1;
  e->function_name = xstrdup (name);

  add_skiplist_entry (e);

  printf_filtered (_("Function %s will be skipped when stepping.\n"), name);
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


/* See skip.h.  */

int
function_name_is_marked_for_skip (const char *function_name,
				  const struct symtab_and_line *function_sal)
{
  int searched_for_fullname = 0;
  const char *fullname = NULL;
  struct skiplist_entry *e;

  if (function_name == NULL)
    return 0;

  ALL_SKIPLIST_ENTRIES (e)
    {
      if (!e->enabled)
	continue;

      /* Does the pc we're stepping into match e's stored pc? */
      if (e->function_name != NULL
	  && strcmp_iw (function_name, e->function_name) == 0)
	return 1;

      if (e->filename != NULL)
	{
	  /* Check first sole SYMTAB->FILENAME.  It does not need to be
	     a substring of symtab_to_fullname as it may contain "./" etc.  */
	  if (function_sal->symtab != NULL
	      && compare_filenames_for_search (function_sal->symtab->filename,
					       e->filename))
	    return 1;

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if (!basenames_may_differ
	      && (function_sal->symtab == NULL
	          || filename_cmp (lbasename (function_sal->symtab->filename),
				   lbasename (e->filename)) != 0))
	    continue;

	  /* Get the filename corresponding to this FUNCTION_SAL, if we haven't
	     yet.  */
	  if (!searched_for_fullname)
	    {
	      if (function_sal->symtab != NULL)
		fullname = symtab_to_fullname (function_sal->symtab);
	      searched_for_fullname = 1;
	    }
	  if (fullname != NULL
	      && compare_filenames_for_search (fullname, e->filename))
	    return 1;
	}
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

  add_prefix_cmd ("skip", class_breakpoint, skip_function_command, _("\
Ignore a function while stepping.\n\
Usage: skip [FUNCTION NAME]\n\
If no function name is given, ignore the current function."),
                  &skiplist, "skip ", 1, &cmdlist);

  c = add_cmd ("file", class_breakpoint, skip_file_command, _("\
Ignore a file while stepping.\n\
Usage: skip file [FILENAME]\n\
If no filename is given, ignore the current file."),
	       &skiplist);
  set_cmd_completer (c, filename_completer);

  c = add_cmd ("function", class_breakpoint, skip_function_command, _("\
Ignore a function while stepping.\n\
Usage: skip function [FUNCTION NAME]\n\
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
