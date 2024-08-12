/* Manages interpreters for GDB, the GNU debugger.

   Copyright (C) 2000-2023 Free Software Foundation, Inc.

   Written by Jim Ingham <jingham@apple.com> of Apple Computer, Inc.

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

/* This is just a first cut at separating out the "interpreter"
   functions of gdb into self-contained modules.  There are a couple
   of open areas that need to be sorted out:

   1) The interpreter explicitly contains a UI_OUT, and can insert itself
   into the event loop, but it doesn't explicitly contain hooks for readline.
   I did this because it seems to me many interpreters won't want to use
   the readline command interface, and it is probably simpler to just let
   them take over the input in their resume proc.  */

#include "defs.h"
#include "gdbcmd.h"
#include "ui-out.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "interps.h"
#include "completer.h"
#include "top.h"		/* For command_loop.  */
#include "main.h"
#include "gdbsupport/buildargv.h"

/* Each UI has its own independent set of interpreters.  */

struct ui_interp_info
{
  /* Each top level has its own independent set of interpreters.  */
  struct interp *interp_list;
  struct interp *current_interpreter;
  struct interp *top_level_interpreter;

  /* The interpreter that is active while `interp_exec' is active, NULL
     at all other times.  */
  struct interp *command_interpreter;
};

/* Get UI's ui_interp_info object.  Never returns NULL.  */

static struct ui_interp_info *
get_interp_info (struct ui *ui)
{
  if (ui->interp_info == NULL)
    ui->interp_info = XCNEW (struct ui_interp_info);
  return ui->interp_info;
}

/* Get the current UI's ui_interp_info object.  Never returns
   NULL.  */

static struct ui_interp_info *
get_current_interp_info (void)
{
  return get_interp_info (current_ui);
}

/* The magic initialization routine for this module.  */

static struct interp *interp_lookup_existing (struct ui *ui,
					      const char *name);

interp::interp (const char *name)
  : m_name (make_unique_xstrdup (name))
{
}

interp::~interp ()
{
}

/* An interpreter factory.  Maps an interpreter name to the factory
   function that instantiates an interpreter by that name.  */

struct interp_factory
{
  interp_factory (const char *name_, interp_factory_func func_)
  : name (name_), func (func_)
  {}

  /* This is the name in "-i=INTERP" and "interpreter-exec INTERP".  */
  const char *name;

  /* The function that creates the interpreter.  */
  interp_factory_func func;
};

/* The registered interpreter factories.  */
static std::vector<interp_factory> interpreter_factories;

/* See interps.h.  */

void
interp_factory_register (const char *name, interp_factory_func func)
{
  /* Assert that no factory for NAME is already registered.  */
  for (const interp_factory &f : interpreter_factories)
    if (strcmp (f.name, name) == 0)
      {
	internal_error (_("interpreter factory already registered: \"%s\"\n"),
			name);
      }

  interpreter_factories.emplace_back (name, func);
}

/* Add interpreter INTERP to the gdb interpreter list.  The
   interpreter must not have previously been added.  */
static void
interp_add (struct ui *ui, struct interp *interp)
{
  struct ui_interp_info *ui_interp = get_interp_info (ui);

  gdb_assert (interp_lookup_existing (ui, interp->name ()) == NULL);

  interp->next = ui_interp->interp_list;
  ui_interp->interp_list = interp;
}

/* This sets the current interpreter to be INTERP.  If INTERP has not
   been initialized, then this will also run the init method.

   The TOP_LEVEL parameter tells if this new interpreter is
   the top-level one.  The top-level is what is requested
   on the command line, and is responsible for reporting general
   notification about target state changes.  For example, if
   MI is the top-level interpreter, then it will always report
   events such as target stops and new thread creation, even if they
   are caused by CLI commands.  */

