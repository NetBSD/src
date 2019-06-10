/* Command-line output logging for GDB, the GNU debugger.

   Copyright (C) 2003-2017 Free Software Foundation, Inc.

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
#include "gdbcmd.h"
#include "ui-out.h"
#include "interps.h"

static char *saved_filename;

static char *logging_filename;
static void
show_logging_filename (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("The current logfile is \"%s\".\n"),
		    value);
}

static int logging_overwrite;

static void
maybe_warn_already_logging ()
{
  if (saved_filename)
    warning (_("Currently logging to %s.  Turn the logging off and on to "
	       "make the new setting effective."), saved_filename);
}

static void
set_logging_overwrite (char *args, int from_tty, struct cmd_list_element *c)
{
  maybe_warn_already_logging ();
}

static void
show_logging_overwrite (struct ui_file *file, int from_tty,
			struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file,
		    _("Whether logging overwrites or "
		      "appends to the log file is %s.\n"),
		    value);
}

/* Value as configured by the user.  */
static int logging_redirect;

static void
set_logging_redirect (char *args, int from_tty, struct cmd_list_element *c)
{
  maybe_warn_already_logging ();
}

static void
show_logging_redirect (struct ui_file *file, int from_tty,
		       struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("The logging output mode is %s.\n"), value);
}

/* If we've pushed output files, close them and pop them.  */
static void
pop_output_files (void)
{
  current_interp_set_logging (NULL, false);

  /* Stay consistent with handle_redirections.  */
  if (!current_uiout->is_mi_like_p ())
    current_uiout->redirect (NULL);
}

/* See cli-interp.h.  */

ui_file *
make_logging_output (ui_file *curr_output, ui_file_up logfile,
		     bool logging_redirect)
{
  if (logging_redirect)
    return logfile.release ();
  else
    {
      /* Note that the "tee" takes ownership of the log file.  */
      ui_file *out = new tee_file (curr_output, false,
				   logfile.get (), true);
      logfile.release ();
      return out;
    }
}

/* This is a helper for the `set logging' command.  */
static void
handle_redirections (int from_tty)
{
  if (saved_filename != NULL)
    {
      fprintf_unfiltered (gdb_stdout, "Already logging to %s.\n",
			  saved_filename);
      return;
    }

  stdio_file_up log (new stdio_file ());
  if (!log->open (logging_filename, logging_overwrite ? "w" : "a"))
    perror_with_name (_("set logging"));

  /* Redirects everything to gdb_stdout while this is running.  */
  if (from_tty)
    {
      if (!logging_redirect)
	fprintf_unfiltered (gdb_stdout, "Copying output to %s.\n",
			    logging_filename);
      else
	fprintf_unfiltered (gdb_stdout, "Redirecting output to %s.\n",
			    logging_filename);
    }

  saved_filename = xstrdup (logging_filename);

  /* Let the interpreter do anything it needs.  */
  current_interp_set_logging (std::move (log), logging_redirect);

  /* Redirect the current ui-out object's output to the log.  Use
     gdb_stdout, not log, since the interpreter may have created a tee
     that wraps the log.  Don't do the redirect for MI, it confuses
     MI's ui-out scheme.  Note that we may get here with MI as current
     interpreter, but with the current ui_out as a CLI ui_out, with
     '-interpreter-exec console "set logging on"'.  */
  if (!current_uiout->is_mi_like_p ())
    current_uiout->redirect (gdb_stdout);
}

static void
set_logging_on (char *args, int from_tty)
{
  char *rest = args;

  if (rest && *rest)
    {
      xfree (logging_filename);
      logging_filename = xstrdup (rest);
    }
  handle_redirections (from_tty);
}

static void 
set_logging_off (char *args, int from_tty)
{
  if (saved_filename == NULL)
    return;

  pop_output_files ();
  if (from_tty)
    fprintf_unfiltered (gdb_stdout, "Done logging to %s.\n", saved_filename);
  xfree (saved_filename);
  saved_filename = NULL;
}

static void
set_logging_command (char *args, int from_tty)
{
  printf_unfiltered (_("\"set logging\" lets you log output to a file.\n"
		       "Usage: set logging on [FILENAME]\n"
		       "       set logging off\n"
		       "       set logging file FILENAME\n"
		       "       set logging overwrite [on|off]\n"
		       "       set logging redirect [on|off]\n"));
}

static void
show_logging_command (char *args, int from_tty)
{
  if (saved_filename)
    printf_unfiltered (_("Currently logging to \"%s\".\n"), saved_filename);
  if (saved_filename == NULL
      || strcmp (logging_filename, saved_filename) != 0)
    printf_unfiltered (_("Future logs will be written to %s.\n"),
		       logging_filename);

  if (logging_overwrite)
    printf_unfiltered (_("Logs will overwrite the log file.\n"));
  else
    printf_unfiltered (_("Logs will be appended to the log file.\n"));

  if (logging_redirect)
    printf_unfiltered (_("Output will be sent only to the log file.\n"));
  else
    printf_unfiltered (_("Output will be logged and displayed.\n"));
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_cli_logging;

void
_initialize_cli_logging (void)
{
  static struct cmd_list_element *set_logging_cmdlist, *show_logging_cmdlist;

  add_prefix_cmd ("logging", class_support, set_logging_command,
		  _("Set logging options"), &set_logging_cmdlist,
		  "set logging ", 0, &setlist);
  add_prefix_cmd ("logging", class_support, show_logging_command,
		  _("Show logging options"), &show_logging_cmdlist,
		  "show logging ", 0, &showlist);
  add_setshow_boolean_cmd ("overwrite", class_support, &logging_overwrite, _("\
Set whether logging overwrites or appends to the log file."), _("\
Show whether logging overwrites or appends to the log file."), _("\
If set, logging overrides the log file."),
			   set_logging_overwrite,
			   show_logging_overwrite,
			   &set_logging_cmdlist, &show_logging_cmdlist);
  add_setshow_boolean_cmd ("redirect", class_support, &logging_redirect, _("\
Set the logging output mode."), _("\
Show the logging output mode."), _("\
If redirect is off, output will go to both the screen and the log file.\n\
If redirect is on, output will go only to the log file."),
			   set_logging_redirect,
			   show_logging_redirect,
			   &set_logging_cmdlist, &show_logging_cmdlist);
  add_setshow_filename_cmd ("file", class_support, &logging_filename, _("\
Set the current logfile."), _("\
Show the current logfile."), _("\
The logfile is used when directing GDB's output."),
			    NULL,
			    show_logging_filename,
			    &set_logging_cmdlist, &show_logging_cmdlist);
  add_cmd ("on", class_support, set_logging_on,
	   _("Enable logging."), &set_logging_cmdlist);
  add_cmd ("off", class_support, set_logging_off,
	   _("Disable logging."), &set_logging_cmdlist);

  logging_filename = xstrdup ("gdb.txt");
}