static void
interp_set (struct interp *interp, bool top_level)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *old_interp = ui_interp->current_interpreter;

  /* If we already have an interpreter, then trying to
     set top level interpreter is kinda pointless.  */
  gdb_assert (!top_level || !ui_interp->current_interpreter);
  gdb_assert (!top_level || !ui_interp->top_level_interpreter);

  if (old_interp != NULL)
    {
      current_uiout->flush ();
      old_interp->suspend ();
    }

  ui_interp->current_interpreter = interp;
  if (top_level)
    ui_interp->top_level_interpreter = interp;

  if (interpreter_p != interp->name ())
    interpreter_p = interp->name ();

  bool warn_about_mi1 = false;

  /* Run the init proc.  */
  if (!interp->inited)
    {
      interp->init (top_level);
      interp->inited = true;

      if (streq (interp->name (), "mi1"))
	warn_about_mi1 = true;
    }

  /* Do this only after the interpreter is initialized.  */
  current_uiout = interp->interp_ui_out ();

  /* Clear out any installed interpreter hooks/event handlers.  */
  clear_interpreter_hooks ();

  interp->resume ();

  if (warn_about_mi1)
    warning (_("MI version 1 is deprecated in GDB 13 and "
	       "will be removed in GDB 14.  Please upgrade "
	       "to a newer version of MI."));
}

/* Look up the interpreter for NAME.  If no such interpreter exists,
   return NULL, otherwise return a pointer to the interpreter.  */

static struct interp *
interp_lookup_existing (struct ui *ui, const char *name)
{
  struct ui_interp_info *ui_interp = get_interp_info (ui);
  struct interp *interp;

  for (interp = ui_interp->interp_list;
       interp != NULL;
       interp = interp->next)
    {
      if (strcmp (interp->name (), name) == 0)
	return interp;
    }

  return NULL;
}

/* See interps.h.  */

struct interp *
interp_lookup (struct ui *ui, const char *name)
{
  if (name == NULL || strlen (name) == 0)
    return NULL;

  /* Only create each interpreter once per top level.  */
  struct interp *interp = interp_lookup_existing (ui, name);
  if (interp != NULL)
    return interp;

  for (const interp_factory &factory : interpreter_factories)
    if (strcmp (factory.name, name) == 0)
      {
	interp = factory.func (name);
	interp_add (ui, interp);
	return interp;
      }

  return NULL;
}

/* See interps.h.  */

void
set_top_level_interpreter (const char *name)
{
  /* Find it.  */
  struct interp *interp = interp_lookup (current_ui, name);

  if (interp == NULL)
    error (_("Interpreter `%s' unrecognized"), name);
  /* Install it.  */
  interp_set (interp, true);
}

void
current_interp_set_logging (ui_file_up logfile, bool logging_redirect,
			    bool debug_redirect)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *interp = ui_interp->current_interpreter;

  interp->set_logging (std::move (logfile), logging_redirect, debug_redirect);
}

/* Temporarily overrides the current interpreter.  */
struct interp *
scoped_restore_interp::set_interp (const char *name)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *interp = interp_lookup (current_ui, name);
  struct interp *old_interp = ui_interp->current_interpreter;

  if (interp)
    ui_interp->current_interpreter = interp;
  return old_interp;
}

/* Returns true if the current interp is the passed in name.  */
int
current_interp_named_p (const char *interp_name)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *interp = ui_interp->current_interpreter;

  if (interp != NULL)
    return (strcmp (interp->name (), interp_name) == 0);

  return 0;
}

/* The interpreter that was active when a command was executed.
   Normally that'd always be CURRENT_INTERPRETER, except that MI's
   -interpreter-exec command doesn't actually flip the current
   interpreter when running its sub-command.  The
   `command_interpreter' global tracks when interp_exec is called
   (IOW, when -interpreter-exec is called).  If that is set, it is
   INTERP in '-interpreter-exec INTERP "CMD"' or in 'interpreter-exec
   INTERP "CMD".  Otherwise, interp_exec isn't active, and so the
   interpreter running the command is the current interpreter.  */

struct interp *
command_interp (void)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  if (ui_interp->command_interpreter != NULL)
    return ui_interp->command_interpreter;
  else
    return ui_interp->current_interpreter;
}

/* See interps.h.  */

void
interp_pre_command_loop (struct interp *interp)
{
  gdb_assert (interp != NULL);

  interp->pre_command_loop ();
}

/* See interp.h  */

int
interp_supports_command_editing (struct interp *interp)
{
  return interp->supports_command_editing ();
}

/* interp_exec - This executes COMMAND_STR in the current 
   interpreter.  */

struct gdb_exception
interp_exec (struct interp *interp, const char *command_str)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  /* See `command_interp' for why we do this.  */
  scoped_restore save_command_interp
    = make_scoped_restore (&ui_interp->command_interpreter, interp);

  return interp->exec (command_str);
}

/* A convenience routine that nulls out all the common command hooks.
   Use it when removing your interpreter in its suspend proc.  */
void
clear_interpreter_hooks (void)
{
  deprecated_print_frame_info_listing_hook = 0;
  /*print_frame_more_info_hook = 0; */
  deprecated_query_hook = 0;
  deprecated_warning_hook = 0;
  deprecated_readline_begin_hook = 0;
  deprecated_readline_hook = 0;
  deprecated_readline_end_hook = 0;
  deprecated_context_hook = 0;
  deprecated_call_command_hook = 0;
  deprecated_error_begin_hook = 0;
}

static void
interpreter_exec_cmd (const char *args, int from_tty)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();
  struct interp *old_interp, *interp_to_use;
  unsigned int nrules;
  unsigned int i;

  /* Interpreters may clobber stdout/stderr (e.g.  in mi_interp::resume at time
     of writing), preserve their state here.  */
  scoped_restore save_stdout = make_scoped_restore (&gdb_stdout);
  scoped_restore save_stderr = make_scoped_restore (&gdb_stderr);
  scoped_restore save_stdlog = make_scoped_restore (&gdb_stdlog);
  scoped_restore save_stdtarg = make_scoped_restore (&gdb_stdtarg);
  scoped_restore save_stdtargerr = make_scoped_restore (&gdb_stdtargerr);

  if (args == NULL)
    error_no_arg (_("interpreter-exec command"));

  gdb_argv prules (args);
  nrules = prules.count ();

  if (nrules < 2)
    error (_("Usage: interpreter-exec INTERPRETER COMMAND..."));

  old_interp = ui_interp->current_interpreter;

  interp_to_use = interp_lookup (current_ui, prules[0]);
  if (interp_to_use == NULL)
    error (_("Could not find interpreter \"%s\"."), prules[0]);

  interp_set (interp_to_use, false);

  for (i = 1; i < nrules; i++)
    {
      struct gdb_exception e = interp_exec (interp_to_use, prules[i]);

      if (e.reason < 0)
	{
	  interp_set (old_interp, 0);
	  error (_("error in command: \"%s\"."), prules[i]);
	}
    }

  interp_set (old_interp, 0);
}

/* See interps.h.  */

void
interpreter_completer (struct cmd_list_element *ignore,
		       completion_tracker &tracker,
		       const char *text, const char *word)
{
  int textlen = strlen (text);

  for (const interp_factory &interp : interpreter_factories)
    {
      if (strncmp (interp.name, text, textlen) == 0)
	{
	  tracker.add_completion
	    (make_completion_match_str (interp.name, text, word));
	}
    }
}

struct interp *
top_level_interpreter (void)
{
  struct ui_interp_info *ui_interp = get_current_interp_info ();

  return ui_interp->top_level_interpreter;
}

/* See interps.h.  */

struct interp *
current_interpreter (void)
{
  struct ui_interp_info *ui_interp = get_interp_info (current_ui);

  return ui_interp->current_interpreter;
}

/* This just adds the "interpreter-exec" command.  */
void _initialize_interpreter ();
void
_initialize_interpreter ()
{
  struct cmd_list_element *c;

  c = add_cmd ("interpreter-exec", class_support,
	       interpreter_exec_cmd, _("\
Execute a command in an interpreter.\n\
Usage: interpreter-exec INTERPRETER COMMAND...\n\
The first argument is the name of the interpreter to use.\n\
The following arguments are the commands to execute.\n\
A command can have arguments, separated by spaces.\n\
These spaces must be escaped using \\ or the command\n\
and its arguments must be enclosed in double quotes."), &cmdlist);
  set_cmd_completer (c, interpreter_completer);
}
